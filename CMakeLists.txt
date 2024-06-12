cmake_minimum_required(VERSION 3.5)

# Set the project name and the C++ standard
project(MyRaylibProject VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the path to the Raylib library
add_subdirectory(dep/raylib)

# Add the executable
add_executable(my_game main.cpp)

# Link the Raylib library
target_link_libraries(my_game raylib)

# Include directories for Raylib
target_include_directories(my_game PRIVATE dep/raylib/src)