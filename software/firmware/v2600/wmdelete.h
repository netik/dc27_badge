/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: wmdelete.h,v 1.2 1996/04/01 14:51:50 alex Exp $
******************************************************************************/

/*
  Prototypes to code that handles WM_DELETE window manager messages.
  */


#ifndef WMDELETE_H
#define WMDELETE_H

void 
add_delete(Widget w, XtEventHandler HandleMessages);
void
unrealizeDelete (Widget w, XtPointer client_data, XEvent *ev, Boolean *disp);

#endif
