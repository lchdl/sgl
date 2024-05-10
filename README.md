## Table of contents

- [About SGL](#about-sgl-software-graphics-library)
- [Demos and tests](#demos-and-tests)
- [Features and TODOs](#features-and-todos)
- [How to compile SGL (using CMake)](#how-to-compile-sgl-using-cmake)
- [For developers](#for-developers)

## About SGL (Software Graphics Library)

<b>SGL</b> is a tiny, light-weight cross-platform (for Windows & Linux-based systems) <i><b>software rasterizer written in C++</i></b> by strictly following the classic GPU rasterization pipeline specifications with <i><b>skeletal animation support</i></b>.
Users can program their own <i><b>vertex and fragment shaders</i></b> and attach them to the pipeline to achieve custom effects.
The overall design of the rasterization pipeline is shown below.

<p align="center">
  <img width="700" src="https://github.com/lchdl/sgl/blob/develop/demos/pipeline.png">
</p>

* SGL is currently used as my own retro-style game engine.

## Demos and tests
| Demo or test name |  Demo showcase  | Description |
|:------------------|:---------------:|:------------|
| `test_hello_world.cpp` | ![](https://github.com/lchdl/sgl/blob/develop/demos/test_hello_world.png) | A simple hello world demo. Demonstrating the <b>basic functionalities</b> of the rasterization pipeline (implemented in `sgl_pipeline.cpp`), including: perspective projection, basic texturing, custom vertex & fragment shaders. |
| `test_bone_anim.cpp` | ![](https://github.com/lchdl/sgl/blob/develop/demos/test_bone_anim.gif) | Skeletal animation & Assimp md5mesh import demo (model: <b>boblamp</b>, <b>1027 triangles</b>). Render time: <b>~1 ms</b> per frame in 320x240 resolution, <b>~3 ms</b> per frame in 800x600 resolution. |

## How to compile SGL (using CMake)

* <b>SGL relies on these external libraries</b>:

| Library |  Link  | Description |
|:-------:|:-------|:------------|
| SDL2 | https://github.com/libsdl-org/SDL | Frame buffer visualization & window message loop handling. |
| Assimp | https://github.com/assimp/assimp | Open-Asset-Importer-Library for importing model assets. |

* Before `git clone` this repository, you need to <b>manually compile</b> all external libraries and remember the paths of all compiled libraries (\*.lib/\*.a/\*.so) and header files (\*.h/\*.hpp). 

### For Windows (Visual Studio IDE)
1. If you are using <b>Visual Studio 2017</b>, things will become much more simpler, I have provided the precompiled libraries, include headers, and all external dependencies in [<b>here</b>](https://drive.google.com/file/d/11XBagdOkChDR2-2krSxKdTlhQcbmsMoI/view?usp=sharing) for download.
   
   > Otherwise, you may need to compile all external libraries using your own version of Visual Studio.

2. Using CMake build system (<b>cmake-gui</b>) to generate Visual Studio solutions. Select "<b>x64</b>" platform. CMake will prompt you to <b>specify the path for compiled libraries and the location of include headers</b> (shown below).

   <p align="center">
     <img src="https://github.com/lchdl/sgl/blob/develop/demos/cmake_windows_compile.png">
   </p>
   
   > Under the "<b>COMPILER</b>" option list, select "<b>MSVC</b>".
   
   Then, press `Configure`, `Generate`, and `Open Project` to open Visual Studio. In Visual Studio, choose `Release` or `MinSizeRel` and compile SGL (if you want to debug SGL on Windows, select "Debug").

   > After compiling SGL, you may need to copy all \*.dll files and the contents of the `res/` folder to the same location as the generated executables.

### For Linux-based systems (g++ & make)
1. Manually compile all external libraries. I have provided the precompiled libraries in [<b>here</b>](https://drive.google.com/file/d/1Z_MBPST6IFheGnUseI-6bwaOUG4MM3s3/view?usp=sharing) for download.

   > All libraries are <b>statically built in "<b>x64</b>" mode</b> using g++ <b>11.4.0</b> under Ubuntu 22.04.<br>
   > <b>NOTE</b>: If the precompiled libraries are not compatible with your development environment, you will need to compile them manually. Please also note that all libraries should be <i><b>statically linked</b></i> if you want to build SGL on Linux.

2. Make sure your g++ compiler supports <b>C++17</b> language standard. I recommend using <b>ccmake</b> to generate makefiles. Create a new empty directory and `cd` into it, then use the following command
   
   > **ccmake \<path_to_sgl_CMakeLists.txt\>**
   
   to initiate CMake build system and configure the project. ccmake will also prompt you to <b>input the path of those precompiled libraries and include headers</b>. Here are the configurations

   > Under the "<b>COMPILER</b>" option list, select "<b>GCC</b>".<br>
   > Under the "<b>BUILD_TYPE</b>" option list, select "<b>Release</b>" (if you want to debug SGL on Linux, select "Debug").<br>

   Then, ccmake will prompt you to provide the file paths of the precompiled libraries (\*.a) and headers (\*.h). After filling in all the paths, the final configuration should look like this:

   <p align="center">
     <img width=800 src="https://github.com/lchdl/sgl/blob/develop/demos/cmake_linux_compile.png">
   </p>

   Hit `g` to generate makefile and ccmake will automatically quit if succeed.

   Finally, `make -jN` to compile SGL using `N` threads (such as `make -j8`) and wait for it to finish.
   
   > After compiling SGL, you may need to copy all the contents of the `res/` folder to the same location as the generated executables.

## For developers
### Project structure
* CMakeLists.txt
  - For CMAKE.
* `demos/`
  - All demo outputs (*.png, *.gif).
* `include/`
  - Header files and inline implementations (*.h).
* `src/`
  - Source files (*.cpp).
* `tests/`
  - All CI test cases (*.cpp sources will be compiled and linked as executables).
* `doc/`
  - Documentations, devlog, tutorials, notes, etc.
* `res/`
  - Resource files (models, textures, etc.).
* `ext/` <i><b>(not included in this repo)</b></i>
  - External dependencies, including the source code for all 3rd-party libraries.
* `build/` <i><b>(not included in this repo)</b></i>
  - Project files generated by CMAKE/CCMAKE under different platforms.
  - For example, `build/gcc_x64_static/` stores all the project files generated for 64-bit gcc/g++ compilers, using static linking. `build/vs2017/` stores all the project files generated for the Microsoft Visual Studio 2017.
* `build_deps/` <i><b>(not included in this repo)</b></i>
  - Dependencies for building the project. Which contains:
    - Include headers of 3rd-party libraries.
    - Compiled libraries (*.lib/*.a) and shared objects / DLLs (*.dll/*.so).
  - Usually all the files in this folder are collected from `ext/`. You need to compile all the 3rd-party libraries in `ext/` and then copy the generated binaries and include headers to this folder.

### Features and TODOs
#### Features
* Flexible vertex format
* Vertex & fragment shader support
* md5 (*.md5mesh, *.md5anim) format import & parsing
* <b>Skeletal animation</b> support
#### TODOs
* Wireframe rendering
* Shadow mapping
* Reflection effect
  - Stencil buffer
  - Alpha blending
* Text rendering
* Phong shading
* Texture baking
  - Ray tracing
