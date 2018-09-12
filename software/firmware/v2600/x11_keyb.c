/*****************************************************************************

   This file is part of Virtual 2600, the Atari 2600 Emulator
   ==========================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: x11_keyb.c,v 1.5 1996/12/03 22:35:05 ahornby Exp $
******************************************************************************/

/* The keyboard and mouse interface. */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Intrinsic.h>
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

/* The main Xt application context */
extern XtAppContext app_context;
extern Widget canvas, toplevel;

extern int keymap[103];

#define NUMKEY 65536
static int keyboard_array[NUMKEY];

static void
process_key (Widget w, XtPointer p, XEvent * ev, Boolean * b)
{
  XKeyEvent *kev = (XKeyEvent *) ev;
  KeySym ks;
  XLookupString (kev, NULL, 0, &ks, NULL);

  switch (((XKeyEvent *) kev)->type)
    {
    case KeyPress:
      keyboard_array[ks] = 1;
      break;
    case KeyRelease:
      keyboard_array[ks] = 0;
      break;
    }
}

/* Update the current keyboard state. */
void
keybdrv_update (void)
{
  tv_event ();
}

/* Is this key presssed? */
int
keybdrv_pressed(int key)
{
  return (keyboard_array[keymap[key]]);
}

/* Defines the keyboard mappings */
void
keybdrv_setmap(void)
{
  keymap[  kmapSYSREQ              ]=XK_Pause;
  keymap[  kmapCAPSLOCK            ]=XK_Caps_Lock;
  keymap[  kmapNUMLOCK             ]=XK_Num_Lock;
  keymap[  kmapSCROLLLOCK          ]=XK_Scroll_Lock;
  keymap[  kmapLEFTCTRL            ]=XK_Control_L;
  keymap[  kmapLEFTALT             ]=XK_Alt_L;
  keymap[  kmapLEFTSHIFT           ]=XK_Shift_L;
  keymap[  kmapRIGHTCTRL           ]=XK_Control_R;
  keymap[  kmapRIGHTALT            ]=XK_Alt_R;
  keymap[  kmapRIGHTSHIFT          ]=XK_Shift_R;
  keymap[  kmapESC                 ]=XK_Escape;
  keymap[  kmapBACKSPACE           ]=XK_BackSpace;
  keymap[  kmapENTER               ]=XK_Return;
  keymap[  kmapSPACE               ]=XK_space;
  keymap[  kmapTAB                 ]=XK_Tab;
  keymap[  kmapF1                  ]=XK_F1;
  keymap[  kmapF2                  ]=XK_F2;
  keymap[  kmapF3                  ]=XK_F3;
  keymap[  kmapF4                  ]=XK_F4;
  keymap[  kmapF5                  ]=XK_F5;
  keymap[  kmapF6                  ]=XK_F6;
  keymap[  kmapF7                  ]=XK_F7;
  keymap[  kmapF8                  ]=XK_F8;
  keymap[  kmapF9                  ]=XK_F9;
  keymap[  kmapF10                 ]=XK_F10;
  keymap[  kmapF11                 ]=XK_F11;
  keymap[  kmapF12                 ]=XK_F12;
  keymap[  kmapA                   ]=XK_a;
  keymap[  kmapB                   ]=XK_b;
  keymap[  kmapC                   ]=XK_b;
  keymap[  kmapD                   ]=XK_d;
  keymap[  kmapE                   ]=XK_e;
  keymap[  kmapF                   ]=XK_f;
  keymap[  kmapG                   ]=XK_g;
  keymap[  kmapH                   ]=XK_h;
  keymap[  kmapI                   ]=XK_i;
  keymap[  kmapJ                   ]=XK_j;
  keymap[  kmapK                   ]=XK_k;
  keymap[  kmapL                   ]=XK_l;
  keymap[  kmapM                   ]=XK_m;
  keymap[  kmapN                   ]=XK_n;
  keymap[  kmapO                   ]=XK_o;
  keymap[  kmapP                   ]=XK_p;
  keymap[  kmapQ                   ]=XK_q;
  keymap[  kmapR                   ]=XK_r;
  keymap[  kmapS                   ]=XK_s;
  keymap[  kmapT                   ]=XK_t;
  keymap[  kmapU                   ]=XK_u;
  keymap[  kmapV                   ]=XK_v;
  keymap[  kmapW                   ]=XK_w ;
  keymap[  kmapX                   ]=XK_x ;
  keymap[  kmapY                   ]=XK_y;
  keymap[  kmapZ                   ]=XK_z ;
  keymap[  kmap1                   ]=XK_1;
  keymap[  kmap2                   ]=XK_2;
  keymap[  kmap3                   ]=XK_3;
  keymap[  kmap4                   ]=XK_4;
  keymap[  kmap5                   ]=XK_5;
  keymap[  kmap6                   ]=XK_6;
  keymap[  kmap7                   ]=XK_7;
  keymap[  kmap8                   ]=XK_8;
  keymap[  kmap9                   ]=XK_9;
  keymap[  kmap0                   ]=XK_0;
  keymap[  kmapMINUS               ]=XK_minus;
  keymap[  kmapEQUAL               ]=XK_equal;
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
  keymap[  kmapLARROW              ]=XK_Left;
  keymap[  kmapRARROW              ]=XK_Right;
  keymap[  kmapUARROW              ]=XK_Up;
  keymap[  kmapDARROW              ]=XK_Down;
  keymap[  kmapKEYPAD0             ]=XK_KP_0;
  keymap[  kmapKEYPAD1             ]=XK_KP_1;
  keymap[  kmapKEYPAD2             ]=XK_KP_2;
  keymap[  kmapKEYPAD3             ]=XK_KP_3;
  keymap[  kmapKEYPAD4             ]=XK_KP_4;
  keymap[  kmapKEYPAD5             ]=XK_KP_5;
  keymap[  kmapKEYPAD6             ]=XK_KP_6;
  keymap[  kmapKEYPAD7             ]=XK_KP_7;
  keymap[  kmapKEYPAD8             ]=XK_KP_8;
  keymap[  kmapKEYPAD9             ]=XK_KP_9;
  keymap[  kmapKEYPADDEL           ]=XK_KP_Decimal;
  keymap[  kmapKEYPADSTAR          ]=XK_KP_Multiply;
  keymap[  kmapKEYPADMINUS         ]=XK_KP_Subtract;
  keymap[  kmapKEYPADPLUS          ]=XK_KP_Add;
  keymap[  kmapKEYPADENTER         ]=XK_KP_Enter;
  keymap[  kmapCTRLPRTSC           ]=0xB7;
  keymap[  kmapSHIFTPRTSC          ]=0xB7;
  keymap[  kmapKEYPADSLASH         ]=XK_KP_Divide;
}

/* Zeros the keyboard array, and installs event handlers. */
int
keybdrv_init (void)
{
  int i;

  /* Zero the array */
  for (i = 0; i < NUMKEY; i++)
    keyboard_array[NUMKEY] = 0;

  /* Install the event handler */
  XtAddEventHandler(canvas, KeyPressMask | KeyReleaseMask, 0, process_key, NULL);
  return 0;
}

/* Close the keyboard and tidy up */
void
keybdrv_close (void)
{
  int i;
  /* Remove the event handler */
  XtRemoveEventHandler (canvas, KeyPressMask | KeyReleaseMask,
			0, process_key, NULL);

  /* Zero the array */
  for (i = 0; i < NUMKEY; i++)
    keyboard_array[NUMKEY] = 0;
}







