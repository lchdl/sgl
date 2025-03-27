## Table of contents

- [About SGL](#about-sgl-software-graphics-library)
- [Demos and tests](#demos-and-tests)
- [How to compile SGL (using CMake)](#how-to-compile-sgl-using-cmake)
- [For developers](#for-developers)

## About SGL (Software Graphics Library)

<b>SGL</b> is a tiny, light-weight, cross-platform <i><b>software rasterizer</i></b> for Windows and Linux-based systems, written in C++. It <i><b>strictly follows the classic GPU rasterization pipeline specifications</b></i> and supports <i><b>skeletal animation</i></b>.
Users can program their own <i><b>vertex and fragment shaders</i></b> and attach them to the pipeline to achieve custom effects.
The overall design of the rasterization pipeline is shown below.

<p align="center">
  <img width="700" src="https://github.com/lchdl/sgl/blob/develop/assets/readme/pipeline.png">
</p>

* Currently, SGL is designed to <i><b>serve as a graphical backend for retro-style game engines</b></i>. Rendering something onto the screen with SGL can be <i><b>very simple</b></i>: simply provide the vertex positions, texture coordinates, and normals. SGL will then draw the model for you without any help from the dedicated GPU.

* <b><i>Write your own vertex and fragment shaders for the software rasterizer</i></b>: SGL supports customized vertex and fragment shaders. The image below shows an example:
  <p align="center">
    <img src="https://github.com/lchdl/sgl/blob/develop/assets/readme/shaders_code_demo.png">
  </p>

* Due to the benefit of abstracting the entire rendering operation into multiple <b><i>render passes</i></b>, SGL can also easily support hardware acceleration (GPU rendering using OpenGL) to render complex scenes. This feature is currently working in progress.

## Demos and tests
| Demo or test name |  Demo showcase  | Description |
|:------------------|:---------------:|:------------|
| `test_hello_world.cpp` | ![](https://github.com/lchdl/sgl/blob/develop/assets/readme/test_hello_world.png) | A simple hello world demo. Demonstrating the <b>basic functionalities</b> of the rasterization pipeline (implemented in `sgl_pipeline.h`), including: perspective projection, basic texturing, custom vertex & fragment shaders. |
| `test_skeletal_anim.cpp` | ![](https://github.com/lchdl/sgl/blob/develop/assets/readme/test_bone_anim.gif) | Skeletal animation & Assimp md5mesh import demo (model: <b>boblamp</b>, <b>1027 triangles</b>). Including orthographic / perspective projection. |
| `test_sprite_render.cpp` | ![](https://github.com/lchdl/sgl/blob/develop/assets/readme/test_sprite_render.png) | A simple demo for rendering multiple 2D sprites onto a texture with just a few lines of code. The chess sprites used in this demo were downloaded from [here](https://dani-maccari.itch.io/pixel-chess). |
| `test_normal_mapping.cpp` | ![](https://github.com/lchdl/sgl/blob/develop/assets/readme/test_normal_mapping.png) | Demonstration of normal mapping techniques and nearest/linear texture sampling. |
| `test_font_rendering.cpp` | ![](https://github.com/lchdl/sgl/blob/develop/assets/readme/test_font_rendering.png) | Stylized font rendering with decent rasterization performance. |
| `test_opengl.cpp` | ![](https://github.com/lchdl/sgl/blob/develop/assets/readme/test_opengl.png) | A simple OpenGL demo showcasing font rendering, framebuffer objects, sprite rendering, and mesh rendering. |
| `test_opengl_MRT.cpp` | ![](https://github.com/lchdl/sgl/blob/develop/assets/readme/test_opengl_MRT.png) | OpenGL Multiple Render Targets (MRT) demonstration. |

### Performance benchmarking
| Demo or test name | Description |
|:------------------|:------------|
| `test_skeletal_anim.cpp` | CPU: Core i7-3770 8 cores 3.4 GHz.<br>Mesh & render info: <b>1027</b> triangles with backface culling.<br>OpenMP multi-thread: <b>1.78 ms</b> per frame in classic 320x240 resolution, <b>3.69 ms</b> per frame in 800x600 resolution. Single-thread: <b>2.14ms</b> in classic 320x240 resolution, <b>8.31ms</b> in 800x600 resolution. |

* Now SGL supports multiple internal texture formats (BGRA, RGBA, etc.). Using the <b>native texture format</b> (BGRA8888) can gain an extra <b>8%</b> in performance (3.69 ms to 3.40 ms).

## How to compile SGL (using CMake)

* <b>SGL relies on these external libraries</b>:

| Library |  Link  | Description |
|:-------:|:-------|:------------|
| SDL2 | https://github.com/libsdl-org/SDL | Frame buffer visualization & window message loop handling. |
| Assimp | https://github.com/assimp/assimp | Open-Asset-Importer-Library for importing model assets. |

* Before `git clone` this repository, you need to <b>manually compile</b> all external libraries and remember the paths of all compiled libraries (\*.lib/\*.a/\*.so) and header files (\*.h/\*.hpp).
* For convenience, the <b>precompiled binaries</b> for Windows and Linux-based systems are provided below.

### For Windows (Visual Studio IDE)
1. If you are using <b>Visual Studio 2017</b>, things will become much more simpler, I have provided the precompiled libraries, include headers, and all external dependencies in [<b>here</b>](https://drive.google.com/file/d/11XBagdOkChDR2-2krSxKdTlhQcbmsMoI/view?usp=sharing) for download.
   
   > Otherwise, you may need to compile all external libraries using your own version of Visual Studio (newer versions might also work, but I have not tested them yet).

2. Using CMake build system (<b>cmake-gui</b>) to generate Visual Studio solutions. Press `Configure`, select "<b>x64</b>" platform, under the "<b>COMPILER</b>" option list, select "<b>MSVC</b>". Then, CMake will prompt you to <b>specify the path for compiled libraries and the location of include headers</b> (shown below).

   > Watch this self-explanatory [<b>video</b>](https://github.com/lchdl/sgl/blob/develop/assets/readme/how_to_compile_using_cmake_gui.mp4) for more infomation.

   <p align="center">
     <img src="https://github.com/lchdl/sgl/blob/develop/assets/readme/cmake_windows_compile.png">
   </p>

   > <b>DO NOT add extra "\\" after directories</b> to avoid strange include errors when building the project.
   
   Finally, press `Generate`, and `Open Project` to open Visual Studio. In Visual Studio, choose `Release` or `MinSizeRel` and compile SGL (if you want to debug SGL on Windows, select "Debug").

   > After compiling SGL, you may need to copy all \*.dll files and the contents of the `assets/` folder to the same location as the generated executables.

### For Linux-based systems (g++ & make)
1. Manually compile all external libraries. I have provided the precompiled libraries in [<b>here</b>](https://drive.google.com/file/d/1Z_MBPST6IFheGnUseI-6bwaOUG4MM3s3/view?usp=sharing) for download.

   > All libraries are <b>statically built in "<b>x64</b>" mode</b> using g++ <b>11.4.0</b> under Ubuntu 22.04.<br>
   > <b>NOTE</b>: If the precompiled libraries are not compatible with your development environment, you will need to compile them manually. Please also note that all libraries should be <i><b>statically linked</b></i> if you want to build SGL on Linux.

2. Make sure your g++ compiler supports <b>C++17</b> language standard. I recommend using <b>ccmake</b> to generate makefiles. Create a new empty directory and `cd` into it, then use the following command
   
   > **ccmake \<path_to_CMakeLists.txt\>**
   
   to initiate CMake build system and configure the project. ccmake will also prompt you to <b>input the path of those precompiled libraries and include headers</b>. Here are the configurations

   > Under the "<b>COMPILER</b>" option list, select "<b>GCC</b>".<br>
   > Under the "<b>BUILD_TYPE</b>" option list, select "<b>Release</b>" (if you want to debug SGL on Linux, select "Debug").<br>

   Then, ccmake will prompt you to provide the file paths of the precompiled libraries (\*.a) and headers (\*.h). After filling in all the paths, the final configuration should look like this:

   <p align="center">
     <img width=650 src="https://github.com/lchdl/sgl/blob/develop/assets/readme/cmake_linux_compile.png">
   </p>

   Hit `g` to generate makefile and ccmake will automatically quit if succeed.

   Finally, `make -jN` to compile SGL using `N` threads (such as `make -j8`) and wait for it to finish.
   
   > After compiling SGL, you may need to copy all the contents of the `assets/` folder to the same location as the generated executables.

## For developers

### Quick start
* After successfully compiling SGL in debug mode, you may want to start with the [<b>hello world</b>](https://github.com/lchdl/sgl/blob/develop/tests/test_hello_world.cpp) demo in `tests/test_hello_world.cpp`, which can help you quickly go through the entire software raterization pipeline.

### Before commit
* Code formatting: please make sure all source and header files are indented using <b>2 spaces</b> (not tabs).

### Project structure
* `include/`
  - Header files and inline implementations (*.h).
* `src/`
  - Source files (*.cpp).
* `tests/`
  - All CI test cases (*.cpp sources will be compiled and linked as executables).
* `doc/`
  - Documentations.
* `CMakeLists.txt`
  - For CMake.
* `assets/`
  - Contains assets (models, textures, etc.) for running demos.
* `ext/` <i><b>(not included in this repo)</b></i>
  - External dependencies, including:
    - `3rd_Party_Libraries/`: Zipped source code for all third-party libraries.
    - `Resources/`: All resources used in this project (tools, images, audio files, etc.).
* `build/` <i><b>(not included in this repo)</b></i>
  - Project files generated by CMAKE/CCMAKE under different platforms.
  - For example, `build/gcc_x64_static/` stores all the project files generated for 64-bit gcc/g++ compilers, using static linking. `build/vs2017/` stores all the project files generated for the Microsoft Visual Studio 2017.
* `build_deps/` <i><b>(not included in this repo)</b></i>
  - Contains dependencies required for building the project, including:
    - Header files for third-party libraries.
    - Compiled libraries (*.lib/*.a) and shared objects / DLLs (*.dll/*.so).
  - Typically, you need to compile all third-party libraries located in `ext/3rd_Party_Libraries` and copy the generated binaries and header files into this folder. You can organize binaries for different platforms into separate subfolders. For example:
    - `build_deps/gcc_x64_static/`: GCC 64-bit statically linked third-party libraries.
    - `build_deps/vs2017_x64/`: MSVC 2017 64-bit linked third-party libraries.
* `temp/` <i><b>(not included in this repo)</b></i>
  - Temporary files that are loosely organized but retained for potential future use.

### Features and TODOs
#### Features
* Flexible vertex format
* Fully customized vertex & fragment shader
* md5 (*.md5mesh, *.md5anim) format import & parsing
* Skeletal animation
* Wireframe rendering
* Perspective / orthographic projection
* Text rendering
#### TODOs (lots of things...)
* Shadow mapping
* Stencil buffer
* Alpha blending
* Ray tracing
* Texture baking
