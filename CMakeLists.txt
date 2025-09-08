cmake_minimum_required(VERSION 3.31)
project(Fireworks)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(${PROJECT_NAME})

if (WIN32 AND NOT ${CMAKE_BUILD_TYPE} STREQUAL Debug)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            WIN32_EXECUTABLE TRUE
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE -static)
endif ()

target_sources(${PROJECT_NAME} PRIVATE src/main.cpp src/ParticleText.cpp src/util.cpp)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

include(FetchContent)
FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.1
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(SFML)

target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Graphics)