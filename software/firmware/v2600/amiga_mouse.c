#include "config.h"
#include "options.h"
#include <stdio.h>
#include <exec/types.h>
#include <hardware/custom.h>
#include <hardware/cia.h>


#define CIAAPRA 0xBFE001


extern struct Custom far custom;
struct CIA *ciamou = (struct CIA *) CIAAPRA;

extern int moudrv_x, moudrv_y, moudrv_but;

/* Used to initialise mouse. Not needed for Amiga */
void moudrv_init(void)
{
}

void moudrv_update(void)
{
}

void moudrv_close(void)
{
}

/* Used to read mouse */
void moudrv_read(void)
{
UWORD joy;

	joy = custom.joy0dat;
	moudrv_x=(joy & 0x00FF);
	moudrv_y=(joy >> 8);
	moudrv_but=!(ciamou->ciapra & 0x0040);
}
