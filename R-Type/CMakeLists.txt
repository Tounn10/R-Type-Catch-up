cmake_minimum_required(VERSION 3.14)
project(R-Type_Library)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)

# (Optional) Include directories if needed
include_directories(
    ${CMAKE_SOURCE_DIR}/Server/include
    ${CMAKE_SOURCE_DIR}/R-Type/include
    ${CMAKE_SOURCE_DIR}/R-Type/include/Entity
    ${CMAKE_SOURCE_DIR}/ECS
    ${CMAKE_SOURCE_DIR}/ECS/systems
    ${CMAKE_SOURCE_DIR}/ECS/components
)

# The R-Type "core" library sources
set(RTYPE_SOURCES
    src/PlayerAction.cpp
    src/Entity/GeneralEntity.cpp
)

set(GAMESTATE_SOURCES
        src/GameState.cpp
        src/PlayerAction.cpp
        src/Entity/GeneralEntity.cpp
)

add_library(R-Type ${RTYPE_SOURCES})

add_library(GameState SHARED ${GAMESTATE_SOURCES})
set_target_properties(GameState PROPERTIES OUTPUT_NAME "GameState" POSITION_INDEPENDENT_CODE ON)

set(PONG_SOURCES
    src/Pong.cpp
    src/PlayerAction.cpp
    src/Entity/GeneralEntity.cpp
)

add_library(Pong SHARED ${PONG_SOURCES})
set_target_properties(Pong PROPERTIES OUTPUT_NAME "Pong" POSITION_INDEPENDENT_CODE ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/R-Type/)

# Link against Boost::Boost, SFML::SFML, ECSLib if needed
target_link_libraries(R-Type
    Boost::Boost
    SFML::SFML
    ECSLib
)
