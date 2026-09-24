#include "process_supervisor.h"
#include <fstream>
#include <iostream>

int main() {
    const std::string path = "/tmp/processpilot_test.conf";
    std::ofstream out(path);
    out << "interval_seconds=2\n";
    out << "[process]\n";
    out << "name=sleep\n";
    out << "command=sleep 30\n";
    out << "auto_restart=true\n";
    out << "max_restarts=2\n";
    out.close();

    ProcessSupervisor supervisor(path);
    if (!supervisor.load_config()) {
        std::cerr << "Configuration test failed\n";
        return 1;
    }

    std::cout << "ProcessSupervisor configuration test passed\n";
    return 0;
}
