// Matthew Stroup
// Joystick driver for v2600
// January 23, 1997

#include "config.h"
#include "options.h"
#include <exec/types.h>
#include <hardware/custom.h>
#include <hardware/cia.h>

#define LJOYMASK 0x01
#define RJOYMASK 0x02
#define UJOYMASK 0x04
#define DJOYMASK 0x08
#define B1JOYMASK 0x10
#define B2JOYMASK 0x20

#define CIAAPRA 0xBFE001 

#define FIRE   1
#define RIGHT  2
#define LEFT   4
#define DOWN   8
#define UP    16

extern struct Custom far custom;
struct CIA *ciajoy = (struct CIA *) CIAAPRA;
static int joyres[2] = {0, 0};

int get_realjoy (int stick)
{
  return joyres[stick];
}

int read_realjoy (int stick)
{
UWORD joy;
int blah=0;

	if (stick == 1) joy = custom.joy0dat;
	else joy=custom.joy1dat;

	if (!(ciajoy->ciapra & 0x0080)) blah|=B1JOYMASK;
	if (joy & 0x0002) blah|=RJOYMASK;
	if (joy & 0x0200) blah|=LJOYMASK;
	if ((joy >> 1 ^ joy) & 0x0001) blah|=DJOYMASK;
	if ((joy >> 1 ^ joy) & 0x0100) blah|=UJOYMASK;
	return(blah);
}


/* This should be called once per frame */
void update_realjoy (void)
{
	if (base_opts.realjoy == 0) return;
	joyres[0] = read_realjoy (0);
	joyres[1] = read_realjoy (1);
}


/* These can be called from anywhere. */
void calibrate_realjoy (int stick)
{
}


void init_realjoy (void)
{
}


void close_realjoy (void)
{
}
