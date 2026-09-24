#pragma once
#include <string>
#include <vector>

struct ProcessConfig {
    std::string name;
    std::string command;
    bool auto_restart{false};
    int max_restarts{3};
};

struct ProcessStatus {
    ProcessConfig config;
    long pid{-1};
    bool running{false};
    double cpu_percent{0.0};
    double memory_percent{0.0};
    int restart_count{0};
};

class ProcessSupervisor {
public:
    explicit ProcessSupervisor(const std::string& config_path);
    bool load_config();
    void run();

private:
    std::string config_path_;
    int interval_seconds_{5};
    bool running_{false};
    std::vector<ProcessStatus> processes_;

    bool parse_bool(const std::string& value) const;
    bool is_process_running(ProcessStatus& process);
    bool collect_usage(ProcessStatus& process);
    bool restart_process(ProcessStatus& process);
    long find_pid(const std::string& name) const;
    void print_status();
    static std::string trim(const std::string& value);
};
