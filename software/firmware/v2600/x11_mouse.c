/* The keyboard and mouse interface. */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Intrinsic.h>
#include <stdio.h>

#include "config.h"
#include "options.h"
#include "kmap.h"

/* The main Xt application context */
extern XtAppContext app_context;

extern int moudrv_x, moudrv_y, moudrv_but;

/* Used to read mouse. Not needed under X */
void
moudrv_read(void)
{
}

/* Mouse motion for paddle controllers */
static void
vcsMotionAction (Widget w, XEvent * ev, String * params, Cardinal * num_params)
{
  XMotionEvent *mev = (XMotionEvent *) ev;
  /* printf ("Motion to %d\n", mev->x); */
  moudrv_x = mev->x;
  moudrv_y = mev->y;
}

/* Ckeck mouse button for paddle etc. */
static void
vcsButtonAction (Widget w, XEvent * ev, String * params, Cardinal * num_params)
{
  XButtonEvent *bev = (XButtonEvent *) ev;
  moudrv_x = bev->x;
  moudrv_y = bev->y;
  if (bev->type == ButtonPress)
    moudrv_but = 1;
  else
    moudrv_but = 0;
}

void
moudrv_update(void)
{
}

void
moudrv_close(void)
{
}


/* Used to initialise mouse. Not needed under X */
void
moudrv_init(void)
{
  /* Define the Xt Actions */
  XtActionsRec actions[] =
  {
    {"VcsButton", (XtActionProc) vcsButtonAction},
    {"VcsMotion", (XtActionProc) vcsMotionAction}
  };

  /* Add the Actions */
  XtAppAddActions (app_context, actions, 2);
}





