cmake_minimum_required(VERSION 3.10)
project(PixelsGate VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find SDL2
find_package(SDL2 REQUIRED)

# Link directories for Homebrew on Apple Silicon
link_directories(/opt/homebrew/lib)
include_directories(/opt/homebrew/include)

# Find SDL2_image
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# Include directories
include_directories(src ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} /opt/homebrew/include)

# Gather source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(PixelsGate ${SOURCES})

target_include_directories(PixelsGate PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
target_link_libraries(PixelsGate PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2_TTF_LIBRARIES})

# Copy assets directory to the build directory
add_custom_command(TARGET PixelsGate POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    ${CMAKE_BINARY_DIR}/assets)
