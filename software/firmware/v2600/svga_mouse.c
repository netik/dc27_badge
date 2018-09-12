#include "config.h"
#include "options.h"
#include <stdio.h>
#include <vga.h>
#include <vgamouse.h>

extern int moudrv_x, moudrv_y, moudrv_but;

/* Used to initialise mouse. Not needed under X */
void
moudrv_init(void)
{
  int type;

  type= vga_getmousetype();

  mouse_init("/dev/mouse", type, MOUSE_DEFAULTSAMPLERATE);
  mouse_setwrap(MOUSE_NOWRAP);
  mouse_setxrange(0,319);
  mouse_setposition(160,100);
}

void
moudrv_update(void)
{
  mouse_update();
}

void
moudrv_close(void)
{
  mouse_close();
}

/* Used to read mouse. Not needed under X */
void
moudrv_read(void)
{
  moudrv_x=mouse_getx();
  moudrv_y=mouse_gety();
  moudrv_but=(mouse_getbutton() & MOUSE_LEFTBUTTON)>>2;
}





