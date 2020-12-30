cmake_minimum_required(VERSION 3.7)
project(SnakeGame C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")


find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})


add_executable(SnakeGame main.c)
target_link_libraries(SnakeGame ${CURSES_LIBRARIES})