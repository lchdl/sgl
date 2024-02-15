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
\left(\begin{array}{c} x_{\rm{clip}} \\ y_{\rm{clip}} \\ z_{\rm{clip}} \\ w_{\rm{clip}} \end{array}\right)
=\left(\begin{array}{cccc}
  \frac{2n}{r-l} & 0 & \frac{r+l}{r-l} & 0	\\
  0 & \frac{2n}{t-b} & \frac{t+b}{t-b} & 0	\\
  0 & 0 & -\frac{f+n}{f-n} & -\frac{2fn}{f-n}	\\
  0 & 0 & -1 & 0
\end{array}\right)
\left(\begin{array}{c}x_{\rm{view}} \\ y_{\rm{view}} \\ z_{\rm{view}} \\ 1 \end{array}\right) ,
$$

* How to clip in homogeneous space?
  https://stackoverflow.com/questions/60910464/at-what-stage-is-clipping-performed-in-the-graphics-pipeline

* After clipping, perspective divide happens:

$$
\left(\begin{array}{c}x_{\rm{ndc}} \\ y_{\rm{ndc}} \\ z_{\rm{ndc}} \end{array}\right)=\frac{1}{w_{\rm{clip}}}\left( \begin{array}{c} x_{\rm{clip}} \\ y_{\rm{clip}} \\ z_{\rm{clip}} \end{array}\right)
$$

$$
x_{\rm{ndc}}, y_{\rm{ndc}}, z_{\rm{ndc}} \in [-1,1]
$$

* Simple and quick way to determine if a point is in a triangle, and all things you need to know about **barycentric interpolation**:
  https://fgiesen.wordpress.com/2013/02/06/the-barycentric-conspirac/
* Perspective correct **z-interpolation**:
  https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/visibility-problem-depth-buffer-depth-interpolation.html
* Perspective correct **vertex attributes interpolation**:
  https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/perspective-correct-interpolation-vertex-attributes.html
* Fragment shader defined outputs:
  https://www.khronos.org/opengl/wiki/Fragment_Shader

