cmake_minimum_required(VERSION 3.13.2)
project(demo C)

set(CMAKE_C_STANDARD 99)

add_executable(demo main.c store.c segmentation.c)
add_executable(file file.c time_utils.c)
add_executable(server server.c)
add_executable(client client.c)
add_executable(signal signal.c time_utils.c)
add_executable(volatile volatile.c)
add_executable(non_volatile non_volatile.c)
add_executable(asm asm.c)
add_executable(lock_addx lock_addx.c)
add_executable(frame frame.c)
add_executable(def def.c)

add_definitions(-DCC_INTR)
add_executable(branch branch.c)
add_executable(size_demo size_demo.c)
add_executable(template tempate.c)
