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

### Software Rasterizer Tutorial
* The **perspective matrix** in OpenGL:
  http://www.songho.ca/opengl/gl_projectionmatrix.html

$$
\left(
\begin{array}{c}
  x_c \\ y_c \\ z_c \\ w_c 
\end{array}
\right)=
\left(
\begin{array}{cccc}
  \frac{2n}{r-l} & 0 & \frac{r+l}{r-l} & 0	\\
  0 & \frac{2n}{t-b} & \frac{t+b}{t-b} & 0	\\
  0 & 0 & -\frac{f+n}{f-n} & -\frac{2fn}{f-n}	\\
  0 & 0 & -1 & 0
\end{array}
\right)
\left(
\begin{array}{c}
  x_e \\ y_e \\ z_e \\ 1 
\end{array}
\right) ,
$$

$$
\left(
\begin{array}{c}
  x_n \\ y_n \\ z_n 
\end{array}
\right)=
\frac{1}{w_c}
\left(
\begin{array}{c}
  x_c \\ y_c \\ z_c 
\end{array}
\right) = 
-\frac{1}{z_e}
\left(
\begin{array}{c}
  x_c \\ y_c \\ z_c 
\end{array}
\right)
,
w_n=w_c=-z_e
$$

$$
x_n, y_n, z_n \in [-1,1]
$$

* Perspective correct z-interpolation
  https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/visibility-problem-depth-buffer-depth-interpolation.html
* Simple and quick way to determine if a point is in a triangle, and all things you need to know about **barycentric interpolation**
  https://fgiesen.wordpress.com/2013/02/06/the-barycentric-conspirac/
* Also look at this tutorial on how to write a high performance software rasterizer
  https://fgiesen.wordpress.com/2013/02/17/optimizing-sw-occlusion-culling-index/
