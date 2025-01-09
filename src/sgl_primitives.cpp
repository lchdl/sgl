#include "sgl_primitives.h"

namespace sgl {

void draw_pixel(sgl::Texture * target, int x, int y, const Vec4 & color)
{
  if (target == NULL || target->get_width() <= 0 || target->get_height() <= 0 || 
    target->get_bytes_per_pixel() != 4) return; /* only supports 32 bit texture */
  if (x < 0 || x >= target->get_width() || y < 0 || y >= target->get_height())
    return;
  uint32_t* pixels = (uint32_t*)target->get_pixel_data();
  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);
  pixels[y * target->get_width() + x] = packed_color;
}

void draw_line(sgl::Texture * target, int x1, int y1, int x2, int y2, const Vec4 & color)
{
  if (target == NULL || target->get_width() <= 0 || target->get_height() <= 0 ||
    target->get_bytes_per_pixel() != 4) return; /* only supports 32 bit texture */
  
  uint32_t* pixels = (uint32_t*)target->get_pixel_data();
  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);

  int dx, dy;
  int x, y;
  int epsilon = 0;
  int w = target->get_width(), h = target->get_height();
  int Dx = x2 - x1;
  int Dy = y1 - y2;
  Dx > 0 ? dx = +1 : dx = -1;
  Dy > 0 ? dy = -1 : dy = +1;
  Dx = ::abs(Dx), Dy = ::abs(Dy);
  if (Dx > Dy) {
    y = y1;
    for (x = x1; x != x2 && x < w; x += dx) {
      /* draw (x, y) here */
      if (x >= 0 && y >= 0 && y < h)
        pixels[y * w + x] = packed_color;
      /* prepare for next iteration */
      epsilon += Dy;
      if ((epsilon << 1) > Dx) {
        y += dy;
        epsilon -= Dx;
      }
    }
  }
  else {
    x = x1;
    for (y = y1; y != y2 && y < h; y += dy) {
      /* draw (x, y) here */
      if (x >= 0 && x < w && y >= 0)
        pixels[y * w + x] = packed_color;
      /* prepare for next iteration */
      epsilon += Dx;
      if ((epsilon << 1) > Dy) {
        epsilon -= Dy;
        x += dx;
      }
    }
  }
}

void draw_circle(sgl::Texture * target, int x, int y, int r, const Vec4 & color)
{
  if (target == NULL || target->get_width() <= 0 || target->get_height() <= 0 ||
    target->get_bytes_per_pixel() != 4) return; /* only supports 32 bit texture */

  uint32_t* pixels = (uint32_t*)target->get_pixel_data();
  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);
  
  /* define a simple auxiliary function for writing pixels to texture */
  int w = target->get_width(), h = target->get_height();
  auto write = [&](int x0, int y0){
    if (x0 >= 0 && x0 < w && y0 >= 0 && y0 < h)
      pixels[y0 * w + x0] = packed_color;
  };

  /*
  an implementation of the midpoint circle drawing algorithm, see
    * https://www.youtube.com/watch?v=hpiILbMkF9w, and
    * https://www.geeksforgeeks.org/mid-point-circle-drawing-algorithm/
  for more info.
  */

  int dx = r, dy = 0;
  /* the initial point on the axes after translation */
  write(x + dx, y + dy);
  /* When radius is zero only a single point will be printed */
  if (r > 0) {
    write(x - dx, y + dy);
    write(x + dy, y - dx);
    write(x + dy, y + dx);
  }

  /* Initialising the value of P */
  int P = 1 - r;
  while (dx > dy)
  {
    dy++;
    /* Mid-point is inside or on the perimeter */
    if (P <= 0) 
      P = P + 2 * dy + 1;
    /* Mid-point is outside the perimeter */
    else {
      dx--;
      P = P + 2 * dy - 2 * dx + 1;
    }
    /* All the perimeter points have already been printed */
    if (dx < dy)
      break;
    /* 
    Printing the generated point and its reflection
    in the other octants after translation
    */
    write(x + dx, y + dy);
    write(x - dx, y + dy);
    write(x + dx, y - dy);
    write(x - dx, y - dy);
    /*
    If the generated point is on the line x = y then 
    the perimeter points have already been printed
    */
    if (dx != dy) {
      write(x + dy, y + dx);
      write(x - dy, y + dx);
      write(x + dy, y - dx);
      write(x - dy, y - dx);
    }
  }
}

void draw_ellipse(sgl::Texture* target, double x, double y, double rx, double ry, double rotation, const Vec4& color, int nsegs)
{
  if (target == NULL || target->get_width() <= 0 || target->get_height() <= 0 ||
    target->get_bytes_per_pixel() != 4) return; /* only supports 32 bit texture */
  if (nsegs < 1) return;

  uint32_t* pixels = (uint32_t*)target->get_pixel_data();
  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);

  /* auxiliary function for drawing line from (x1, y1) to (x2, y2) */
  int w = target->get_width(), h = target->get_height();
  auto bresenham = [&](int x1, int y1, int x2, int y2) {
    int dx, dy;
    int x, y;
    int epsilon = 0;
    int Dx = x2 - x1;
    int Dy = y1 - y2;
    Dx > 0 ? dx = +1 : dx = -1;
    Dy > 0 ? dy = -1 : dy = +1;
    Dx = ::abs(Dx), Dy = ::abs(Dy);
    if (Dx > Dy) {
      y = y1;
      for (x = x1; x != x2 && x < w; x += dx) {
        /* draw (x, y) here */
        if (x >= 0 && y >= 0 && y < h)
          pixels[y * w + x] = packed_color;
        /* prepare for next iteration */
        epsilon += Dy;
        if ((epsilon << 1) > Dx) {
          y += dy;
          epsilon -= Dx;
        }
      }
    }
    else {
      x = x1;
      for (y = y1; y != y2 && y < h; y += dy) {
        /* draw (x, y) here */
        if (x >= 0 && x < w && y >= 0)
          pixels[y * w + x] = packed_color;
        /* prepare for next iteration */
        epsilon += Dx;
        if ((epsilon << 1) > Dy) {
          epsilon -= Dy;
          x += dx;
        }
      }
    }
  };

  double yx_ratio = (double)ry / (double)rx;
  double angle = 0.0;
  double step = 2 * sgl::PI / (double)nsegs;

  Vec2 start = Vec2(x, y) + sgl::rotate(Vec2(0, -ry), rotation);
  Vec2 end;
  Vec2 scale = Vec2(1.0, yx_ratio);
  for (int i = 0; i < nsegs; i++) {
    Vec2 dxy = sgl::rotate(scale * sgl::rotate(Vec2(0, -rx), (i + 1) * step), rotation);
    if (i < nsegs - 1)
      end = Vec2(x, y) + dxy;
    else
      end = Vec2(x, y) + sgl::rotate(Vec2(0, -ry), rotation);
    bresenham((int)start.x, (int)start.y, (int)end.x, (int)end.y);
    start = end;
  }
}

void draw_rectangle(sgl::Texture * target, int x, int y, int w, int h, const Vec4 & color)
{
  if (target == NULL || target->get_width() <= 0 || target->get_height() <= 0 ||
    target->get_bytes_per_pixel() != 4) return; /* only supports 32 bit texture */
  if (w < 1 || h < 1) return;

  uint32_t* pixels = (uint32_t*)target->get_pixel_data();
  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);

  /* define a simple auxiliary function for writing pixels to texture */
  int tw = target->get_width(), th = target->get_height();
  auto write = [&](int x0, int y0) {
    if (x0 >= 0 && x0 < tw && y0 >= 0 && y0 < th)
      pixels[y0 * tw + x0] = packed_color;
  };

  for (int curx = x; curx < x + w; curx++) {
    write(curx, y);
    write(curx, y + h - 1);
  }
  for (int cury = y; cury < y + h; cury++) {
    write(x, cury);
    write(x + w - 1, cury);
  }
}

void draw_rectangle(sgl::Texture * target, double cx, double cy, double w, double h, double rotation, const Vec4 & color)
{
  if (target == NULL || target->get_width() <= 0 || target->get_height() <= 0 ||
    target->get_bytes_per_pixel() != 4) return; /* only supports 32 bit texture */
  if (w < 1 || h < 1) return;

  uint32_t* pixels = (uint32_t*)target->get_pixel_data();
  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);


  /* define four corners */
  double dx = w / 2.0, dy = h / 2.0;
  Vec2 p[4];
  Vec2 o = Vec2(cx, cy);
  p[0] = o + sgl::rotate(Vec2(-dx, -dy), rotation);
  p[1] = o + sgl::rotate(Vec2(+dx, -dy), rotation);
  p[2] = o + sgl::rotate(Vec2(+dx, +dy), rotation);
  p[3] = o + sgl::rotate(Vec2(-dx, +dy), rotation);

  /* auxiliary function for drawing line from (x1, y1) to (x2, y2) */
  int tw = target->get_width(), th = target->get_height();
  auto bresenham = [&](int x1, int y1, int x2, int y2) {
    int dx, dy;
    int x, y;
    int epsilon = 0;
    int Dx = x2 - x1;
    int Dy = y1 - y2;
    Dx > 0 ? dx = +1 : dx = -1;
    Dy > 0 ? dy = -1 : dy = +1;
    Dx = ::abs(Dx), Dy = ::abs(Dy);
    if (Dx > Dy) {
      y = y1;
      for (x = x1; x != x2 && x < tw; x += dx) {
        /* draw (x, y) here */
        if (x >= 0 && y >= 0 && y < th)
          pixels[y * tw + x] = packed_color;
        /* prepare for next iteration */
        epsilon += Dy;
        if ((epsilon << 1) > Dx) {
          y += dy;
          epsilon -= Dx;
        }
      }
    }
    else {
      x = x1;
      for (y = y1; y != y2 && y < th; y += dy) {
        /* draw (x, y) here */
        if (x >= 0 && x < tw && y >= 0)
          pixels[y * tw + x] = packed_color;
        /* prepare for next iteration */
        epsilon += Dx;
        if ((epsilon << 1) > Dy) {
          epsilon -= Dy;
          x += dx;
        }
      }
    }
  };

  bresenham((int)p[0].x, (int)p[0].y, (int)p[1].x, (int)p[1].y);
  bresenham((int)p[1].x, (int)p[1].y, (int)p[2].x, (int)p[2].y);
  bresenham((int)p[2].x, (int)p[2].y, (int)p[3].x, (int)p[3].y);
  bresenham((int)p[3].x, (int)p[3].y, (int)p[0].x, (int)p[0].y);
}

void draw_bezier(sgl::Texture * target, const Vec2 & p0, const Vec2 & p1, const Vec2 & p2, const Vec2 & p3, const Vec4 & color, int nsegs)
{
  /*
  See
  https://stackoverflow.com/questions/785097/how-do-i-implement-a-b%C3%A9zier-curve-in-c
  for more infomation about drawing B¨¦zier curves in C.
  */
  if (target == NULL || target->get_width() <= 0 || target->get_height() <= 0 ||
    target->get_bytes_per_pixel() != 4) return; /* only supports 32 bit texture */

  uint32_t* pixels = (uint32_t*)target->get_pixel_data();
  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);

  double t_step = 1.0 / nsegs;
  Vec2 Q_last, Q;
  
  auto interpolate = [](Vec2 a, Vec2 b, double t) { return a * (1.0 - t) + b * t; };
  auto line_to_Q = [&](){
    /* Draw a line from Q_last to Q, both positions of Q_last and Q are maintained 
    properly from the main for loop. */
    int x1 = (int)Q_last.x, x2 = (int)Q.x;
    int y1 = (int)Q_last.y, y2 = (int)Q.y;
    int tw = target->get_width(), th = target->get_height();
    int dx, dy;
    int x, y;
    int epsilon = 0;
    int Dx = x2 - x1;
    int Dy = y1 - y2;
    Dx > 0 ? dx = +1 : dx = -1;
    Dy > 0 ? dy = -1 : dy = +1;
    Dx = ::abs(Dx), Dy = ::abs(Dy);
    if (Dx > Dy) {
      y = y1;
      for (x = x1; x != x2 && x < tw; x += dx) {
        /* draw (x, y) here */
        if (x >= 0 && y >= 0 && y < th)
          pixels[y * tw + x] = packed_color;
        /* prepare for next iteration */
        epsilon += Dy;
        if ((epsilon << 1) > Dx) {
          y += dy;
          epsilon -= Dx;
        }
      }
    }
    else {
      x = x1;
      for (y = y1; y != y2 && y < th; y += dy) {
        /* draw (x, y) here */
        if (x >= 0 && x < tw && y >= 0)
          pixels[y * tw + x] = packed_color;
        /* prepare for next iteration */
        epsilon += Dx;
        if ((epsilon << 1) > Dy) {
          epsilon -= Dy;
          x += dx;
        }
      }
    }
  };

  for (int i = 0; i <= nsegs; i++) {
    double t = i * t_step;
    Vec2 P01 = interpolate(p0, p1, t);
    Vec2 P12 = interpolate(p1, p2, t);
    Vec2 P23 = interpolate(p2, p3, t);
    Vec2 P0112 = interpolate(P01, P12, t);
    Vec2 P1223 = interpolate(P12, P23, t);
    Q = interpolate(P0112, P1223, t);
    if (i > 0)
      line_to_Q();
    Q_last = Q;
  }
}

void draw_bezier2(sgl::Texture * target, const Vec2 & p1, const Vec2 & p1_tangent, const Vec2 & p2, const Vec2 & p2_tangent, const Vec4 & color, int nsegs)
{
  Vec2 P0 = p1;
  Vec2 P1 = p1 + p1_tangent;
  Vec2 P2 = p2;
  Vec2 P3 = p2 + p2_tangent;
  draw_bezier(target, P0, P1, P2, P3, color, nsegs);
}


};


