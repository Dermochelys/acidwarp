/* Floating point image generator for Acidwarp */

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#include "handy.h"
#include "acidwarp.h"
#include "img_float.h"

double lut_sin (double a)
{
  return TRIG_UNIT * sin(a * M_PI * 2 / ANGLE_UNIT);
}

double lut_cos (double a)
{
  return TRIG_UNIT * cos(a * M_PI * 2 / ANGLE_UNIT);
}

double lut_angle (double dx, double dy)
{
  double angle = atan2(dy, dx) * ANGLE_UNIT / (M_PI * 2);
  /* Always return a positive result */
  if (angle < 0.0) { angle += ANGLE_UNIT_2; }
  return angle;
}

double lut_dist (double x, double y)
{
  return sqrt(x*x + y*y);
}