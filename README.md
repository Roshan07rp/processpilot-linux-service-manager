# ProcessPilot – Linux Service Manager and Process Supervisor

ProcessPilot is a lightweight Linux process supervisor written in C++17. It monitors configured processes, reports their state, restarts failed processes when enabled, and records recovery events.

## Features
- Process health monitoring
- Configurable polling interval
- Automatic restart
- Maximum restart limit
- CPU and memory monitoring
- Simple configuration
- CMake build system
- Unit test

## Structure
```text
processpilot/
├── config/processpilot.conf
├── docs/architecture.md
├── include/process_supervisor.h
├── scripts/run_processpilot.sh
├── src/main.cpp
├── src/process_supervisor.cpp
├── systemd/processpilot.service
├── tests/test_process_supervisor.cpp
├── CMakeLists.txt
├── .gitignore
└── README.md
```

## Build on Ubuntu / WSL
```bash
sudo apt update
sudo apt install -y build-essential cmake procps
cmake -S . -B build
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

## Run
```bash
./build/processpilot config/processpilot.conf
```

Press Ctrl+C to stop.
