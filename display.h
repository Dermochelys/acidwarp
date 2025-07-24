/* display.h */

void disp_setPalette(unsigned char *palette);

void disp_beginUpdate(UCHAR **p,
                      unsigned int *pitch,
                      unsigned int *w,
                      unsigned int *h);

void disp_finishUpdate(void);
void disp_swapBuffers(void);
void disp_processInput(void);

void disp_init(int width,
               int height,
               int flags);
