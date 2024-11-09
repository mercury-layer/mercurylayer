# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.15)

# Define the project name and language
project(MercuryLockbox VERSION 0.1
                    DESCRIPTION "Mercury Lockbox"
                    LANGUAGES CXX)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the executable and source files
add_executable(MercuryLockbox src/main.cpp)

# Include directories (e.g., header files in include/)
# target_include_directories(MercuryLockbox PRIVATE include)

add_library(LockboxLibrary src/library.cpp)
target_include_directories(LockboxLibrary PUBLIC include)

find_package(Crow)

# Link the library to the main executable
target_link_libraries(MercuryLockbox PRIVATE LockboxLibrary Crow::Crow)
