/* IMG_FLOAT.H */

#ifndef IMG_FLOAT_H
#define IMG_FLOAT_H

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

double lut_sin(double a);
double lut_cos(double a);
double lut_angle(double dx, double dy);
double lut_dist(double x, double y);

#endif /* IMG_FLOAT_H */
