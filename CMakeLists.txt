cmake_minimum_required(VERSION 3.16)
project(ProcessPilot VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(processpilot src/main.cpp src/process_supervisor.cpp)
target_include_directories(processpilot PRIVATE include)
target_compile_options(processpilot PRIVATE -Wall -Wextra -Wpedantic)

include(CTest)
enable_testing()
add_executable(test_process_supervisor tests/test_process_supervisor.cpp src/process_supervisor.cpp)
target_include_directories(test_process_supervisor PRIVATE include)
target_compile_options(test_process_supervisor PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME process_supervisor_test COMMAND test_process_supervisor)
