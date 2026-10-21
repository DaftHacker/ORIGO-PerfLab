cmake_minimum_required(VERSION 3.20)
project(ORIGO_PerfLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT ORIGO_ENGINE_SOURCE_DIR)
    message(FATAL_ERROR "ORIGO_ENGINE_SOURCE_DIR must point to the ORIGO Engine source tree")
endif()

file(GLOB_RECURSE ORIGO_GAME_SCRIPT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Scripts/*.cpp")

add_library(ORIGO_GAME SHARED
    GameModule/GameModule.cpp
    ${ORIGO_GAME_SCRIPT_SOURCES}
)

if(EXISTS "${ORIGO_ENGINE_SOURCE_DIR}/Engine/CMake/OrigoEngine.cmake")
    include("${ORIGO_ENGINE_SOURCE_DIR}/Engine/CMake/OrigoEngine.cmake")
    set(ORIGO_GAME_ENGINE_INCLUDES ${ORIGO_ENGINE_INCLUDE_DIRS})
elseif(EXISTS "${ORIGO_ENGINE_SOURCE_DIR}/cmake/OrigoEngine.cmake")
    include("${ORIGO_ENGINE_SOURCE_DIR}/cmake/OrigoEngine.cmake")
    set(ORIGO_GAME_ENGINE_INCLUDES ${ORIGO_ENGINE_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Could not locate OrigoEngine.cmake under ORIGO_ENGINE_SOURCE_DIR")
endif()

target_include_directories(ORIGO_GAME PRIVATE
    ${ORIGO_GAME_ENGINE_INCLUDES}
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Scripts"
)
set_target_properties(ORIGO_GAME PROPERTIES OUTPUT_NAME "ORIGO_GAME" PREFIX "")
