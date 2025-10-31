/*
 * Amiga backend stuff for 'acidwarp'
 * by megacz@usa.com
 *
 * Hello! If you managed to look in here then maybe you can do a solid
 * cleanup on whole proggy ;-) ?
*/

#include "amiga.h"
#include "handy.h"
#include <signal.h>
#include <time.h>
#undef MIN
#undef MAX 
/* See Aminet(chunky.lha)! */
#include <chunky.h>



struct GfxBase *GfxBase = NULL;
extern struct IntuitionBase *IntuitionBase;
struct SignalSemaphore procsem;
struct procdata procdat;
struct Task *proctask = NULL;
struct Task *parenttask = NULL;
static struct amiga_screen as;
UBYTE progtitlebuf[PTBUF];
extern LONG amiopts;
extern UBYTE *buf_graf;
extern int XMax;
extern int YMax;
extern int ECSMODE;



/* ------------------------------- INTERNALS ------------------------------- */

BOOL amiga_openscreen(struct amiga_screen *as)
{
  if ((as->as_signal = AllocSignal(-1)) != -1)
  {
    if ((as->as_screen = OpenScreenTags(NULL, 
                                         SA_Depth,           as->as_depth,
                                         SA_DisplayID,       as->as_modeid,
                                         SA_Title,           (LONG)as->as_scrname,
                                         SA_PubName,         (LONG)as->as_pubname,
                                         SA_PubSig,          as->as_signal,
                                         SA_ShowTitle,       FALSE,
                                         SA_Quiet,           TRUE,
                                         /*SA_Interleaved,     TRUE,*/
                                         TAG_DONE,           0)) != NULL)
    {
      SetRGB4(&as->as_screen->ViewPort, 0, 0, 0, 0);

      if ((as->as_window = OpenWindowTags(NULL,
                                          WA_Left,           0,
                                          WA_Top,            0,
                                          WA_MinWidth,       as->as_screen->Width,
                                          WA_MinHeight,      as->as_screen->Height,
                                          WA_Width,          as->as_screen->Width,
                                          WA_Height,         as->as_screen->Height,
                                          WA_MaxWidth,       as->as_screen->Width,
                                          WA_MaxHeight,      as->as_screen->Height,
                                          WA_PubScreen,      (LONG)as->as_screen,
                                          WA_SizeGadget,     FALSE,
                                          WA_DragBar,        FALSE,
                                          WA_DepthGadget,    FALSE,
                                          WA_CloseGadget,    FALSE,
                                          WA_Backdrop,       TRUE,
                                          WA_Borderless,     TRUE,
                                          WA_SimpleRefresh,  FALSE,
                                          WA_Activate,       TRUE,
                                          WA_IDCMP,          IDCMP_VANILLAKEY,
                                          TAG_DONE,          0)) != NULL)
      {
        if (amiga_openosd(as))
        {
          CopyMem(as->as_window->RPort, &as->as_temprp, sizeof(struct RastPort));

          as->as_temprp.Layer = NULL;

          if ((as->as_temprp.BitMap = AllocBitMap(as->as_screen->Width, 1, 
               as->as_depth, BMF_CLEAR, as->as_window->RPort->BitMap)) != NULL)
          {
            return TRUE;
          }

          amiga_closeosd(as);
        }

        CloseWindow(as->as_window);

        as->as_window = NULL;
      }

      CloseScreen(as->as_screen);

      as->as_screen = NULL;
    }

    FreeSignal(as->as_signal);

    as->as_signal = -1;
  }

  return FALSE;
}

void amiga_closescreen(struct amiga_screen *as)
{
  BOOL chk = TRUE;


  if (as->as_temprp.BitMap)
  {
    FreeBitMap(as->as_temprp.BitMap);

    as->as_temprp.BitMap = NULL;
  }

  amiga_closeosd(as);

  if (as->as_window)
  {
    CloseWindow(as->as_window);

    as->as_window = NULL;
  }

  if ((as->as_screen) && (as->as_signal > -1))
  {
    while (chk)
    {
      Wait((1L << as->as_signal) | SIGBREAKF_CTRL_C);

      Forbid();

      chk = amiga_chkforvis(as->as_screen);

      if (!(chk))
      {
        if (as->as_screen)
        {
          CloseScreen(as->as_screen);

          as->as_screen = NULL;
        }

        if (as->as_signal != -1)
        {
          FreeSignal(as->as_signal);

          as->as_signal = -1;
        }
      }

      Permit();
    }
  }
}

BOOL amiga_openosd(struct amiga_screen *as)
{
  as->as_tf = NULL;

  if ((as->as_osd = OpenWindowTags(NULL,
                                          WA_Left,           OSD_POS_X,
                                          WA_Top,            as->as_screen->Height - OSD_POS_Y,
                                          WA_MinWidth,       OSD_SIZE_X,
                                          WA_MinHeight,      OSD_SIZE_Y,
                                          WA_Width,          OSD_SIZE_X,
                                          WA_Height,         OSD_SIZE_Y,
                                          WA_MaxWidth,       OSD_SIZE_X,
                                          WA_MaxHeight,      OSD_SIZE_Y,
                                          WA_PubScreen,      (LONG)as->as_screen,
                                          WA_SizeGadget,     FALSE,
                                          WA_DragBar,        FALSE,
                                          WA_DepthGadget,    FALSE,
                                          WA_CloseGadget,    FALSE,
                                          WA_Backdrop,       TRUE,
                                          WA_Borderless,     TRUE,
                                          WA_SimpleRefresh,  FALSE,
                                          WA_Activate,       FALSE,
                                          TAG_DONE,          0)) != NULL)
  {
    as->as_ta.ta_Name = "topaz.font";
  
    as->as_ta.ta_YSize = 8;
  
    as->as_ta.ta_Style = 0;
  
    as->as_ta.ta_Flags = 0;

    if ((as->as_tf = OpenFont(&as->as_ta)))
    {
      SetFont(as->as_osd->RPort, as->as_tf);
    }

    WindowToBack(as->as_osd);

    return TRUE;
  }

  return FALSE;
}

void amiga_closeosd(struct amiga_screen *as)
{
  if (as->as_tf)
  {
    CloseFont(as->as_tf);

    as->as_tf = NULL;
  }

  if (as->as_osd)
  {
    CloseWindow(as->as_osd);

    as->as_osd = NULL;
  }
}

void amiga_putosdtext(struct amiga_screen *as, UBYTE *string)
{
  if (string)
  {
    WindowToFront(as->as_osd);

    SetRast(as->as_osd->RPort, OSD_B_PEN);

    SetBPen(as->as_osd->RPort, OSD_B_PEN);

    SetAPen(as->as_osd->RPort, OSD_A_PEN);

    Move(as->as_osd->RPort, OSD_DRAW_X, OSD_DRAW_Y);

    Text(as->as_osd->RPort, string, strlen(string));
  }
  else
  {
    WindowToBack(as->as_osd);
  }
}

BOOL amiga_chkforvis(struct Screen *screen)
{
  if (screen->FirstWindow != NULL)
  {
    return TRUE;
  }
  else
  {
    if (amiga_getviscnt(screen) != 0)
    {
      return TRUE;
    }
  }

  return FALSE;
}

WORD amiga_getviscnt(struct Screen *screen)
{
  struct PubScreenNode *psn;
  struct List *publist;
  WORD rc = -1;


  Forbid();

  if ((publist = LockPubScreenList()) != NULL)
  {
    ITERATE_LIST(publist, struct PubScreenNode *, psn)
    {
      if (psn->psn_Screen == screen)
      {
        rc = psn->psn_VisitorCount;

        break;
      }
    }

    UnlockPubScreenList();
  }

  Permit();

  return rc;
}

BOOL amiga_openlibs(void)
{
  if((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37L)) != NULL)
  {
/*
    if((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37L)) != NULL)
    {          
      return TRUE;
    }
*/
    return TRUE;
  }

  amiga_closelibs();

  return FALSE;
}

void amiga_closelibs(void)
{
/*
  if (IntuitionBase)
  {
    CloseLibrary((struct Library *)IntuitionBase);
  
    IntuitionBase = NULL;
  }
*/

  if (GfxBase)
  {
    CloseLibrary((struct Library *)GfxBase);

    GfxBase = NULL;
  }
}

int amiga_getkey(struct amiga_screen *as)
{
  struct IntuiMessage *msg;
  int rc = 0;


  while ((msg=(struct IntuiMessage *)GetMsg(as->as_window->UserPort)))
  {
    if (msg->Class == IDCMP_VANILLAKEY)
    {
      rc = msg->Code;
    }

    ReplyMsg((struct Message *)msg);
  }

  return rc;
}

ULONG amiga_getwbmodeid(void)
{
  struct Screen *screen;
  ULONG rc = 0;
  ULONG lock;


  lock = LockIBase(NULL);

  screen = IntuitionBase->FirstScreen;

  while (screen)
  {
    if ((strcmp(screen->Title, "Workbench Screen")) == NULL)
    {
      rc = GetVPModeID(&screen->ViewPort);

      break;
    }

    screen = screen->NextScreen;
  }

  UnlockIBase(lock);

  return rc;
}

ULONG *amiga_allocrgb32(ULONG num)
{
  ULONG *palette;


  if ((palette = AllocVec(((num * 3) + 1 + 1) * 4, MEMF_ANY | MEMF_CLEAR)) != NULL)
  {
    palette[0] = ((num << 16) + 0);
  }

  return palette;
}

ULONG amiga_detecttask(ULONG address)
{
  struct Task *task;
  ULONG rc = 0;


  Forbid();

  ITERATE_LIST(&SysBase->TaskReady, struct Task *, task)
  {
    if (address == (ULONG)task)
    {
      rc = 1;

      goto ___quit;
    }
  }

  ITERATE_LIST(&SysBase->TaskWait, struct Task *, task)
  {
    if (address == (ULONG)task)
    {
      rc = 1;

      break;
    }
  }

  ___quit:
  Permit();

  return rc;
}

ULONG amiga_forceclut(ULONG modeid)
{
  struct NameInfo ni;
  struct DimensionInfo di;
  DisplayInfoHandle dih;
  UBYTE stringcpy[128];
  UBYTE *nameptr;
  ULONG lmid = INVALID_ID;
  ULONG rc = modeid;
  UWORD dimx = 0;
  UWORD dimy = 0;
  UWORD cdimx = 8192;
  UWORD cdimy = 8192;


  if ((dih = FindDisplayInfo(modeid)))
  {
    if (GetDisplayInfoData(dih, (char *)&di, sizeof(struct DimensionInfo), DTAG_DIMS, NULL))
    {
      if (di.MaxDepth > 8)
      {
        dimx = di.Nominal.MaxX;
    
        dimy = di.Nominal.MaxY;

        rc = INVALID_ID;

        if (GetDisplayInfoData(dih, (char *)&ni, sizeof(struct NameInfo), DTAG_NAME, NULL))
        {
          if ((nameptr = strchr(ni.Name, ':')))
          {
            *nameptr = '\0';
          }
    
          stringcpy[0] = '\0';
    
          strncat(stringcpy, (char *)ni.Name, 128);
    
          while ((lmid = NextDisplayInfo(lmid)) != INVALID_ID)
          {
            if ((dih = FindDisplayInfo(lmid)))
            {
              if (GetDisplayInfoData(dih, (char *)&ni, sizeof(struct NameInfo), DTAG_NAME, NULL))
              {
                if ((nameptr = strchr(ni.Name, ':')))
                {
                  *nameptr = '\0';
                }
  
                if ((strcmp((char *)ni.Name, (char *)stringcpy)) == NULL)
                {
                  if (GetDisplayInfoData(dih, (char *)&di, sizeof(struct DimensionInfo), DTAG_DIMS, NULL))
                  {
                    if (di.MaxDepth <= 8)
                    {
                      if ((dimx == di.Nominal.MaxX)  &&
                          (dimy == di.Nominal.MaxY))
                      {
                        rc = lmid;
  
                        break;
                      }
  
                      if ((di.Nominal.MaxX < cdimx)  ||
                          (di.Nominal.MaxY < cdimy))
                      {
                        rc = lmid;
  
                        cdimx = di.Nominal.MaxX;
  
                        cdimy = di.Nominal.MaxY;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return rc;
}



/* ------------------------------- BORROWED -------------------------------- */

ULONG mst_AtoULONG(UBYTE *string)
{
  register UBYTE *str_r = string;
  register UBYTE *hex_r = string + 9;
  register UBYTE *end_r = NULL;
  register ULONG value = 0;
  register ULONG place = 1;


  if (*str_r == '$')
  {
    end_r = str_r;

    while ((*++str_r != '\0') && (str_r < hex_r));
  }
  else
  {
    if (*str_r == '0')
    {
      str_r++;

      if (*str_r == 'x')
      {
        hex_r++;

        end_r = str_r;

        while ((*++str_r != '\0') && (str_r < hex_r));
      }
      else
      {
        str_r = string;
      }
    }
  }

  if (end_r != NULL)
  {
    if (*str_r == '\0')
    {
      while (--str_r > end_r)
      {
        if (((*str_r < '0') || (*str_r > '9'))  &&
            ((*str_r < 'a') || (*str_r > 'f'))  &&
            ((*str_r < 'A') || (*str_r > 'F')))
        {
          return 0;
        }
        else
        {
          if (*str_r > '9')
          {
            if (*str_r > 'F')
            {
              value += place * (*str_r - 87);
            }
            else
            {
              value += place * (*str_r - 55);
            }
          }
          else
          {
            value += place * (*str_r - '0');
          }
  
          place *= 16;
        }
      }
    }
  }
  else
  {
    while (*str_r)
    {
      if ((*str_r < '0') || (*str_r > '9'))
      {
        return 0;
      }
      else
      {
        value = 10 * value + (*str_r++ - '0');
      }
    }
  }

  return value;
}

char *mst_modULONGtoAHEX(char *buffer, ULONG value, UBYTE align)
{
  register char *output = "0123456789ABCDEF";
  register char *bufptr = (char *)((LONG)buffer + (LONG)(ITOAHEXBUF - 1));
  register ULONG val_r = value;


  *bufptr = '\0';

  do
  {
    *--bufptr = output[val_r & 15];

    val_r >>= 4;
  } while (val_r);

  if (align > 8)
  {
    align = 8;
  }

  buffer = (char *)((LONG)buffer + (LONG)((ITOAHEXBUF - 1) - align));

  while(bufptr > buffer)
  {
    *--bufptr = '0';
  }

  *--bufptr = '$';

  return bufptr;
}



/* ---------------------------------- API ---------------------------------- */

int amikey_keypressed(int anything)
{
  return amiga_getkey(&as);
}

BOOL amibreaksig_check(void)
{
  if ((SetSignal(0L,0L) & SIGBREAKF_CTRL_C) == SIGBREAKF_CTRL_C)
  {
    return TRUE;
  }

  return FALSE;
}

void unix_control_c_handler(int signo) 
{
  Signal(FindTask(0L), SIGBREAKF_CTRL_C); 
}

void gl_setpalettecolors(int s, int n, void *_dp)
{
  unsigned char *dp = _dp;
  int i;
  int j;


  if (as.as_ecs)
  {
    j = 0;
    for (i = s; i < s + n; i++)
    {
      unsigned char r = *(dp++);
      unsigned char g = *(dp++);
      unsigned char b = *(dp++);
      if (ECSMODE == 0)
      {
         as.as_rgb4[j] = (UBYTE)((r / 4)) << 8;
         as.as_rgb4[j] |= (UBYTE)((g / 4)) << 4;
         as.as_rgb4[j] |= (UBYTE)((b % 16));
      }
      else if (ECSMODE == 1)
      {
         as.as_rgb4[j] = (UBYTE)((r % 16)) << 8;
         as.as_rgb4[j] |= (UBYTE)((g % 16)) << 4;
         as.as_rgb4[j] |= (UBYTE)((b % 16));
      }
      else
      {
         as.as_rgb4[j] = (UBYTE)((r % 16)) << 8;
         as.as_rgb4[j] |= (UBYTE)((g % 16)) << 4;
         as.as_rgb4[j] |= (UBYTE)((b / 4));
      }
      j++;
    }
    LoadRGB4(&as.as_screen->ViewPort, as.as_rgb4, 64);
  }
  else
  {
    j = 1;
    for (i = s; i < s + n; i++)
    {
      as.as_rgb32[j] = *(dp++) * 0x3FFFFFF;   /* R */
      as.as_rgb32[++j] = *(dp++) * 0x3FFFFFF; /* G */
      as.as_rgb32[++j] = *(dp++) * 0x3FFFFFF; /* B */
      j++;
    }

    LoadRGB32(&as.as_screen->ViewPort, as.as_rgb32);
  }
}

void gl_putbox(int x, int y, int w, int h, void *dp)
{
  struct c2pStruct c2p;


  LockLayerRom(as.as_window->RPort->Layer);

  if ((amiopts & AO_BMAP) == AO_BMAP)
  {
    c2p.bmap = as.as_window->RPort->BitMap;
    c2p.startx = x;
    c2p.starty = y;
    c2p.width = w;
    c2p.height = h;
    c2p.ChunkyBuffer = dp;

    ChunkyToPlanar(&c2p);
  }
  else
  {
    if (GfxBase->LibNode.lib_Version >= 40)
    {
      WriteChunkyPixels(as.as_window->RPort, x, y, w, h, dp, w);
    }
    else
    {
      WritePixelArray8(as.as_window->RPort, x, y, w-1, h-1, dp, &as.as_temprp);
    }
  }

  UnlockLayerRom(as.as_window->RPort->Layer);
}

BOOL amigl_allocatecontext(ULONG modeid)
{
  UBYTE hexbuf[ITOAHEXBUF];
  UBYTE *hexptr;


  parenttask = FindTask(NULL);
 
  hexptr = mst_modULONGtoAHEX(hexbuf, (ULONG)parenttask, 8); 

  progtitlebuf[0] = '\0';

  strncat(progtitlebuf, "AcidWarp(", PTBUF);

  strncat(progtitlebuf, hexptr, PTBUF);

  strncat(progtitlebuf, ")", PTBUF);

  as.as_ecs = 0;

  as.as_screen = NULL;

  as.as_window = NULL;

  as.as_osd = NULL;

  as.as_signal = -1;

  if ((amiopts & AO_ECS) == AO_ECS)
  {
    as.as_depth = 6;

    as.as_ecs = 1;
  }
  else
  {
    as.as_depth = 8;
  }

  as.as_scrname = progtitlebuf;

  as.as_pubname = as.as_scrname;

  as.as_rgb32 = NULL;

  if((signal(SIGINT, &unix_control_c_handler)) == SIG_ERR) 
  { 
    return FALSE;
  }

  InitSemaphore(&procsem);

  procdat.task = NULL;

  if (amiga_openlibs())
  {
    if (modeid == 0)
    {
      as.as_modeid = amiga_getwbmodeid();
    }
    else
    {
      as.as_modeid = modeid;
    }

    as.as_modeid = amiga_forceclut(as.as_modeid);

    /* everything beyond this value is assumed to be gfx card */
    if (as.as_modeid > 0xAFFFF)
    {
      amiopts &= ~AO_BMAP;
    }

    if (amiga_openscreen(&as))
    {
      XMax = as.as_screen->Width;

      YMax = as.as_screen->Height;

      if ((buf_graf = AllocVec(((as.as_screen->Width + 1) * (as.as_screen->Height + 1)), MEMF_ANY | MEMF_CLEAR)) != NULL)
      {
        if ((as.as_rgb32 = amiga_allocrgb32(256)) != NULL)
        {
          return TRUE;
        }
      }
    }
  }

  amigl_destroycontext();

  return FALSE;
}

void amigl_destroycontext(void)
{
  WaitBlit();

  Forbid();

  if ((procdat.task) && (amiga_detecttask((ULONG)procdat.task)))
  {
    Signal(procdat.task, SIGBREAKF_CTRL_C);
  }

  Permit();

  ObtainSemaphore(&procsem);

  ReleaseSemaphore(&procsem);

  if (as.as_rgb32)
  {
    FreeVec(as.as_rgb32);

    as.as_rgb32 = NULL;
  }

  if (buf_graf)
  {
    FreeVec(buf_graf);

    buf_graf = NULL;
  }

  amiga_closescreen(&as);

  amiga_closelibs();
}

void amihelp_convbuftoecs(UBYTE *buffer, ULONG buflen, UBYTE peek)
{
  register UBYTE *bufreg = (UBYTE *)((LONG)buffer - (LONG)1);
  register UBYTE *bufend = (UBYTE *)((LONG)buffer + (LONG)buflen);
  LONG mpeek = (peek + 1) * ((256 / (peek + 1)) / 2);


  while (++bufreg < bufend)
  {
    if ((*bufreg > peek)   &&
        (*bufreg < mpeek))
    {
      *bufreg = (peek + 1) - (*bufreg % peek);
    }
    else
    {
      *bufreg %= peek;
    }
  }
}

void amihelp_renderproc(void)
{
  struct ExecBase *SysBase = (*((struct ExecBase **) 4));


  ObtainSemaphore(&procsem);

  procdat.task = FindTask(NULL);

  Signal(parenttask, SIGBREAKF_CTRL_E);

  if ((amiopts & AO_SYNC) == AO_SYNC)
  {
    procdat.task->tc_Node.ln_Pri--;
  }

  generate_image(procdat.imageFuncNum, procdat.buf_graf, procdat.xcenter, 
            procdat.ycenter, procdat.xmax, procdat.ymax, procdat.colormax);

  procdat.task = NULL;

  ReleaseSemaphore(&procsem);
}

void amihelp_renderbuffer(int imageFuncNum, int maxfunc, UBYTE *buf_graf, int xcenter, 
                          int ycenter, int xmax, int ymax, int colormax)
{
  procdat.imageFuncNum = imageFuncNum;

  procdat.buf_graf = buf_graf;

  procdat.xcenter = xcenter;

  procdat.ycenter = ycenter;

  procdat.xmax = xmax;

  procdat.ymax = ymax;

  procdat.colormax = colormax;

  if (proctask == NULL)
  {
    procdat.imageFuncNum = RANDOM(maxfunc + 1);

    if (AttemptSemaphore(&procsem))
    {
      generate_image(imageFuncNum, buf_graf, xcenter, ycenter, xmax, ymax, colormax);

      ReleaseSemaphore(&procsem);
    }
  }

  ObtainSemaphore(&procsem);

  ReleaseSemaphore(&procsem);

  gl_putbox(0,0,xmax,ymax,buf_graf);

  proctask = (struct Task *)CreateNewProcTags(NP_Entry      , (ULONG)amihelp_renderproc,
                                            NP_Cli        , TRUE,
                                            NP_CommandName, (ULONG)"render",
                                            NP_Name       , (ULONG)progtitlebuf,
                                            NP_Priority   , (parenttask->tc_Node.ln_Pri),
                                            NP_StackSize  , 4096,
                                            TAG_DONE      , NULL);

  if (proctask != NULL)
  {
    Wait(SIGBREAKF_CTRL_E);
  }
}

int amihelp_putosdtext(UBYTE *text)
{
  amiga_putosdtext(&as, text);

  return (time(NULL) + OSD_TIME);
}
