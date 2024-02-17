cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17) # set c++17 to use std::filesystem
set(CMAKE_CXX_STANDARD_REQUIRED ON)

############################################################
#                      GLOBAL OPTIONS                      #
############################################################
project(sgl VERSION 0.0.1)

# OPTION: COMPILER
set(COMPILER "MSVC" CACHE STRING "Compiler.")
set_property(CACHE COMPILER PROPERTY STRINGS "MinGW on Windows" "MSVC")
# SETUP COMPILER
if(COMPILER STREQUAL "MSVC")
  add_compile_definitions(WINDOWS)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  message(STATUS "[*] Compile on Windows." )
elseif(COMPILER STREQUAL "MinGW on Windows")
  add_compile_definitions(WINDOWS)
  add_compile_definitions(MINGW)
  message(STATUS "[*] Compile on Windows (MinGW).")
else()
  message(FATAL_ERROR "Unrecognized/unsupported compiler: ${COMPILER}.")
endif()

# SETUP C++ LANGUAGE STANDARD
if(COMPILER STREQUAL "MSVC")
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("/std:c++17" _cpp_17_supported)
  # enable c++17 for MSVC
  if (MSVC_VERSION GREATER_EQUAL "1900")
    # vs 2017+
    if (_cpp_17_supported)
      add_compile_options("/std:c++17")
    endif()
  endif()
elseif (COMPILER STREQUAL "MinGW on Windows")
endif()

# OPTION: ENABLE_OPENMP 
set(ENABLE_OPENMP "Enable" CACHE STRING "Enable OpenMP.")
set_property(CACHE ENABLE_OPENMP PROPERTY STRINGS "Enable" "Disable")
# SETUP OPENMP
if(COMPILER STREQUAL "MSVC")
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("/openmp" _openmp_supported)
  # enable/disable OpenMP for MSVC
  if (_openmp_supported AND ENABLE_OPENMP STREQUAL "Enable")
    add_compile_options("/openmp")
	  message(STATUS "[*] Enable OpenMP.")
	else ()
    add_compile_options("/openmp-")
	  message(STATUS "[*] Disable OpenMP.")
  endif()
elseif (COMPILER STREQUAL "MinGW on Windows")
  message(FATAL_ERROR "Unrecognized/unsupported compiler: ${COMPILER}.")
else()
  message(FATAL_ERROR "Unrecognized/unsupported compiler: ${COMPILER}.")
endif()

############################################################
#                     OUTPUT EXE & LIB                     #
############################################################

# "zip" (https://github.com/kuba--/zip) is used as internal library - compiled alongside with main
file(GLOB_RECURSE ZIP_LIBRARY_FILES "${CMAKE_CURRENT_SOURCE_DIR}/ext/zip/src/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/ext/zip/src/*.c") 

# sgl
file(GLOB_RECURSE ALL_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE ALL_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")

add_executable(sgl ${ALL_SRC_FILES} ${ALL_HEADER_FILES} ${ZIP_LIBRARY_FILES})

target_include_directories(sgl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ext/zip/src/")
target_include_directories(sgl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")

############################################################
#                    EXTERNAL LIBRARIES                    #
############################################################

# SDL2
if(COMPILER STREQUAL "MinGW on Windows")
	message(STATUS "* Selected compiler: MinGW on Windows")
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
if(COMPILER STREQUAL "MinGW on Windows")
  message(FATAL_ERROR "Cannot configure Assimp for MinGW on Windows. Please provide an implementation here.")
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


