cmake_minimum_required(VERSION 3.7)
project(SnakeGame-Server C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")


find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})


add_executable(SnakeGame-Server Server.c Server.h)
add_executable(SnakeGame-Client Client.c Client.h)
target_link_libraries(SnakeGame-Server ${CURSES_LIBRARIES})

project(SnakeGame-Client C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")


find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

target_link_libraries(SnakeGame-Client ${CURSES_LIBRARIES})
