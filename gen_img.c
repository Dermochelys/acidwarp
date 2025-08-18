#pragma ide diagnostic ignored "cert-msc50-cpp"
#include <math.h>
#include "handy.h"
#include "img_float.c"

/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 * Ported to Linux by Steven Wills
 * Ported to SDL by Boris Gjenero
 * Ported to Android by Matthew Zavislak
 */

void generate_image_float(int imageFuncNum,
                          UCHAR *buf_graf,
                          UINT _xcenter,
                          UINT _ycenter,
                          UINT _width,
                          UINT _height,
                          UINT colors,
                          UINT pitch,
                          UINT normalize)
{

  int x1,x2,x3,x4,y1,y2,y3,y4;

  int _x, _y;
  ULONG _color;
  double x, y;
  double dx, dy;
  double dist, angle;
  double color;
  double x_center, y_center, width, height;

  // Original DOS version targeted 8:5 display (320x200) but also supported 4:3 displays.
  // It appears during the SDL port that added floating point conversion, it assumed a 8:5 display ratio.
  // Apply the necessary correction here:
  const double aspect_correction = (8.0 / 5.0) / ((double)_width / (double)_height);

  if (normalize) {
    x_center = (_xcenter * 320.0) / (double)_width;
    y_center = (_ycenter * 200.0 * aspect_correction) / (double)_height;
    width = 320.0;
    height = 200.0 * aspect_correction;
  } else {
    x_center = _xcenter;
    y_center = _ycenter;
    width = _width;
    height = _height;
  }

  /* Some general purpose random angles and offsets.
   * Not all functions use them.
   */

  x1 = RANDOM(40)-20;  x2 = RANDOM(40)-20;
  x3 = RANDOM(40)-20;  x4 = RANDOM(40)-20;
  y1 = RANDOM(40)-20;  y2 = RANDOM(40)-20;
  y3 = RANDOM(40)-20;  y4 = RANDOM(40)-20;

  for (_y = 0; _y < _height && !abort_draw; ++_y)
  {
    if (normalize) {
      y = (double)(_y * 200 * aspect_correction) / _height;
    } else {
      y = _y;
    }

    for (_x = 0; _x < _width; ++_x)
    {
      if (normalize) {
        x = (double)(_x * 320) / _width;
      } else {
        x = _x;
      }
      dx = x - x_center;
      /* dy may be altered below, so calculate here */
      dy = y - y_center;

      dist  = lut_dist (dx, dy);
      angle = lut_angle (dx, dy);

      /* select function. Could be made a separate function, but since
         this function is evaluated over a large iteration of values I am
         afraid that it might slow things down even more to have a
         separate function.        */
      switch (imageFuncNum)
      {
        case 0: /* Rays plus 2D Waves */
          color = angle + lut_sin (dist * 10) / 64 +
                  lut_cos (x * ANGLE_UNIT / width * 2) / 32 +
                  lut_cos (y * ANGLE_UNIT / height * 2) / 32;
          break;

        case 1:        /* Rays plus 2D Waves */
          color = angle + lut_sin (dist * 10) / 16 +
                  lut_cos (x * ANGLE_UNIT / width * 2) / 8 +
                  lut_cos (y * ANGLE_UNIT / height * 2) / 8;
          break;

        case 2:
          color = lut_sin (lut_dist(dx + x1, dy + y1) *  4) / 32 +
                  lut_sin (lut_dist(dx + x2, dy + y2) *  8) / 32 +
                  lut_sin (lut_dist(dx + x3, dy + y3) * 16) / 32 +
                  lut_sin (lut_dist(dx + x4, dy + y4) * 32) / 32;
          break;

        case 3:        /* Peacock */
          color = angle + lut_sin (lut_dist(dx + 20, dy) * 10) / 32 +
                  angle + lut_sin (lut_dist(dx - 20, dy) * 10) / 32;
          break;

        case 4:
          color = lut_sin (dist) / 16;
          break;

        case 5:        /* 2D Wave + Spiral */
          color = lut_cos (x * ANGLE_UNIT / width) / 8 +
                  lut_cos (y * ANGLE_UNIT / height) / 8 +
                  angle + lut_sin(dist) / 32;
          break;

        case 6:        /* Peacock, three centers */
          color = lut_sin (lut_dist(dx,      dy - 20) * 4) / 32+
                  lut_sin (lut_dist(dx + 20, dy + 20) * 4) / 32+
                  lut_sin (lut_dist(dx - 20, dy + 20) * 4) / 32;
          break;

        case 7:        /* Peacock, three centers */
          color = angle +
                  lut_sin (lut_dist(dx,      dy - 20) * 8) / 32+
                  lut_sin (lut_dist(dx + 20, dy + 20) * 8) / 32+
                  lut_sin (lut_dist(dx - 20, dy + 20) * 8) / 32;
          break;

        case 8:        /* Peacock, three centers */
          color = lut_sin (lut_dist(dx,      dy - 20) * 12) / 32+
                  lut_sin (lut_dist(dx + 20, dy + 20) * 12) / 32+
                  lut_sin (lut_dist(dx - 20, dy + 20) * 12) / 32;
          break;

        case 9:        /* Five Arm Star */
          color = dist + lut_sin (5 * angle) / 64;
          break;

        case 10:        /* 2D Wave */
          color = lut_cos (x * ANGLE_UNIT / width * 2) / 4 +
                  lut_cos (y * ANGLE_UNIT / height * 2) / 4;
          break;

        case 11:        /* 2D Wave */
          color = lut_cos (x * ANGLE_UNIT / width) / 8 +
                  lut_cos (y * ANGLE_UNIT / height) / 8;
          break;

        case 12:        /* Simple Concentric Rings */
          color = dist;
          break;

          /* Good for testing proper wrapping of angle. This was
           * flawed in original Acidwarp 4.10, resulting in a
           * double-width stripe going right from the centre.
           */
        case 13:        /* Simple Rays */
          color = angle;
          break;

          /* Good for testing proper wrapping of negative color.
           * Errors will show as a dashed seam going right from centre.
           */
        case 14:        /* Toothed Spiral Sharp */
          color = angle + lut_sin(dist * 8)/32;
          break;

        case 15:        /* Rings with sine */
          color = lut_sin(dist * 4)/32;
          break;

        case 16:        /* Rings with sine with sliding inner Rings */
          color = dist+ lut_sin(dist * 4) / 32;
          break;

        case 17:
          color = lut_sin(lut_cos(2 * x * ANGLE_UNIT / width)) / (20 + dist)
                  + lut_sin(lut_cos(2 * y * ANGLE_UNIT / height)) / (20 + dist);
          break;

        case 18:        /* 2D Wave */
          color = lut_cos(7 * x * ANGLE_UNIT / width)/(20 + dist) +
                  lut_cos(7 * y * ANGLE_UNIT / height)/(20 + dist);
          break;

        case 19:        /* 2D Wave */
          color = lut_cos(17 * x * ANGLE_UNIT/width)/(20 + dist) +
                  lut_cos(17 * y * ANGLE_UNIT/height)/(20 + dist);
          break;

        case 20:        /* 2D Wave Interference */
          color = lut_cos(17 * x * ANGLE_UNIT / width) / 32 +
                  lut_cos(17 * y * ANGLE_UNIT / height) / 32 + dist + angle;
          break;

        case 21:        /* 2D Wave Interference */
          color = lut_cos(7 * x * ANGLE_UNIT / width) / 32 +
                  lut_cos(7 * y * ANGLE_UNIT / height) / 32 + dist;
          break;

        case 22:        /* 2D Wave Interference */
          color = lut_cos( 7 * x * ANGLE_UNIT / width) / 32 +
                  lut_cos( 7 * y * ANGLE_UNIT / height) / 32 +
                  lut_cos(11 * x * ANGLE_UNIT / width) / 32 +
                  lut_cos(11 * y * ANGLE_UNIT / height) / 32;
          break;

        case 23:
          color = lut_sin (angle * 7) / 32;
          break;

        case 24:
          color = lut_sin (lut_dist(dx + x1, dy + y1) * 2) / 12 +
                  lut_sin (lut_dist(dx + x2, dy + y2) * 4) / 12 +
                  lut_sin (lut_dist(dx + x3, dy + y3) * 6) / 12 +
                  lut_sin (lut_dist(dx + x4, dy + y4) * 8) / 12;
          break;

        case 25:
          color = angle + lut_sin (lut_dist(dx + x1, dy + y1) * 2) / 16 +
                  angle + lut_sin (lut_dist(dx + x2, dy + y2) * 4) / 16 +
                  lut_sin (lut_dist(dx + x3, dy + y3) * 6) /  8 +
                  lut_sin (lut_dist(dx + x4, dy + y4) * 8) /  8;
          break;

        case 26:
          color = angle + lut_sin (lut_dist(dx + x1, dy + y1) * 2) / 12 +
                  angle + lut_sin (lut_dist(dx + x2, dy + y2) * 4) / 12 +
                  angle + lut_sin (lut_dist(dx + x3, dy + y3) * 6) / 12 +
                  angle + lut_sin (lut_dist(dx + x4, dy + y4) * 8) / 12;
          break;

        case 27:
          color = lut_sin (lut_dist(dx + x1, dy + y1) * 2) / 32 +
                  lut_sin (lut_dist(dx + x2, dy + y2) * 4) / 32 +
                  lut_sin (lut_dist(dx + x3, dy + y3) * 6) / 32 +
                  lut_sin (lut_dist(dx + x4, dy + y4) * 8) / 32;
          break;

        case 28:        /* Random Curtain of Rain (in strong wind) */
          if (y == 0 || x == 0)
            color = RANDOMD(16);
          else
            color = (  *(buf_graf + (pitch *  _y   ) + (_x-1))
                       + *(buf_graf + (pitch * (_y-1)) +    _x)) / 2.0
                    + RANDOMD(16) - 8;
          break;

        case 29:
          if (y == 0 || x == 0)
            color = RANDOMD(1024);
          else
            color = dist/6 + (*(buf_graf + (pitch * _y    ) + (_x-1))
                              +  *(buf_graf + (pitch * (_y-1)) +    _x)) / 2.0
                    + RANDOMD(16) - 8;
          break;

        case 30:
          color = (((int)lut_sin (lut_dist(dx,     dy - 20) * 4) / 32 ^
                    (int)lut_sin (lut_dist(dx + 20,dy + 20) * 4) / 32) ^
                   (int)lut_sin (lut_dist(dx - 20,dy + 20) * 4) / 32);
          break;

        case 31:
          color = ((int)fmod(angle, (ANGLE_UNIT/4)) ^ (int)dist);
          break;

        case 32:        /* Plaid (Useful for aspect ratio verification) */
          color = ((int)dy ^ (int)dx);
          break;

        case 33:        /* Variation on Rain */
          if (y == 0 || x == 0)
            color = RANDOMD(16);
          else
            color = (  *(buf_graf + (pitch *  _y   ) + (_x-1))
                       + *(buf_graf + (pitch * (_y-1)) +  _x   )  ) / 2.0;

          color += RANDOMD(2) - 1;

          if (color < 64)
            color += RANDOMD(16) - 8;
          break;

        case 34:        /* Variation on Rain */
          if (y == 0 || x == 0)
            color = RANDOMD(16);
          else
            color = (  *(buf_graf + (pitch *  _y   ) + (_x-1))
                       + *(buf_graf + (pitch * (_y-1)) +  _x   )  ) / 2.0;

          if (color < 100)
            color += RANDOMD(16) - 8;
          break;

        case 35:
          color = angle + lut_sin(dist * 8)/32;
          dx = x - x_center;
          dy = (y - y_center) * 2;
          dist  = lut_dist (dx, dy);
          angle = lut_angle (dx, dy);
          color = (color + angle + lut_sin(dist * 8)/32) / 2;
          break;

        case 36:
          color = angle + lut_sin (dist * 10) / 16 +
                  lut_cos (x * ANGLE_UNIT / width * 2) / 8 +
                  lut_cos (y * ANGLE_UNIT / height * 2) / 8;
          dx = x - x_center;
          dy = (y - y_center) * 2;
          dist  = lut_dist (dx, dy);
          angle = lut_angle (dx, dy);
          color = (color + angle + lut_sin(dist * 8)/32) / 2;
          break;

        case 37:
          color = angle + lut_sin (dist * 10) / 16 +
                  lut_cos (x * ANGLE_UNIT / width * 2) / 8 +
                  lut_cos (y * ANGLE_UNIT / height * 2) / 8;
          dx = x - x_center;
          dy = (y - y_center) * 2;
          dist  = lut_dist (dx, dy);
          angle = lut_angle (dx, dy);
          color = (color + angle + lut_sin (dist * 10) / 16 +
                   lut_cos (x * ANGLE_UNIT / width * 2) / 8 +
                   lut_cos (y * ANGLE_UNIT / height * 2) / 8)  /  2;
          break;

        case 38:
          /* Intent is to interlace two different screens */
          if (_y % 2)
          {
            dy *= 2;
            dist  = lut_dist (dx, dy);
            angle = lut_angle (dx, dy);
          }
          color = angle + lut_sin(dist * 8)/32;
          break;

        case 39:
          color = ((int)fmod(angle, (ANGLE_UNIT/4)) ^ (int)dist);
          dx = x - x_center;
          dy = (y - y_center) * 2;
          dist = lut_dist (dx, dy);
          angle = lut_angle (dx, dy);
          color = (color +  (((int)fmod(angle, (ANGLE_UNIT/4)) ^ (int)dist))) / 2;
          break;

        case 40:
          color = ((int)dy ^ (int)dx);
          dx = x - x_center;
          dy = (y - y_center) * 2;
          color = (color +  ((int)dy ^ (int)dx)) / 2;
          break;

        case 41:        /* 12-fold Mandala Symmetry #claude */
          {
            double sym_angle = fmod(angle * 6, ANGLE_UNIT/2);
            if (sym_angle > ANGLE_UNIT/4) sym_angle = ANGLE_UNIT/2 - sym_angle;
            color = sym_angle * 4 + lut_sin(dist * 8) / 32 +
                    lut_cos(dist * 3) / 64;
          }
          break;

        default:
          color = RANDOMD(colors - 1) + 1;
          break;
      }

      /* Fit color value into the palette range using modulo.  It seems
         that the Turbo-C MOD function does not behave the way I expect.
         It gives negative values for the MOD of a negative number.
         I expect MOD to function as it does on my HP-28S.
       */


      /* Final colors need to go from 1 to colors, because color 0 is
       * not used. Floating point conversion to the palette is by
       * truncation, not rounding, as if each whole number is a bin
       * (0 for 0 to 1, 1 for 1 to 2 and so on). 1 is added later,
       * so right here this requires: 0 <= color < colors
       */
      _color = (long)color % (colors-1);
      /* The -1.0 < color < 0 bin is mapped to the last bin,
       * meaning right before the 0 <= color < 1 bin in rotation.
       */
      if (color < 0)
        _color += (colors - 2);

      ++_color;
      /* color 0 is never used, so all colors are from 1 through 255 */

      *(buf_graf + (pitch * _y) + _x) = (UCHAR)_color;
      /* Store the color in the buffer */
    }
    /* end for (y = 0; y < height; ++y)        */
  }
  /* end for (x = 0; x < width; ++x)        */

}
