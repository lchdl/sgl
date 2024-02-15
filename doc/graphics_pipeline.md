### Software Rasterizer Tutorial

* The OpenGL **render pipeline** overview:
  https://www.khronos.org/opengl/wiki/Rendering_Pipeline_Overview

* The **model transformation** matrix in OpenGL:
  https://learnopengl.com/Getting-started/Transformations

* The **view matrix** in OpenGL:
  http://www.songho.ca/opengl/gl_camera.html

* The **perspective matrix** in OpenGL:
  http://www.songho.ca/opengl/gl_projectionmatrix.html

$$
\left(\begin{array}{c} x_c \\ y_c \\ z_c \\ w_c \end{array}\right)
=\left(\begin{array}{cccc}
  \frac{2n}{r-l} & 0 & \frac{r+l}{r-l} & 0	\\
  0 & \frac{2n}{t-b} & \frac{t+b}{t-b} & 0	\\
  0 & 0 & -\frac{f+n}{f-n} & -\frac{2fn}{f-n}	\\
  0 & 0 & -1 & 0
\end{array}\right)
\left(\begin{array}{c}x_e \\ y_e \\ z_e \\ 1 \end{array}\right) ,
$$

* How to clip in homogeneous space?
  https://stackoverflow.com/questions/60910464/at-what-stage-is-clipping-performed-in-the-graphics-pipeline

* After clipping, perspective divide happens:

$$
\left(\begin{array}{c}x_n \\ y_n \\ z_n \end{array}\right)
=\frac{1}{w_c}\left(
\begin{array}{c}
  x_c \\ y_c \\ z_c 
\end{array}\right)
$$

$$
x_n, y_n, z_n \in [-1,1]
$$

* Simple and quick way to determine if a point is in a triangle, and all things you need to know about **barycentric interpolation**:
  https://fgiesen.wordpress.com/2013/02/06/the-barycentric-conspirac/
* Perspective correct **z-interpolation**:
  https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/visibility-problem-depth-buffer-depth-interpolation.html
* Perspective correct **vertex attributes interpolation**:
  https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/perspective-correct-interpolation-vertex-attributes.html
* The D3D10 rules of rasterization, that ensures each pixel of a triangle is processed once and **only once**: "Any pixel center which falls inside a triangle is drawn; a pixel is assumed to be inside if it passes the top-left rule. The top-left rule is that a pixel center is defined to lie inside of a triangle if it lies on the top edge or the left edge of a triangle."
  https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/




