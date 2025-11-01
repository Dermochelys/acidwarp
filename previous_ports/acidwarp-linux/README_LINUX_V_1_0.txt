Acidwarp for Linux 1.0 by Steven Wills <willssl@jmu.edu>

What is this?

This is a linux port of the popular dos program Acidwarp.  Acidwarp is a
program that makes neat looking pictures and rotates the palette. Its only
use is for entertainment, but it is neat. I stumbled upon source code for
Acidwarp on the net one day, and decided that it was high time someone
ported it to linux. So, here it is. (for more info about Acidwarp, see 
the README for the dos version, included below)

I used gcc 2.7.2, svgalib 1.2.10 and libc 5.2.18.  I added an option to
use higher resolution and control the rate of palette rotation. I did not
impliment the option to select which palette is used.  This port was done 
on intel hardware, but I suspect that it will work on others. This is 
the second public release of the linux version, and it has been fairly
well tested. Still, please let me know if you discover any bugs, or if you
just want to offer feedback. Thanks. 

In this version I have added a few more features. These are mostly
keyboard commands. They are:

  Up: causes the pallette to rotate faster.
Down: causes the pallette to rotate slower.
   l: (L)ock on the current pattern.
   k: switch to the next pallette.
   p: (P)ause. Totally stops pallette rotation and pattern changing.
   q: (Q)uit. Causes Acidwarp to exit.
   n: (N)ext pattern.

Acidwarp is distributed under the GPL.