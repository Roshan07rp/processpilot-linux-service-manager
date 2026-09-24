#include "process_supervisor.h"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace {
volatile std::sig_atomic_t g_stop = 0;

void handle_signal(int) {
    g_stop = 1;
}

std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
}

ProcessSupervisor::ProcessSupervisor(const std::string& config_path)
    : config_path_(config_path) {}

std::string ProcessSupervisor::trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool ProcessSupervisor::parse_bool(const std::string& value) const {
    const auto v = lower_copy(trim(value));
    return v == "true" || v == "1" || v == "yes";
}

bool ProcessSupervisor::load_config() {
    std::ifstream file(config_path_);
    if (!file) {
        std::cerr << "[ERROR] Cannot open config: " << config_path_ << '\n';
        return false;
    }

    processes_.clear();
    std::string line;
    ProcessStatus current;
    bool in_process = false;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line == "[process]") {
            if (in_process && !current.config.name.empty())
                processes_.push_back(current);
            current = ProcessStatus{};
            in_process = true;
            continue;
        }

        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        const std::string key = trim(line.substr(0, eq));
        const std::string value = trim(line.substr(eq + 1));

        if (!in_process) {
            if (key == "interval_seconds")
                interval_seconds_ = std::max(1, std::stoi(value));
            continue;
        }

        if (key == "name") current.config.name = value;
        else if (key == "command") current.config.command = value;
        else if (key == "auto_restart") current.config.auto_restart = parse_bool(value);
        else if (key == "max_restarts")
            current.config.max_restarts = std::max(0, std::stoi(value));
    }

    if (in_process && !current.config.name.empty())
        processes_.push_back(current);

    if (processes_.empty()) {
        std::cerr << "[ERROR] No processes configured.\n";
        return false;
    }

    std::cout << "[INFO] Loaded " << processes_.size()
              << " process configuration(s), interval="
              << interval_seconds_ << "s\n";
    return true;
}

long ProcessSupervisor::find_pid(const std::string& name) const {
    std::string command = "pgrep -x '" + name + "' 2>/dev/null";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return -1;

    long pid = -1;
    char buffer[128]{};
    if (fgets(buffer, sizeof(buffer), pipe)) {
        try { pid = std::stol(buffer); }
        catch (...) { pid = -1; }
    }
    pclose(pipe);
    return pid;
}

bool ProcessSupervisor::is_process_running(ProcessStatus& process) {
    process.pid = find_pid(process.config.name);
    process.running = process.pid > 0;
    return process.running;
}

bool ProcessSupervisor::collect_usage(ProcessStatus& process) {
    if (!process.running) return false;

    std::string command =
        "ps -p " + std::to_string(process.pid) + " -o %cpu=,%mem= 2>/dev/null";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return false;

    char buffer[256]{};
    bool ok = false;
    if (fgets(buffer, sizeof(buffer), pipe)) {
        std::istringstream iss(buffer);
        double cpu = 0.0, mem = 0.0;
        if (iss >> cpu >> mem) {
            process.cpu_percent = cpu;
            process.memory_percent = mem;
            ok = true;
        }
    }
    pclose(pipe);
    return ok;
}

bool ProcessSupervisor::restart_process(ProcessStatus& process) {
    if (!process.config.auto_restart) return false;

    if (process.restart_count >= process.config.max_restarts) {
        std::cerr << "[ERROR] Restart limit reached for "
                  << process.config.name << '\n';
        return false;
    }

    std::cout << "[INFO] Attempting restart: "
              << process.config.name << '\n';

    const int rc = std::system(
        (process.config.command + " >/dev/null 2>&1 &").c_str());

    if (rc != 0) {
        std::cerr << "[ERROR] Failed to launch "
                  << process.config.name << '\n';
        return false;
    }

    ++process.restart_count;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    process.pid = find_pid(process.config.name);
    process.running = process.pid > 0;

    if (process.running) {
        std::cout << "[INFO] Restart successful. New PID: "
                  << process.pid << '\n';
        return true;
    }

    std::cerr << "[ERROR] Restart command completed but process was not detected.\n";
    return false;
}

void ProcessSupervisor::print_status() {
    std::cout << "\n========== ProcessPilot ==========\n";
    for (const auto& process : processes_) {
        std::cout << "Process        : " << process.config.name << '\n'
                  << "PID            : "
                  << (process.running ? std::to_string(process.pid) : "N/A") << '\n'
                  << "State          : "
                  << (process.running ? "RUNNING" : "STOPPED") << '\n'
                  << "CPU            : " << std::fixed << std::setprecision(1)
                  << process.cpu_percent << " %\n"
                  << "Memory         : " << process.memory_percent << " %\n"
                  << "Restart count  : " << process.restart_count << '\n'
                  << "Auto restart   : "
                  << (process.config.auto_restart ? "YES" : "NO") << '\n'
                  << "-----------------------------------\n";
    }
}

void ProcessSupervisor::run() {
    running_ = true;
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    std::cout << "[INFO] ProcessPilot started.\n";

    while (running_ && !g_stop) {
        for (auto& process : processes_) {
            if (!is_process_running(process)) {
                std::cerr << "[WARNING] Process "
                          << process.config.name
                          << " is not running.\n";
                restart_process(process);
            } else {
                collect_usage(process);
            }
        }

        print_status();
        std::this_thread::sleep_for(
            std::chrono::seconds(interval_seconds_));
    }

    running_ = false;
    std::cout << "[INFO] ProcessPilot stopped.\n";
}
