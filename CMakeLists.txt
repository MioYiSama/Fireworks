cmake_minimum_required(VERSION 3.31)
project(Fireworks)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
    message("DEBUG")
    add_executable(${PROJECT_NAME})
else ()
    add_executable(${PROJECT_NAME} WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE -static)
endif ()

target_sources(${PROJECT_NAME} PRIVATE main.cpp ParticleText.cpp util.cpp)
target_precompile_headers(${PROJECT_NAME} PRIVATE pch.h)

include(FetchContent)
FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.1
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(SFML)

target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Graphics)