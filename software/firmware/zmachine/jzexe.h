
/* $Id: jzexe.h,v 1.1.1.1 2000/05/10 14:21:34 jholder Exp $
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information
 * --------------------------------------------------------------------
 *
 * File name: $Id: jzexe.h,v 1.1.1.1 2000/05/10 14:21:34 jholder Exp $
 *
 * Description:
 *
 * Modification history:
 * $Log: jzexe.h,v $
 * Revision 1.1.1.1  2000/05/10 14:21:34  jholder
 *
 * imported
 *
 *
 * --------------------------------------------------------------------
 */

/*
 * jzexe.h
 * 
 * Definitions of the "magic" string and flags used by JZip and JZexe to 
 * create standalone games.
 *
 * Written by Magnus Olsson (mol@df.lth.se), 19 November, 1995,
 * as part of the JZip distribution. 
 * You may use this code in any way you like as long as it is
 * attributed to its author.
 *
 */

/* *JWK* Altered by John W. Kennedy */

/* *JWK* signed/unsigned char problems cleaned up */

/* 2000-02-29 */

#ifndef JZEXE_H
#define JZEXE_H

/* 
 * The following string is used to identify the patch area in the JZip
 * interpreter. The patch area itself is the four '~' characters at 
 * the end; these are part of the string to assure that the memory
 * locations are contiguous under all compilers.
 *
 * The algorithms used require that
 * a) The first character of the string must be unique.
 * b) The string must appear only once within the MSDOS JZip executable.
 * c) The string ends with a patch area of four non-zero bytes.
 * As long as these requirements are fulfilled, the string is arbitrary.
 */
#define MAGIC_STRING "JZip/magic (mol)~~~~"


/* 
 * MAGIC_END is the length of the magic string proper, i.e. 
 * strlen(MAGIC_STRING) - 4.
 */
#define MAGIC_END 16

/* 
 * The first byte of the patch area is used as a flag to tell if the
 * interpreter is part of a standalone game. When creating a standalone
 * game, JZexe patches this byte to a zero. 
 * Hence, the macro STANDALONE_FLAG will be TRUE for a standalone game,
 * and FALSE for the ordinary JZip interpreter.
 */
#define STANDALONE_FLAG (!magic[MAGIC_END])

/*
 * The magic string and patch area are stored in a global, initialized
 * string variable. All C compilers I know of will store this string
 * somewhere in the executable, so JZexe can patch it. If you have
 * an advanced C compiler that doesn't, please contact me (mol@df.lth.se).
 */
extern char *magic;             

/*
 * For ANSI C, a prototype for a function defined in fileio.c.
 */
int analyze_exefile( void );

#endif
