cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

############################################################
#                          MACROS                          #
############################################################
# preprocessor macro
if(WIN32)
    add_compile_definitions(WINDOWS)
    message(STATUS "[*] Compile on Windows." )
elseif(UNIX)
    add_compile_definitions(LINUX)
    message(STATUS "[*] Compile on Linux.")
else()
    message(FATAL_ERROR "CMake cannot determine the platform thay you are using.")
endif()
# CRT_SECURE_NO_WARNINGS
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
endif()

############################################################
#                         OPTIONS                          #
############################################################

# COMPILER
set(COMPILER "MSVC" CACHE STRING "Compiler.")
set_property(CACHE COMPILER PROPERTY STRINGS "MinGW" "MSVC")

############################################################
#                     OUTPUT EXE & LIB                     #
############################################################

project(sgl VERSION 0.0.1)
file(GLOB_RECURSE ALL_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE ALL_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
add_executable(sgl ${ALL_SRC_FILES} ${ALL_HEADER_FILES})

target_include_directories(sgl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")


############################################################
#                    EXTERNAL LIBRARIES                    #
############################################################

# SDL2
if(COMPILER STREQUAL "MinGW")
    message(STATUS "* Selected compiler: MinGW")
    set(SDL2_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/SDL2-2.30.0/x86_64-w64-mingw32")
    target_link_libraries(sgl "${SDL2_SOURCE_DIR}/lib/libSDL2.a")
    target_link_libraries(sgl "${SDL2_SOURCE_DIR}/lib/libSDL2main.a")
    target_include_directories(sgl PUBLIC "${SDL2_SOURCE_DIR}/include/")
elseif(COMPILER STREQUAL "MSVC")
    message(STATUS "* Selected compiler: MSVC")
    set(SDL2_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/SDL2-2.30.0/x86_64-vc")
    target_link_libraries(sgl "${SDL2_SOURCE_DIR}/lib/x64/SDL2.lib")
    target_link_libraries(sgl "${SDL2_SOURCE_DIR}/lib/x64/SDL2main.lib")
    target_include_directories(sgl PUBLIC "${SDL2_SOURCE_DIR}/include/")
else()
    message(FATAL_ERROR "Invalid compiler.")
endif()

# stb_image
target_include_directories(sgl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ext/stb/")

# Assimp
set(ASSIMP_LIB_PATH "<Please paste the compiled Assimp library path.>" CACHE STRING "")
if(COMPILER STREQUAL "MinGW")
    message(FATAL_ERROR "Cannot configure Assimp for MinGW. Please provide an implementation here.")
elseif(COMPILER STREQUAL "MSVC")
    if (NOT EXISTS "${ASSIMP_LIB_PATH}")
        message(FATAL_ERROR "Please compile Assimp manually and then paste the library *.lib path to here.")
    else()
        message(STATUS "* Selected compiler: MSVC")
        set(ASSIMP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/assimp-5.3.1/")
        target_link_libraries(sgl "${ASSIMP_LIB_PATH}")
        target_include_directories(sgl PUBLIC "${ASSIMP_SOURCE_DIR}/include/")
    endif()
else()
    message(FATAL_ERROR "Invalid compiler.")
endif()


