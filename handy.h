#ifndef __HANDY
#define __HANDY 1

/* Some basic data types */
/* typedef unsigned char   CHAR; Is this a typo */
typedef unsigned char   UCHAR;
typedef unsigned int    UINT;
typedef unsigned long   ULONG;
typedef enum {FALSE, TRUE} BOOL;

/* Random number stuff that SHOULD have been there */
#include <time.h>
#include <stdio.h> /* Needed for NULL * */
#include <stdlib.h>
#define RANDOMIZE() (srand((UINT)time( (time_t *)NULL )))
/* This is skewed due to integer division, but
 * good enough when a is much smaller than RAND_MAX.
 */
#define RANDOM(a) (rand() / (RAND_MAX / (a) + 1))
#define RANDOMD(a) (rand() / (RAND_MAX / (a) + 1.0))

/* Stuff that's already there, but is faster as a MACRO */
#define MIN(a,b)  (((a) < (b)) ?  (a) : (b) )
#define MAX(a,b)  (((a) > (b)) ?  (a) : (b) )

#define ABS(a)    (((a) < 0  ) ? -(a) : (a) )

/* This seems nifty to me */
#define DONE        1
#define NOT_DONE		0

#endif
