/* Floating point image generator for Acidwarp */

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "handy.h"
#include "acidwarp.h"

/* ANGLE_UNIT corresponds to M_PI * 2, the full circle in radians.
 * To avoid visible seams, resulting colours need to go from 0 to 254,
 * with each getting an equal slice of the circle.
 */
#define ANGLE_UNIT                  (255.0)
#define ANGLE_UNIT_2                (ANGLE_UNIT*2)

/* TRIG_UNIT is full scale of trig functions. It was changed to
 * try to avoid visible seams where the palette wraps.
 */
#define TRIG_UNIT                   (ANGLE_UNIT*2)

static double lut_sin (double a)
{
    return TRIG_UNIT * sin(a * M_PI * 2 / ANGLE_UNIT);
}

static double lut_cos (double a)
{
    return TRIG_UNIT * cos(a * M_PI * 2 / ANGLE_UNIT);
}

static double lut_angle (double dx, double dy)
{
    double angle = atan2(dy, dx) * ANGLE_UNIT / (M_PI * 2);
    /* Always return a positive result */
    if (angle < 0.0) { angle += ANGLE_UNIT_2; }
    return angle;
}

static double lut_dist (double x, double y)
{
  return sqrt(x*x + y*y);
}
