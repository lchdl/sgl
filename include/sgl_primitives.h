#pragma once

/**
Defines common 2D and 3D primitives for drawing operations.
2D shapes are rendered directly onto the target texture, while 
3D primitives return a generated mesh for further use.
**/

#include "sgl_math.h"
#include "sgl_texture.h"

namespace sgl {

/* draw primitives onto texture directly */
void draw_pixel(sgl::Texture* target, int x, int y, const Vec4& color);
void draw_line(sgl::Texture* target, int x1, int y1, int x2, int y2, const Vec4& color);
void draw_circle(sgl::Texture* target, int x, int y, int r, const Vec4& color);
void draw_ellipse(sgl::Texture* target, double x, double y, double rx, double ry, double rotation, const Vec4& color, int nsegs);
void draw_rectangle(sgl::Texture* target, int x, int y, int w, int h, const Vec4& color);
void draw_rectangle(sgl::Texture* target, double cx, double cy, double w, double h, double rotation, const Vec4& color);
void draw_bezier(sgl::Texture* target, const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color, int nsegs);
void draw_bezier2(sgl::Texture* target, const Vec2& p1, const Vec2& p1_tangent, const Vec2& p2, const Vec2& p2_tangent, const Vec4& color, int nsegs);

};
