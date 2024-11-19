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

add_library(LockboxLibrary src/library.cpp src/server.cpp src/utils.cpp src/key_manager.cpp)
target_include_directories(LockboxLibrary PUBLIC include)
target_link_libraries(LockboxLibrary Crow::Crow cpr::cpr pqxx tomlplusplus::tomlplusplus google-cloud-cpp::storage google-cloud-cpp::kms google-cloud-cpp::secretmanager)

include(FetchContent)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

FetchContent_Declare(
    libpqxx
    GIT_REPOSITORY https://github.com/jtv/libpqxx.git
    GIT_TAG        7.8.1
)

FetchContent_MakeAvailable(libpqxx)

find_package(Crow)
find_package(cpr REQUIRED)
find_package(google_cloud_cpp_storage REQUIRED)
find_package(google_cloud_cpp_kms REQUIRED)
find_package(google_cloud_cpp_secretmanager REQUIRED)

target_include_directories(MercuryLockbox PRIVATE include)
# Link the library to the main executable
target_link_libraries(MercuryLockbox PRIVATE LockboxLibrary Crow::Crow cpr::cpr pqxx tomlplusplus::tomlplusplus google-cloud-cpp::storage google-cloud-cpp::kms google-cloud-cpp::secretmanager)
