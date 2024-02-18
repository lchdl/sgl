## SGL Engine Devlog

### Motivation
* Obsessed with retro game graphics such as PS1 style games (RE2/3).
* And I am wondering whether I can make such an engine to render graphics with similar style. 

### Developer environment setup
* Install **gcc/g++**;
* Install **libgl-dev** & **libxext-dev** for OpenGL headers;
* Install **cmake** & **ccmake**;
* Compile **SDL2** from source;
* Compile **Assimp** from source;
* IDE: **Visual Studio Code** (just for now);

### Rasterization pipeline
* Motivation: implement a software rasterizer for learning purpose.
* An overview of OpenGL rasterization pipeline.
  * World, view, and projection matrices;
  * *Vertex* and *fragment* shaders;
  * *Vertex attribute interpolation*.
* Simple **vector math** library implementation.
  * Vector & matrix operations.
* Key definition: **Vertex**, **Vertex_gl**, **Fragment_gl**

### TODO
* Assimp model loading.
* Simple UI system.

   

