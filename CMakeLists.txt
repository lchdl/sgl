cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(tinyvis VERSION 0.0.1)
file(GLOB_RECURSE ALL_SRC_FILES src/*.c src/*.cpp)
add_executable(tinyvis ${ALL_SRC_FILES})

target_include_directories(tinyvis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
