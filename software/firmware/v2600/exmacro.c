/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for Details.
   
   $Id: exmacro.c,v 1.6 1996/08/26 15:04:20 ahornby Exp $
******************************************************************************/

/* Things that used to be macros, but are now functions.
   Done for ease of debugging */

#include "types.h"
#include "extern.h"
#include "macro.h"
#include "memory.h"

/* Loads an absolute address uising the quicker load mechanism */
ADDRESS
load_abs_addr (void)
{
  return (LOADEXEC (PC + 1) | (LOADEXEC (PC + 2) << 8));
}

/* Used in variable cycle time indexed addressing */
/* a: address to be incremented */
/* b: increment value */
/* returns: TRUE if an address increment will cross a page boundary */
int
pagetest (ADDRESS a, BYTE b)
{
  return (LOWER (a) + b > 0xff);
}

/* Used in variable cycle time branches */
/* a: high byte of new address */
/* returns: TRUE if a branch is to the same page */
int
brtest (BYTE a)
{
  return (UPPER (PC) == (a));
}

/* Convert from binary to BCD (Binary Coded Decimal) */
/* a: binary value */
/* returns: BCD value */
int
toBCD (int a)
{
  return (((a % 100) / 10) << 4) | (a % 10);
}

/* Convert from BCD to binary */
/* a: BCD value */
/* returns: binary value */
int
fromBCD (int a)
{
  return ((a >> 4) & 0xf) * 10 + (a & 0xf);
}
