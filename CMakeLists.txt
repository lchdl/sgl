cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17) # set c++17 to use std::filesystem
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # export to enable clangd LSP
include(CheckCXXCompilerFlag)

############################################################
#                      GLOBAL OPTIONS                      #
############################################################
project(sgl VERSION 0.0.1)

# OPTION: COMPILER
set(COMPILER "<Please select compiler.>" CACHE STRING "Compiler.")
set_property(CACHE COMPILER PROPERTY STRINGS "MSVC" "GCC")
if (COMPILER STREQUAL "<Please select compiler.>")
  message(FATAL_ERROR "Please select \"COMPILER\".")
endif()

# OPTION: BUILD TYPE (for non-IDE environments only)
if (COMPILER STREQUAL "MSVC")
  # MSVC IDE will let users select the build mode (Debug/Release) when building
elseif (COMPILER STREQUAL "GCC")
  set(BUILD_TYPE "<Please select build type.>" CACHE STRING "")
  set_property(CACHE BUILD_TYPE PROPERTY STRINGS "Release" "Debug")
  if (BUILD_TYPE STREQUAL "<Please select build type.>")
    message(FATAL_ERROR "Please select \"BUILD_TYPE\".")
  else()
    if (BUILD_TYPE STREQUAL "Release")
      set(CMAKE_CXX_FLAGS "-Wall -std=c++17 -O3 -DNDEBUG")
      # -DNDEBUG will define macro "NDEBUG", which will turn off standard library assertions.
    else()
      # Debug
      set(CMAKE_CXX_FLAGS "-Wall -std=c++17 -O0 -g")
    endif()
  endif()
else()
  message(FATAL_ERROR "Invalid compiler selected.")
endif()

# SETUP COMPILER
if(COMPILER STREQUAL "MSVC")
  add_compile_definitions(WINDOWS)
  add_compile_definitions(MSVC_COMPILER)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  message(STATUS "[*] Compile on Windows." )
elseif(COMPILER STREQUAL "GCC")
  add_compile_definitions(LINUX)
  add_compile_definitions(GCC_COMPILER)
  message(STATUS "[*] Compile on Linux (GCC).")
  # manually setting compiler paths
  set(C_COMPILER_PATH "/usr/bin/gcc" CACHE STRING "")
  set(CXX_COMPILER_PATH "/usr/bin/g++" CACHE STRING "")
  if (NOT EXISTS "${C_COMPILER_PATH}" OR NOT EXISTS "${CXX_COMPILER_PATH}")
    message(FATAL_ERROR "Cannot find C/C++ compiler! Make sure \"C_COMPILER_PATH\" and \"CXX_COMPILER_PATH\" are both correctly set.")
  else()
    set(CMAKE_C_COMPILER ${C_COMPILER_PATH})
    set(CMAKE_CXX_COMPILER ${CXX_COMPILER_PATH})
    message(STATUS "Found C/C++ compilers: \"${C_COMPILER_PATH}\" and \"${CXX_COMPILER_PATH}\".")
  endif()
else()
  message(FATAL_ERROR "Unrecognized/unsupported compiler: ${COMPILER}.")
endif()

# SETUP C++ LANGUAGE STANDARD
if(COMPILER STREQUAL "MSVC")
  CHECK_CXX_COMPILER_FLAG("/std:c++17" _cpp_17_supported)
  # enable c++17 for MSVC
  if (MSVC_VERSION GREATER_EQUAL "1900")
    # vs 2017+
    if (_cpp_17_supported)
      add_compile_options("/std:c++17")
    endif()
  endif()
elseif (COMPILER STREQUAL "GCC")
  CHECK_CXX_COMPILER_FLAG("-std=c++17" _cpp_17_supported)
  if(_cpp_17_supported)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
  else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no c++17 support.")
  endif()
endif()

# OPTION: ENABLE_OPENMP 
set(ENABLE_OPENMP "Enable" CACHE STRING "Enable OpenMP.")
set_property(CACHE ENABLE_OPENMP PROPERTY STRINGS "Enable" "Disable")
# SETUP OPENMP
if(COMPILER STREQUAL "MSVC")
  CHECK_CXX_COMPILER_FLAG("/openmp" _openmp_supported)
  # enable/disable OpenMP for MSVC
  if (_openmp_supported AND ENABLE_OPENMP STREQUAL "Enable")
    add_compile_options("/openmp")
    message(STATUS "[*] Enable OpenMP.")
  else ()
    add_compile_options("/openmp-")
    message(STATUS "[*] Disable OpenMP.")
  endif()
elseif (COMPILER STREQUAL "GCC")
  CHECK_CXX_COMPILER_FLAG("-fopenmp" _openmp_supported)
  if (_openmp_supported AND ENABLE_OPENMP STREQUAL "Enable")
    add_compile_options("-fopenmp")
    add_link_options("-fopenmp")
    message(STATUS "[*] Enable OpenMP.")
  else()
    message(STATUS "[*] Disable OpenMP.")
  endif()
else()
  message(FATAL_ERROR "Unrecognized/unsupported compiler: ${COMPILER}.")
endif()

############################################################
#                       MAIN FILES &                       #
#                    INTERNAL LIBRARIES                    #
############################################################

# zip (https://github.com/kuba--/zip) is used as internal library - compiled alongside with sgl
file(GLOB_RECURSE ZIP_LIBRARY_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/zip/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/zip/*.c") 

# sgl
## sgl main library
file(GLOB_RECURSE SGL_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE SGL_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
add_library(sgl ${SGL_SRC_FILES} ${SGL_HEADER_FILES} ${ZIP_LIBRARY_FILES})
## CI tests for sgl library
file(GLOB_RECURSE SGL_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
foreach(test_source ${SGL_TESTS})
    get_filename_component(test_name ${test_source} NAME_WE) # cut off the file extension and directory path
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} sgl) # make sure sgl is linked to each CI test
endforeach(test_source ${SGL_TESTS})

# includes
target_include_directories(sgl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
target_include_directories(sgl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/stb/") # stb_image
target_include_directories(sgl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/zip/") # mini zip

############################################################
#                    EXTERNAL LIBRARIES                    #
############################################################

# SDL2
if(COMPILER STREQUAL "MSVC")
  # Setting up SDL2.lib, SDL2main.lib and SDL2 include dir
  set(SDL2_LIB_PATH "<Please paste the compiled SDL2 library path (SDL2.lib).>" CACHE STRING "")
  set(SDL2MAIN_LIB_PATH "<Please paste the path of SDL2main.lib.>" CACHE STRING "")
  set(SDL2_INCLUDE_DIR "<Please paste the path of SDL2 include dir.>" CACHE STRING "")
  if (NOT EXISTS "${SDL2_LIB_PATH}")
    message(FATAL_ERROR "Please paste the path of SDL2.lib.")
  endif()
  if (NOT EXISTS "${SDL2MAIN_LIB_PATH}")
    message(FATAL_ERROR "Please paste the path of SDL2main.lib.")
  endif()
  if (NOT EXISTS "${SDL2_INCLUDE_DIR}")
    message(FATAL_ERROR "Please paste the path of SDL2 include dir.")
  endif()
  target_link_libraries(sgl "${SDL2_LIB_PATH}")
  target_link_libraries(sgl "${SDL2MAIN_LIB_PATH}")
  target_include_directories(sgl PUBLIC "${SDL2_INCLUDE_DIR}")
elseif(COMPILER STREQUAL "GCC")
  # Setting up libSDL2.a, libSDL2main.a, and SDL2 include dir
  set(SDL2_LIB_PATH "<Please paste the compiled SDL2 library path (libSDL2.a).>" CACHE STRING "")
  set(SDL2MAIN_LIB_PATH "<Please paste the path of libSDL2main.a.>" CACHE STRING "")
  set(SDL2_INCLUDE_DIR "<Please paste the path of SDL2 include dir.>" CACHE STRING "")
  if (NOT EXISTS "${SDL2_LIB_PATH}")
    message(FATAL_ERROR "Please paste the path of libSDL2.a.")
  endif()
  if (NOT EXISTS "${SDL2MAIN_LIB_PATH}")
    message(FATAL_ERROR "Please paste the path of libSDL2main.a.")
  endif()
  if (NOT EXISTS "${SDL2_INCLUDE_DIR}")
    message(FATAL_ERROR "Please paste the path of SDL2 include dir.")
  endif()
  target_link_libraries(sgl "${SDL2_LIB_PATH}")
  target_link_libraries(sgl "${SDL2MAIN_LIB_PATH}")
  target_include_directories(sgl PUBLIC "${SDL2_INCLUDE_DIR}")
else()
  message(FATAL_ERROR "Invalid compiler.")
endif()

# Assimp
if(COMPILER STREQUAL "MSVC")
  set(ASSIMP_LIB_PATH "<Please paste the compiled Assimp library path (assimp-*.lib).>" CACHE STRING "")
  #set(ASSIMP_ZLIB_PATH "<Please paste the compiled zlib library (libzlibstatic.a). It will be generated when statically compiling Assimp.>" CACHE STRING "")
  set(ASSIMP_INCLUDE_DIR "<Please paste the path of Assimp include dir.>" CACHE STRING "")
  if (NOT EXISTS "${ASSIMP_LIB_PATH}")
    message(FATAL_ERROR "Cannot find libassimp. Please compile Assimp manually and then paste the library (*.lib) path to here.")
  endif()
  if (NOT EXISTS "${ASSIMP_INCLUDE_DIR}")
    message(FATAL_ERROR "Please paste the path of Assimp include dir here.")
  endif()
  target_link_libraries(sgl "${ASSIMP_LIB_PATH}")
  target_include_directories(sgl PUBLIC "${ASSIMP_INCLUDE_DIR}")
elseif(COMPILER STREQUAL "GCC")
  # if compiler is gcc, then assimp and its internal zlib should build statically,
  # and developer should tell cmake the path of statically linked zlib here.
  set(ASSIMP_LIB_PATH "<Please paste the compiled Assimp library path (libassimp.a).>" CACHE STRING "")
  set(ASSIMP_ZLIB_PATH "<Please paste the compiled zlib library (libzlibstatic.a). It will be generated when statically compiling Assimp.>" CACHE STRING "")
  set(ASSIMP_INCLUDE_DIR "<Please paste the path of Assimp include dir.>" CACHE STRING "")
  if (NOT EXISTS "${ASSIMP_LIB_PATH}")
    message(FATAL_ERROR "Cannot find libassimp. Please compile Assimp manually and then paste the library *.a path to here. Also make sure to compile Assimp and its assiciated zlib in static mode.")
  endif()
  if (NOT EXISTS "${ASSIMP_ZLIB_PATH}")
    message(FATAL_ERROR "Cannot find zlib (libzlibstatic.a) built by libassimp. Please specify the correct path of zlib. NOTE: libzlibstatic.a can be found in .../contrib/, use \"find\" command.")
  endif()
  if (NOT EXISTS "${ASSIMP_INCLUDE_DIR}")
    message(FATAL_ERROR "Please paste the path of Assimp include dir here.")
  endif()
  target_link_libraries(sgl "${ASSIMP_LIB_PATH}")
  target_link_libraries(sgl "${ASSIMP_ZLIB_PATH}")
  target_include_directories(sgl PUBLIC "${ASSIMP_INCLUDE_DIR}")
 else()
  message(FATAL_ERROR "Invalid compiler.")
endif()


