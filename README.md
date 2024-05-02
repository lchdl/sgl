## Project structure
* CMakeLists.txt
  - For CMAKE.
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
* `ext/`
  - External dependencies, including the source code for all 3rd-party libraries.
* `build/`
  - Project files generated by CMAKE/CCMAKE under different platforms.
  - For example, `build/gcc_x64_static/` stores all the project files generated for 64-bit gcc/g++ compilers, using static linking. `build/vs2017/` stores all the project files generated for the Microsoft Visual Studio 2017.
* `build_deps/`
  - Dependencies for building the project. Which contains:
    - Include headers of 3rd-party libraries.
    - Compiled libraries (*.lib/*.a) and shared objects / DLLs (*.dll/*.so).
  - Usually all the files in this folder are collected from `ext/`. You need to compile all the 3rd-party libraries in `ext/` and then copy the generated binaries and include headers to this folder.
