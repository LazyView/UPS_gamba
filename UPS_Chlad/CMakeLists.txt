cmake_minimum_required(VERSION 4.0)
project(UPS_Chlad)

set(CMAKE_CXX_STANDARD 20)

add_executable(UPS_Chlad main.cpp
        server/include/gamba_server.h
        server/include/protocol.h
        server/include/game_logic.h
        server/include/server_config.h
        server/src/game_logic.cpp
        server/src/server_config.cpp
        server/src/gamba_server.cpp
        server/src/main.cpp
)
