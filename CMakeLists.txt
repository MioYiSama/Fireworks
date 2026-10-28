cmake_minimum_required(VERSION 4.4)
project(Fireworks)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
  src/main.cpp
  src/firework.cpp
  src/greeting.cpp
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
  FIREWORKS_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_EXTENSIONS OFF
  INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
)

include(FetchContent)
FetchContent_Declare(
  SFML
  URL https://github.com/SFML/SFML/archive/refs/tags/3.1.0.tar.gz
)
FetchContent_MakeAvailable(SFML)
target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Graphics)
