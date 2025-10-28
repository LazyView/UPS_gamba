cmake_minimum_required(VERSION 4.0)
project(UPS_Chlad)

set(CMAKE_CXX_STANDARD 20)

add_executable(UPS_Chlad main.cpp
        server/include/core/server_config.h
        server/src/core/ServerConfig.cpp
        server/src/main.cpp
        server/src/core/PlayerManager.cpp
        server/include/core/PlayerManager.h
        server/src/core/RoomManager.cpp
        server/include/core/RoomManager.h
        server/src/core/Logger.cpp
        server/include/core/Logger.h
        server/src/game/GameLogic.cpp
        server/include/game/GameLogic.h
        server/src/game/CardDeck.cpp
        server/include/game/CardDeck.h
        server/src/game/GameRules.cpp
        server/include/game/GameRules.h
        server/src/network/MessageHandler.cpp
        server/include/network/MessageHandler.h
        server/src/network/NetworkManager.cpp
        server/include/network/NetworkManager.h
        server/include/core/Player.h
        server/include/core/Room.h
        server/src/network/MessageValidator.cpp
        server/include/network/MessageValidator.h
        server/src/network/MessageParser.cpp
        server/include/network/MessageParser.h
        server/include/network/MessageType.h
        server/src/protocol/ProtocolMessage.cpp
        server/src/protocol/ProtocolHelper.cpp
        server/include/protocol/ProtocolHelper.h
        server/include/protocol/ProtocolMessage.h
        server/src/core/GameManager.cpp
        server/include/core/GameManager.h
)

# Add include directories
target_include_directories(UPS_Chlad PRIVATE server/include)
