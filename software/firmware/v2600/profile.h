/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: profile.h,v 1.2 1996/04/01 14:45:19 alex Exp $
******************************************************************************/

/*
  Prototypes for instruction profiling.
  */

#ifndef PROFILE_H
#define PROFILE_H

extern long profile_count[256];
extern void profile_init(void);
extern void profile_report(void);

#endif
