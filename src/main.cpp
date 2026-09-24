#include "process_supervisor.h"
#include <string>

int main(int argc, char* argv[]) {
    const std::string config =
        (argc > 1) ? argv[1] : "config/processpilot.conf";

    ProcessSupervisor supervisor(config);
    if (!supervisor.load_config()) return 1;
    supervisor.run();
    return 0;
}
