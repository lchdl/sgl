Some tutorials that I found useful on the internet.

### C++ Source Control & Build System

* Simplified **CMake** beginner tutorial:
  https://www.youtube.com/watch?v=mKZ-i-UfGgQ
* General C++ **project structure**: 
  https://www.linkedin.com/pulse/what-general-c-project-structure-like-herbert-elwood-gilliland-iii
* All kinds of **.gitignore** files can be found here:
  https://github.com/github/gitignore
* How to link precompiled **static library** using CMake:
  https://stackoverflow.com/questions/14077611/how-do-i-tell-cmake-to-link-in-a-static-library-in-the-source-directory

### SDL3 (Simple DirectMedia Layer 3)
* Project main page:
  https://wiki.libsdl.org/SDL3/FrontPage
* **Tutorial** by LazyFoo:
  https://lazyfoo.net/tutorials/SDL/
* Initialize OpenGL in SDL:
  https://raw.githubusercontent.com/Overv/Open.GL/master/ebook/Modern%20OpenGL%20Guide.pdf

### Assimp

#### Build Assimp using MSYS2+MinGW64 on Windows
* Download source code from: 
  https://github.com/assimp/assimp/releases/
  
  and unzip the code into an empty folder, enter that folder and create a folder named "**./build/**" in it.
* Use CMake to configure & generate a **MinGW Makefile** under the "**./build/**" directory.
  
  - note that you need to uncheck build zlib and 

* On MSYS2, install mingw32-make:
  > pacman -S mingw-w64-x86_64-make
* Search "**MSYS2 MINGW64**" on windows search bar, to open a MinGW64 MSYS2 shell, *do not open other types of shells*. You can execute
  > mingw32-make
  
  to see if anything goes wrong.
* **cd** into "**./build/**", then
  > mingw32-make

