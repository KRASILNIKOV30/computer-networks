cmake_minimum_required(VERSION 3.10)

project(udp_pinger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(../lib)

add_executable(udp_pinger_client udp_pinger_client.cpp)
add_executable(udp_pinger_server udp_pinger_server.cpp)