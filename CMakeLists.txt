cmake_minimum_required(VERSION 4.4)
project(Fireworks)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
  src/main.cpp
  src/firework.cpp
  src/greeting.cpp
)
if(CMAKE_CROSSCOMPILING)
  # Host source path is meaningless on the target: ship fonts next to the executable.
  set(FIREWORKS_ASSET_DIR "assets")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/NotoSerifSC-VariableFont_wght.ttf
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/NotoSerifSC-OFL.txt
      $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/
  )
else()
  set(FIREWORKS_ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE
  FIREWORKS_ASSET_DIR="${FIREWORKS_ASSET_DIR}"
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
if(WIN32)
  # GUI subsystem (no console window); SFML::Main supplies WinMain that forwards to main().
  set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
  target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Main)
endif()
