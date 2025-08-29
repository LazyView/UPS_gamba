cmake_minimum_required(VERSION 4.0)
project(UPS_Chlad)

set(CMAKE_CXX_STANDARD 20)

add_executable(UPS_Chlad main.cpp
        server/src/basic_sever.cpp
        server/src/basic_sever.h
        server/src/protocol.h
        server/src/basic_sever.cpp)
