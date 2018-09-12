/*****************************************************************************
Author     : Matthew Stroup
Description: Amiga keyboard driver for v2600 Atari emulator
Version    : 1.0 Jan 22, 1997
******************************************************************************/

/* The keyboard and mouse interface. */
#include <stdio.h>
#include "config.h"
#include "options.h"
#include "types.h"
#include "address.h"
#include "vmachine.h"
#include "macro.h"
#include "extern.h"
#include "memory.h"
#include "realjoy.h"
#include "display.h"
#include "kmap.h"
#include <exec/types.h>
#include <hardware/cia.h>
#include <stdio.h>

enum control {STICK, PADDLE, KEYPAD};

extern int mouse_x, mouse_y, mouse_but;

extern int keymap[103];

#define CIAA 0xBFE001

struct CIA *cia = (struct CIA *) CIAA;

/* Update the current keyboard state. */
void keybdrv_update (void)
{
}

/* Is this key presssed? */
int keybdrv_pressed(int key)
{
UBYTE code;

	//Here's an evil hardware-banging keyboard routine...
	/* Get a copy of the SDR value and invert it: */
	code = cia->ciasdr ^ 0xFF;
  
	/* Shift all bits one step to the right, and put the bit that is */
	/* pushed out last: 76543210 -> 07654321                         */
	code = code & 0x01 ? (code>>1)+0x80 : code>>1;

	/* Return the Raw Key Code Value: */
	return(code==keymap[key]);
}

/* Defines the keyboard mappings */
void keybdrv_setmap(void)
{
  keymap[  kmapSYSREQ              ]=0x5c;
  keymap[  kmapCAPSLOCK            ]=0x62;
  keymap[  kmapNUMLOCK             ]=0x5a;
  keymap[  kmapSCROLLLOCK          ]=0x5b;
  keymap[  kmapLEFTCTRL            ]=0x63;
  keymap[  kmapLEFTALT             ]=0x64;
  keymap[  kmapLEFTSHIFT           ]=0x60;
  keymap[  kmapRIGHTCTRL           ]=0x63;
  keymap[  kmapRIGHTALT            ]=0x65;
  keymap[  kmapRIGHTSHIFT          ]=0x61;
  keymap[  kmapESC                 ]=0x45;
  keymap[  kmapBACKSPACE           ]=0x41;
  keymap[  kmapENTER               ]=0x44;
  keymap[  kmapSPACE               ]=0x40;
  keymap[  kmapTAB                 ]=0x42;
  keymap[  kmapF1                  ]=0x50;
  keymap[  kmapF2                  ]=0x51;
  keymap[  kmapF3                  ]=0x52;
  keymap[  kmapF4                  ]=0x53;
  keymap[  kmapF5                  ]=0x54;
  keymap[  kmapF6                  ]=0x55;
  keymap[  kmapF7                  ]=0x56;
  keymap[  kmapF8                  ]=0x57;
  keymap[  kmapF9                  ]=0x58;
  keymap[  kmapF10                 ]=0x59;
  keymap[  kmapF11                 ]=0x59;
  keymap[  kmapF12                 ]=0x59;
  keymap[  kmapA                   ]=0x20;
  keymap[  kmapB                   ]=0x35;
  keymap[  kmapC                   ]=0x33;
  keymap[  kmapD                   ]=0x22;
  keymap[  kmapE                   ]=0x12;
  keymap[  kmapF                   ]=0x23;
  keymap[  kmapG                   ]=0x24;
  keymap[  kmapH                   ]=0x25;
  keymap[  kmapI                   ]=0x17;
  keymap[  kmapJ                   ]=0x26;
  keymap[  kmapK                   ]=0x27;
  keymap[  kmapL                   ]=0x28;
  keymap[  kmapM                   ]=0x37;
  keymap[  kmapN                   ]=0x36;
  keymap[  kmapO                   ]=0x18;
  keymap[  kmapP                   ]=0x19;
  keymap[  kmapQ                   ]=0x10;
  keymap[  kmapR                   ]=0x13;
  keymap[  kmapS                   ]=0x21;
  keymap[  kmapT                   ]=0x14;
  keymap[  kmapU                   ]=0x16;
  keymap[  kmapV                   ]=0x34;
  keymap[  kmapW                   ]=0x11;
  keymap[  kmapX                   ]=0x32;
  keymap[  kmapY                   ]=0x15;
  keymap[  kmapZ                   ]=0x31;
  keymap[  kmap1                   ]=0x1;
  keymap[  kmap2                   ]=0x2;
  keymap[  kmap3                   ]=0x3;
  keymap[  kmap4                   ]=0x4;
  keymap[  kmap5                   ]=0x5;
  keymap[  kmap6                   ]=0x6;
  keymap[  kmap7                   ]=0x7;
  keymap[  kmap8                   ]=0x8;
  keymap[  kmap9                   ]=0x9;
  keymap[  kmap0                   ]=0xa;
  keymap[  kmapMINUS               ]=0xb;
  keymap[  kmapEQUAL               ]=0xc;
  keymap[  kmapLBRACKET            ]=0x1a;
  keymap[  kmapRBRACKET            ]=0x1b;
  keymap[  kmapSEMICOLON           ]=0x29;
  keymap[  kmapTICK                ]=0x0;
  keymap[  kmapAPOSTROPHE          ]=0x2a;
  keymap[  kmapBACKSLASH           ]=0xd;
  keymap[  kmapCOMMA               ]=0x38;
  keymap[  kmapPERIOD              ]=0x39;
  keymap[  kmapSLASH               ]=0x3a;
  keymap[  kmapINS                 ]=0xf;
  keymap[  kmapDEL                 ]=0x46;
  keymap[  kmapHOME                ]=0x3d;
  keymap[  kmapEND                 ]=0x1d;
  keymap[  kmapPGUP                ]=0x3f;
  keymap[  kmapPGDN                ]=0x1f;
  keymap[  kmapLARROW              ]=0x4f;
  keymap[  kmapRARROW              ]=0x4e;
  keymap[  kmapUARROW              ]=0x4c;
  keymap[  kmapDARROW              ]=0x4d;
  keymap[  kmapKEYPAD0             ]=0xf;
  keymap[  kmapKEYPAD1             ]=0x1d;
  keymap[  kmapKEYPAD2             ]=0x1e;
  keymap[  kmapKEYPAD3             ]=0x1f;
  keymap[  kmapKEYPAD4             ]=0x2d;
  keymap[  kmapKEYPAD5             ]=0x2e;
  keymap[  kmapKEYPAD6             ]=0x2f;
  keymap[  kmapKEYPAD7             ]=0x3d;
  keymap[  kmapKEYPAD8             ]=0x3e;
  keymap[  kmapKEYPAD9             ]=0x3f;
  keymap[  kmapKEYPADDEL           ]=0x3c;
  keymap[  kmapKEYPADSTAR          ]=0x5d;
  keymap[  kmapKEYPADMINUS         ]=0x4a;
  keymap[  kmapKEYPADPLUS          ]=0x5e;
  keymap[  kmapKEYPADENTER         ]=0x43;
  keymap[  kmapCTRLPRTSC           ]=0x5d;
  keymap[  kmapSHIFTPRTSC          ]=0x5d;
  keymap[  kmapKEYPADSLASH         ]=0x5c;
}

/* Zeros the keyboard array, and installs event handlers. */
int keybdrv_init (void)
{
	return 0;
}

/* Close the keyboard and tidy up */
void keybdrv_close (void)
{
}
