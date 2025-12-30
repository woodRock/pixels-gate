cmake_minimum_required(VERSION 3.10)
project(PixelsGate VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find SDL2
find_package(SDL2 REQUIRED)

add_executable(PixelsGate src/main.cpp)

target_include_directories(PixelsGate PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(PixelsGate PRIVATE ${SDL2_LIBRARIES})
