








/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: wmdelete.c,v 1.6 1996/12/03 22:35:05 ahornby Exp ahornby $
******************************************************************************/

/* Adds support for the delete button supplied by window managers. */
/* If I was using Motif this would be handled by the shell */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#if HAVE_LIBXM
#include <Xm/Xm.h>
#include <Xm/Protocols.h>
#endif

/* Kill the application */
/* No parameters used */
static void
destroyDelete (Widget w, XtPointer client_data, XEvent * ev, Boolean * disp)
{
  exit (0);
}

/* Just remove the window */
/* w: Xt widget to remove */
void
unrealizeDelete (Widget w, XtPointer client_data, XEvent * ev, Boolean * disp)
{
  XtUnrealizeWidget (w);
}


/* maybe this should be moved to x11_disp.c or motif_disp.c */

/* Adds an event handler to deal with WM_DELETE messages to shells */
/* w: shell widget to add handler to */
/* HandleMessages: the C function handler. Can be NULL to use default */
void
add_delete (Widget w, XtEventHandler HandleMessages)
{

#ifndef HAVE_MOTIF

  Atom _XA_WM_PROTOCOLS;
  Atom protocols[1];
  return;
  _XA_WM_PROTOCOLS = XInternAtom (XtDisplay (w), "WM_PROTOCOLS", False);
  protocols[0] = XInternAtom (XtDisplay (w), "WM_DELETE_WINDOW", False);
  XSetWMProtocols (XtDisplay (w),
		   XtWindow (w),
		   protocols,
		   XtNumber (protocols));
  if (HandleMessages == NULL)
    HandleMessages = destroyDelete;
  XtAddEventHandler (w, NoEventMask, True, HandleMessages, NULL);

#else

  Atom  WM_DELETE_WINDOW;

    /* install event handler for window manager close */
  WM_DELETE_WINDOW = XmInternAtom(XtDisplay(w), "WM_DELETE_WINDOW", False);
  if (HandleMessages == NULL)
      HandleMessages = destroyDelete;
  XmAddWMProtocolCallback(w, WM_DELETE_WINDOW, HandleMessages, NULL);

#endif

}
