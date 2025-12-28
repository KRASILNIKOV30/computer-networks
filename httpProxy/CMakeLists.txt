cmake_minimum_required(VERSION 3.17)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(../lib)

# Находим потоки (важно для Linux)
find_package(Threads REQUIRED)

add_executable(http-proxy
        ../lib/FileDesc.h
        ../lib/Acceptor.h
        ../lib/Connection.h
        ../lib/Socket.h
        main.cpp
        HttpCache.h
        HttpUtils.h
        UpstreamConnection.h
)
