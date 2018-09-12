/*****************************************************************************

   This file is part of Virtual 2600, the Atari 2600 Emulator
   ==========================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: svga_keyb.c,v 1.3 1996/08/28 14:32:34 ahornby Exp $
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

enum control {STICK, PADDLE, KEYPAD};

extern int mouse_x, mouse_y, mouse_but;

extern int keymap[103];

/* Update the current keyboard state. */
void
keybdrv_update (void)
{

}

/* Is this key presssed? */
int
keybdrv_pressed(int key)
{
#ifdef notdef
  return (keyboard_keypressed(keymap[key]));
#endif
  return (0);
}

/* Defines the keyboard mappings */
void
keybdrv_setmap(void)
{
#ifdef notdef
  keymap[  kmapSYSREQ              ]=SCANCODE_PRINTSCREEN;
  keymap[  kmapCAPSLOCK            ]=SCANCODE_CAPSLOCK;
  keymap[  kmapNUMLOCK             ]=SCANCODE_NUMLOCK;
  keymap[  kmapSCROLLLOCK          ]=SCANCODE_SCROLLLOCK;
  keymap[  kmapLEFTCTRL            ]=SCANCODE_LEFTCONTROL;
  keymap[  kmapLEFTALT             ]=SCANCODE_LEFTALT;
  keymap[  kmapLEFTSHIFT           ]=SCANCODE_LEFTSHIFT;
  keymap[  kmapRIGHTCTRL           ]=SCANCODE_RIGHTCONTROL;
  keymap[  kmapRIGHTALT            ]=SCANCODE_RIGHTALT;
  keymap[  kmapRIGHTSHIFT          ]=SCANCODE_RIGHTSHIFT;
  keymap[  kmapESC                 ]=SCANCODE_ESCAPE;
  keymap[  kmapBACKSPACE           ]=SCANCODE_BACKSPACE;
  keymap[  kmapENTER               ]=SCANCODE_ENTER;
  keymap[  kmapSPACE               ]=SCANCODE_SPACE;
  keymap[  kmapTAB                 ]=SCANCODE_TAB;
  keymap[  kmapF1                  ]=SCANCODE_F1;
  keymap[  kmapF2                  ]=SCANCODE_F2;
  keymap[  kmapF3                  ]=SCANCODE_F3;
  keymap[  kmapF4                  ]=SCANCODE_F4;
  keymap[  kmapF5                  ]=SCANCODE_F5;
  keymap[  kmapF6                  ]=SCANCODE_F6;
  keymap[  kmapF7                  ]=SCANCODE_F7;
  keymap[  kmapF8                  ]=SCANCODE_F8;
  keymap[  kmapF9                  ]=SCANCODE_F9;
  keymap[  kmapF10                 ]=SCANCODE_F10;
  keymap[  kmapF11                 ]=SCANCODE_F11;
  keymap[  kmapF12                 ]=SCANCODE_F12;
  keymap[  kmapA                   ]=SCANCODE_A;
  keymap[  kmapB                   ]=SCANCODE_B;
  keymap[  kmapC                   ]=SCANCODE_C;
  keymap[  kmapD                   ]=SCANCODE_D;
  keymap[  kmapE                   ]=SCANCODE_E;
  keymap[  kmapF                   ]=SCANCODE_F;
  keymap[  kmapG                   ]=SCANCODE_G;
  keymap[  kmapH                   ]=SCANCODE_H;
  keymap[  kmapI                   ]=SCANCODE_I;
  keymap[  kmapJ                   ]=SCANCODE_J;
  keymap[  kmapK                   ]=SCANCODE_K;
  keymap[  kmapL                   ]=SCANCODE_L;
  keymap[  kmapM                   ]=SCANCODE_M;
  keymap[  kmapN                   ]=SCANCODE_N;
  keymap[  kmapO                   ]=SCANCODE_O;
  keymap[  kmapP                   ]=SCANCODE_P;
  keymap[  kmapQ                   ]=SCANCODE_Q;
  keymap[  kmapR                   ]=SCANCODE_R;
  keymap[  kmapS                   ]=SCANCODE_S;
  keymap[  kmapT                   ]=SCANCODE_T;
  keymap[  kmapU                   ]=SCANCODE_U;
  keymap[  kmapV                   ]=SCANCODE_V;
  keymap[  kmapW                   ]=SCANCODE_W ;
  keymap[  kmapX                   ]=SCANCODE_X ;
  keymap[  kmapY                   ]=SCANCODE_Y;
  keymap[  kmapZ                   ]=SCANCODE_Z ;
  keymap[  kmap1                   ]=SCANCODE_1;
  keymap[  kmap2                   ]=SCANCODE_2;
  keymap[  kmap3                   ]=SCANCODE_3;
  keymap[  kmap4                   ]=SCANCODE_4;
  keymap[  kmap5                   ]=SCANCODE_5;
  keymap[  kmap6                   ]=SCANCODE_6;
  keymap[  kmap7                   ]=SCANCODE_7;
  keymap[  kmap8                   ]=SCANCODE_8;
  keymap[  kmap9                   ]=SCANCODE_9;
  keymap[  kmap0                   ]=SCANCODE_0;
  keymap[  kmapMINUS               ]=SCANCODE_MINUS;
  keymap[  kmapEQUAL               ]=SCANCODE_EQUAL;
  keymap[  kmapLBRACKET            ]=0x1A;
  keymap[  kmapRBRACKET            ]=0x1B;
  keymap[  kmapSEMICOLON           ]=0x27;
  keymap[  kmapTICK                ]=0x28;
  keymap[  kmapAPOSTROPHE          ]=0x29;
  keymap[  kmapBACKSLASH           ]=0x2B;
  keymap[  kmapCOMMA               ]=0x33;
  keymap[  kmapPERIOD              ]=0x34;
  keymap[  kmapSLASH               ]=0x35;
  keymap[  kmapINS                 ]=0xD2;
  keymap[  kmapDEL                 ]=0xD3;
  keymap[  kmapHOME                ]=0xC7;
  keymap[  kmapEND                 ]=0xCF;
  keymap[  kmapPGUP                ]=0xC9;
  keymap[  kmapPGDN                ]=0xD1;
  keymap[  kmapLARROW              ]=SCANCODE_CURSORLEFT;
  keymap[  kmapRARROW              ]=SCANCODE_CURSORRIGHT;
  keymap[  kmapUARROW              ]=SCANCODE_CURSORUP;
  keymap[  kmapDARROW              ]=SCANCODE_CURSORDOWN;
  keymap[  kmapKEYPAD0             ]=SCANCODE_KEYPAD0;
  keymap[  kmapKEYPAD1             ]=SCANCODE_KEYPAD1;
  keymap[  kmapKEYPAD2             ]=SCANCODE_KEYPAD2;
  keymap[  kmapKEYPAD3             ]=SCANCODE_KEYPAD3;
  keymap[  kmapKEYPAD4             ]=SCANCODE_KEYPAD4;
  keymap[  kmapKEYPAD5             ]=SCANCODE_KEYPAD5;
  keymap[  kmapKEYPAD6             ]=SCANCODE_KEYPAD6;
  keymap[  kmapKEYPAD7             ]=SCANCODE_KEYPAD7;
  keymap[  kmapKEYPAD8             ]=SCANCODE_KEYPAD8;
  keymap[  kmapKEYPAD9             ]=SCANCODE_KEYPAD9;
  keymap[  kmapKEYPADDEL           ]=SCANCODE_KEYPADPERIOD;
  keymap[  kmapKEYPADSTAR          ]=SCANCODE_KEYPADMULTIPLY;
  keymap[  kmapKEYPADMINUS         ]=SCANCODE_KEYPADMINUS;
  keymap[  kmapKEYPADPLUS          ]=SCANCODE_KEYPADPLUS;
  keymap[  kmapKEYPADENTER         ]=SCANCODE_KEYPADENTER;
  keymap[  kmapCTRLPRTSC           ]=SCANCODE_PRINTSCREEN;
  keymap[  kmapSHIFTPRTSC          ]=SCANCODE_PRINTSCREEN;
  keymap[  kmapKEYPADSLASH         ]=SCANCODE_KEYPADDIVIDE;
#endif

}

/* Zeros the keyboard array, and installs event handlers. */
int
keybdrv_init (void)
{
  return 0;
}

/* Close the keyboard and tidy up */
void
keybdrv_close (void)
{
	return;
}







