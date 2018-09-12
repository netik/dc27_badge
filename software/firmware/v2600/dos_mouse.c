#include "config.h"
#include "options.h"
#include <stdio.h>
#include <allegro.h>

extern int moudrv_x, moudrv_y, moudrv_but;

/* Used to initialise mouse. Not needed under X */
void
moudrv_init(void)
{
  install_mouse();
  show_mouse(NULL);
  position_mouse(160,100);
}

void
moudrv_update(void)
{

}

void
moudrv_close(void)
{
  remove_mouse();
}

/* Used to read mouse. Not needed under X */
void
moudrv_read(void)
{
  moudrv_x=mouse_x;
  moudrv_y=mouse_y;
  moudrv_but=mouse_b & 0x01;
}







