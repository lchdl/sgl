# Table of contents

- [About SGL](#about-sgl-software-graphics-library)
- [Demos and tests](#demos-and-tests)
- [Features and TODOs](#features-and-todos)
- [How to compile (CMake)](#how-to-compile-cmake)
- [For developers](#for-developers)

# About SGL (software graphics library)

<b>SGL</b> is a tiny cross-platform (Windows & Linux-based systems) <i><b>software rasterizer written in C++</i></b> by strictly following the classic GPU rasterization pipeline specifications with <i><b>skeletal animation support</i></b>.
Users can program their own <i><b>vertex and fragment shaders</i></b> and attach them to the pipeline to achieve custom effects.

* Currently used as my own retro-style game engine.

# Demos and tests
| Demo or test name | Demo | Description |
|:------------------|:----:|:------------|
| test_hello_world.cpp | ![](https://github.com/lchdl/sgl/blob/develop/demos/test_hello_world.png) | A simple hello world demo. |
| test_bone_anim.cpp | ![](https://github.com/lchdl/sgl/blob/develop/demos/test_bone_anim.gif) | Skeletal animation & Assimp md5mesh import demo (model: <b>boblamp</b>, <b>1027 triangles</b>). Render time: <b>~1 ms</b> per frame in 320x240 resolution, <b>~3 ms</b> per frame in 800x600 resolution. |

# Features and TODOs
## Features
* Flexible vertex format
* Vertex & fragment shader support
* *.md5 mesh import
* Skeletal animation support
## TODOs
* Reflection effect
  - Stencil buffer
  - Alpha blending
* Text rendering
* Phong shading
* Texture baking
  - Ray tracing

# How to compile (CMake)
## For Windows

## For Linux-based systems

# For developers
## Project structure
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
* `ext/` <i><b>(not included in repo)</b></i>
  - External dependencies, including the source code for all 3rd-party libraries.
* `build/` <i><b>(not included in repo)</b></i>
  - Project files generated by CMAKE/CCMAKE under different platforms.
  - For example, `build/gcc_x64_static/` stores all the project files generated for 64-bit gcc/g++ compilers, using static linking. `build/vs2017/` stores all the project files generated for the Microsoft Visual Studio 2017.
* `build_deps/` <i><b>(not included in repo)</b></i>
  - Dependencies for building the project. Which contains:
    - Include headers of 3rd-party libraries.
    - Compiled libraries (*.lib/*.a) and shared objects / DLLs (*.dll/*.so).
  - Usually all the files in this folder are collected from `ext/`. You need to compile all the 3rd-party libraries in `ext/` and then copy the generated binaries and include headers to this folder.
