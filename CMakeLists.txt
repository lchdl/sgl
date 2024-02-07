cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(tinyvis VERSION 0.0.1)
file(GLOB_RECURSE ALL_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(tinyvis ${ALL_SRC_FILES})

target_include_directories(tinyvis PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")

# SDL2
set(SDL2_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/SDL2-2.30.0/x86_64-w64-mingw32")
target_link_libraries(tinyvis "${SDL2_SOURCE_DIR}/lib/libSDL2.a")
target_link_libraries(tinyvis "${SDL2_SOURCE_DIR}/lib/libSDL2main.a")
target_include_directories(tinyvis PUBLIC "${SDL2_SOURCE_DIR}/include/")

