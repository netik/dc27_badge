/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: resource.h,v 1.6 1996/08/24 19:00:26 ahornby Exp $
******************************************************************************/

/*
  The application resource data is defined here.
  Make sure it matches with the code in display.c.
*/

#ifndef RESOURCE_H
#define RESOURCE_H

#include <X11/Intrinsic.h>

extern struct App_data {
    Bool mitshm;
    Bool private;
} app_data;

#endif
