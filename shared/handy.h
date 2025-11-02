#ifndef __HANDY
#define __HANDY 1

/* Two new control structures for C. Yay! */
#define loop      for (;;)
#define loopend(x) {if(x)break;}
/* Usage:  do { ... } until (boolean); */
#define until(a)  while (!(a))

/* Some basic data types */
/* typedef unsigned char   CHAR; Is this a typo */
typedef unsigned char   UCHAR;
typedef unsigned int    UINT;
typedef unsigned long   ULONG;
/* BOOL, FALSE, and TRUE - check if already defined (e.g., by windows.h) */
#ifndef FALSE
/* Neither windows.h nor BOOL have been defined yet - define our own */
typedef enum {FALSE, TRUE} BOOL;
#else
/* FALSE is already defined (likely from windows.h), ensure BOOL and TRUE exist */
#ifndef BOOL
typedef int BOOL;
#endif
#ifndef TRUE
#define TRUE 1
#endif
#endif

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

/* This seems nifty to me */
#define DONE        1
#define NOT_DONE		0

#endif
