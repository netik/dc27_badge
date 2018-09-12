/*****************************************************************************

   This file is part of Virtual 2600, the Atari 2600 Emulator
   ==========================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: x11_disp.c,v 1.13 1998/03/15 01:51:00 ahornby Exp ahornby $
******************************************************************************/
#include "config.h"
#include "version.h"

/* The X11 display code. */

/* Standard Includes */
#include <stdio.h>
#include <stdlib.h>

/* X11 stuff */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>

#if HAVE_LIBXXF86VM
#include "vidmode.h"
#endif

#if HAVE_LIBXM

#include <Xm/MainW.h>
#include <Xm/DrawingA.h>
#endif

#if HAVE_LIBXAW && !HAVE_LIBXM
#include "Canvas.h"

/* String definitions */
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Cardinals.h>

/* Text Widgets */
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Text.h>

/* Splits the window into menu bar and game area */
#include <X11/Xaw/Paned.h>
#endif

/* My headers */
#include "types.h"
#include "vmachine.h"
#include "address.h"
#include "files.h"
#include "colours.h"
#include "keyboard.h"
#include "limiter.h"
#include "ui.h"
#include "options.h"
#include "dbg_mess.h"
#include "wmdelete.h"

/* MIT Shared Memory Extension */
#ifdef HAVE_XSHMATTACH
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
XShmSegmentInfo SHMInfo;
#endif
int UseSHM;

/* Useful X related variables */
Screen *screen;
Display *display;
Visual *visual;
int tv_depth;
static int pix_depth; 
int tv_bytes_pp;
unsigned long tv_red_mask;
unsigned long tv_blue_mask;
unsigned long tv_green_mask;
Window root, tv_win, top_win;
XImage *ximage;
GC defaultGC, gc;
int screen_num;
unsigned long white, black;

Colormap tv_colormap;
#define NUMCOLS 128
XColor colors[NUMCOLS];

/* Various variables and short functions */

/* Start of image data */
BYTE *vscreen;

/* The width and height of the image, including any magnification */
int vwidth, vheight, theight;

/* The frame rate counter. Used by the -rr switch */
int tv_counter = 0;

/* The wide pixel translation */

static unsigned int wide_pixel[256];

/* Optionally set by the X debugger. */
int redraw_flag = 1;

/* Xt definitions */
Widget toplevel, canvas;
XtAppContext app_context;

/* The application resource data is held here. */
struct App_data
{
  Bool mitshm;
  Bool private;
}
app_data;


/* Define the resource types for my own resources. */

/* Useful helper macro */
#define offset(field) XtOffsetOf(struct App_data,field)

/* The Xt resource structure. */
static XtResource resources[] =
{
  {"mitshm", "Mitshm", XtRBool, sizeof (Bool), offset (mitshm),
   XtRString, "true"},
  {"private", "Private", XtRBool, sizeof (Bool), offset (private),
   XtRString, "false"},
};


/* Fallback values for both my own resources and any needed by widgets etc. */
static char *fallback[] =
{    
#ifdef 0
  "Debug*fileButton.label:      File",
  "Debug*optionButton.label:    Options",

  "Debug*stop.Label:            Stop",	 /* Cont - Stop - Stop  */
  "Debug*trace.Label:           Trace",	 /* Step - Run  - Trace */
  "Debug*apply.Label:            Break", /* Next - Next - Break */

  "Debug*stop.labelString:      Stop",	/* Cont - Stop - Stop  */
  "Debug*trace.labelString:     Trace",		/* Step - Run  - Trace */
  "Debug*apply.labelString:      Break",		/* Next - Next - Break */
  "Debug*raster.labelString:    Raster",	/* Next - Next - Break */
  "Debug*stop.width:            135",
  "Debug*trace.width:           135",
  "Debug*next.width:            135",
  "Debug*view.allowVert:        True",
  "Debug*view.forceBars:        True",
  "Debug*view.width:            200",
  "Debug*view.height:           320",
  "Debug*hbox.orientation:      Horizontal",
  "Debug*hbox.borderWidth:      0",
  "Debug*hbox.min:              28",
  "Debug*hbox.max:              28",
  "Debug*vbox.orientation:      Vertical",
  "Debug*vbox.borderWidth:      0",	/* Registers and buttons */
  "Debug*smeBSB*borderWidth:    1",
  "Debug*Label*borderWidth:     0",	/* Register name boxes */
  "Debug*bar.orientation:       Vertical",
  "Debug*bar.length:            320",
  "Debug*bar.shown:             0.02",

  "Debug*foreground:	     blue",
  "Debug*background:	     light blue",
  "Debug*Label.foreground       black",
#endif
  NULL
};

/* Define any explict command line options */
static XrmOptionDescRec *options = NULL;

/* Inline helper to place the tv image */
inline void
put_image (void)
{
#ifdef HAVE_XSHMATTACH
  if (UseSHM)
    XShmPutImage (display, tv_win, defaultGC, ximage,
		  0, 0, 0, 0, vwidth, vheight, False);
  else
    XPutImage (display, tv_win, defaultGC, ximage,
	       0, 0, 0, 0, vwidth, vheight);
#else
  XPutImage (display, tv_win, defaultGC, ximage,
	     0, 0, 0, 0, vwidth, vheight);
#endif
}

/* Handle expose events simply */
/* w: Xt widget */
/* event: Event that triggered the expose  */
/* exposed: The area that needs redrawing */
/* client_data: not used */
/* note: this works for Motif, but the parameter list is wrong,
 * so, for motif, don't use the parameters -- hacky
 */ 


#if HAVE_LIBXM
void
Canvas_Expose (Widget w, XtPointer client_data, 
	       XtPointer /*XmDrawingAreaCallbackStruct*/ call_data)
#else
void
Canvas_Expose (Widget w, XEvent event,
	       Region exposed, void *client_data)
#endif
{
  put_image ();
  XFlush (display);
}

Widget game_pane;

/* Set up the Xt widgets */
void
setup_widgets (void)
{  
#if HAVE_LIBXM
  /* using Motif */
  game_pane = XtCreateManagedWidget("main_window", xmMainWindowWidgetClass,
				     toplevel, NULL, 0);
  fancy_widgets (game_pane);
  canvas = XtVaCreateManagedWidget("canvas", xmDrawingAreaWidgetClass,
				   game_pane,XtNheight, vheight,
				   XtNwidth, vwidth,
				   0);

  XtAddCallback(canvas, XmNexposeCallback, Canvas_Expose, &canvas);
#else
  /* Using non-Motif */
  game_pane = XtCreateManagedWidget ("game_pane", panedWidgetClass,
                                    toplevel, NULL, 0);

  canvas = XtVaCreateManagedWidget
    ((String) "canvas", xfwfcanvasWidgetClass, game_pane,
     (String) "exposeProc", (XtArgVal) Canvas_Expose,
     (String) "exposeProcData", (XtArgVal) (int) NULL,
     (String) XtNheight, (XtArgVal) vheight,
     (String) XtNwidth, (XtArgVal) vwidth,
     0);
#endif 

}


/* Turn on auto repeat when the window is entered */
/* None of the parameters are used. They are necessary for Xt */
void
process_enter (Widget w, XtPointer client_data, XEvent * ev, Boolean * disp)
{
  XAutoRepeatOff (display);
}


/* Turn off auto repeat when the window is left */
/* None of the parameters are used. They are necessary for Xt */
void
process_leave (Widget w, XtPointer client_data, XEvent * ev, Boolean * disp)
{
  XAutoRepeatOn (display);
}


void
process_config(Widget w, XtPointer client_data, XEvent * ev, Boolean * disp)
{
  static int create_image(void);
  static void destroy_image(void);
  XResizeRequestEvent *cev=(XResizeRequestEvent *)ev;
  int magstep=0;

  if(cev->width> 660)
    magstep=3;
  else if(cev->width> 340)
    magstep=2;
  else 
    magstep=1;

  if(magstep==base_opts.magstep) 
    return;
  
  /* Kill the old XImage */
  destroy_image();

  /* Calculate the video width and height */
  base_opts.magstep=magstep;
  vwidth = base_opts.magstep * tv_width * 2;
  theight = (tv_height + tv_overscan);
  vheight = base_opts.magstep * (theight);

  /* Set the canvas width */
  dbg_message(DBG_NORMAL, "New magstep=%d\n",magstep);
  XtVaSetValues(canvas,XtNwidth, vwidth, XtNheight, vheight, NULL);
  create_image();
}


/* Initialise parts of the colors array */
static void
init_colors (void)
{
  int i;
  /* Initialise parts of the colors array */
  for (i = 0; i < NUMCOLS; i++)
    {
      /* Make sure red, green and blue get set */
      colors[i].flags = DoRed | DoGreen | DoBlue;

      /* Use the color values from the color table */
      colors[i].red = (colortable[i * 2] & 0xff0000) >> 8;
      colors[i].green = (colortable[i * 2] & 0x00ff00);
      colors[i].blue = (colortable[i * 2] & 0x0000ff) << 8;
    }
}


/* Create the color map of Atari colors */
static void
create_cmap_pseudo256 (void)
{
  int i;

  /* Try using the shared color map. Difficult if netscape is running */
  if (!(app_data.private))
    {
      /* Get the current color map */
      tv_colormap = DefaultColormap (display, DefaultScreen (display));

      /* Try and allocated the colors needed */
      for (i = 0; i < NUMCOLS; i++)
	{
	  if (XAllocColor (display, tv_colormap, &colors[i]) == 0)
	    {
	      /* There weren't enough colors, so switch to private */
	      fprintf (stderr,
		       "Not enough color cells at color %d,using private\n",
		       i);
	      app_data.private = 1;
	      break;
	    }
	}
    }

  /* If we're usind private color cells. */
  if (app_data.private)
    {
      /* Initialise parts of the colors array */
      for (i = 0; i < NUMCOLS; i++)
	colors[i].pixel = i;

      /* Create my new color map , Need to add check for visual type */
      tv_colormap = XCreateColormap (display, root,
				     DefaultVisual (display, screen_num),
				     AllocAll);

      /* Store the updated colormap */
      XStoreColors (display, tv_colormap, colors, NUMCOLS);
    }
}

/* Create the color map of Atari colors on 16 bit colour displays*/
static void
create_cmap_true (void)
{
  int i;

  /* Get the current color map */
  tv_colormap = DefaultColormap (display, DefaultScreen (display));
  
  /* Allocated the colors needed */
  for (i = 0; i < NUMCOLS; i++)
    {
      if (XAllocColor (display, tv_colormap, &colors[i]) == 0)
	{
	  /* There weren't enough colors */
	  fprintf (stderr,
		   "Bloody surprising lack of colors!\n");
	  break;
	}
    }
}


/* Create the color map of Atari colors */
static void
create_cmap(void)
{
  /* Fill the color array */
  init_colors();

  /* Do the business */
  switch ( tv_depth )
    {
    case 8:
      create_cmap_pseudo256 ();
      break;
    case 16:
      create_cmap_true ();
      break;
    case 32:
      create_cmap_true ();
      break;
    }
}

/* Choose an appropriate visual */
static void
choose_visual ( void )
{
  XVisualInfo vis_info;
  if(XMatchVisualInfo( display, screen_num, 8, PseudoColor, &vis_info))
    {
      /* Got the easiest type of visual, so use it! */    
      dbg_message(DBG_NORMAL, "8 bit pseudo color\n" );
    }
  else if(XMatchVisualInfo( display, screen_num, 16, TrueColor, &vis_info))
    {
      /* This is slightly worse! */
      dbg_message(DBG_NORMAL, "16 bit true color\n" );
    }
  else if(XMatchVisualInfo( display, screen_num, 24, TrueColor, &vis_info))
    {
      /* Again, pretty bad! */
      dbg_message(DBG_NORMAL, "24 bit true color\n" );
    }
  else
    {
      fprintf(stderr, "Cannot get a compatible visual!");
      exit(-1);
    }

  visual = vis_info.visual;

  /* Get the tv_depth of the screen */
  pix_depth = vis_info.depth ;
  if(pix_depth==24)
    tv_depth=32;
  else
    tv_depth=pix_depth;
  tv_bytes_pp = tv_depth / 8 ;
  tv_red_mask = vis_info.red_mask ;
  tv_blue_mask = vis_info.blue_mask ;
  tv_green_mask = vis_info.green_mask ;
  
  dbg_message(DBG_NORMAL, "Screen bit depth is %d\n", tv_depth);
}

/* Create the main window */
/* argc: argument count */
/* argv: argument text */
static void
create_window (int argc, char **argv)
{
  /* Calculate the video width and height */
  vwidth = base_opts.magstep * tv_width * 2;
  theight = (tv_height + tv_overscan);
  vheight = base_opts.magstep * (theight);


  /* Create the top level widget */
  dbg_message(DBG_NORMAL, "Creating toplevel...");

  toplevel = XtVaAppInitialize(&app_context, "V2600", options, 
			      XtNumber (options),
			      &argc, argv,
			      fallback,
			      XtNwidthInc, tv_width*2,
			      0);
  /* Get the commonly used X variables */
  display = XtDisplay (toplevel);
  screen = XtScreen (toplevel);
  screen_num = DefaultScreen (display);
  root = DefaultRootWindow (display);

  /* Set up the mode switching */
#if HAVE_LIBXXF86VM
  modeswitch_on();
#endif
  
  dbg_message(DBG_NORMAL, "OK\n");

  /* Read in the resource file */
  dbg_message(DBG_NORMAL,"Reading resources...");
  XtGetApplicationResources (toplevel, &app_data, resources,
			     XtNumber (resources), NULL, 0);
  dbg_message(DBG_NORMAL,"OK\n");


  /* Call the widget set up */
  dbg_message(DBG_NORMAL,"Setting up widgets...");
  setup_widgets ();
  dbg_message(DBG_NORMAL,"OK\n");

  init_keyboard();
  
  /* Get the visual */
  choose_visual();

  /* Create the color map */
  create_cmap();

  /* Realize toplevel. This actually places the window on screen */
  XtRealizeWidget(toplevel);

  /* Wait for the window to appear */
  while(!XtIsRealized(canvas));

  /* Active the WM_DELETE facility used by window managers. */
  add_delete(toplevel, NULL);

  /* Get the toplevel window */
  top_win = XtWindow (toplevel);


  /* If we're using a private color map, activate it now that we have 
     top_win */
  if (app_data.private)
    XSetWindowColormap (display, top_win, tv_colormap);

  /* Get the tv window from the canvas widget */
  tv_win = XtWindow(canvas);
  
}


static int
create_image(void)
{
  /* Create the XImage */
#ifdef HAVE_XSHMATTACH
  /* First try to create a shared image as it is faster. */
  /* NOTE: Only works if client and server share the same machine */
  if (UseSHM)
    {
      dbg_message(DBG_NORMAL,"OK\n  Using shared memory:\n    Creating image...");
      
      ximage = XShmCreateImage (
				display, visual, pix_depth,
				ZPixmap, NULL, &SHMInfo, vwidth, vheight
				);

      if (!ximage)
	{
	    fprintf (stderr, "FAILED using MIT shared memory, try setting V2600*mitshm: false\n");
	  return (0);
	}

      dbg_message(DBG_NORMAL,"OK\n    Getting SHM info...");

      SHMInfo.shmid = shmget (
		       IPC_PRIVATE, ximage->bytes_per_line * ximage->height,
			       IPC_CREAT | 0777
	);

      if (SHMInfo.shmid < 0)
	{
	  if (Verbose)
	    fprintf (stderr, "FAILED getting SHMInfo\n");
	  return (0);
	}

      dbg_message(DBG_NORMAL,"OK\n    Allocating SHM...");
      vscreen = (BYTE *) (ximage->data = SHMInfo.shmaddr =
			  shmat (SHMInfo.shmid, 0, 0));
      if (!vscreen)
	{
	  if (Verbose)
	    printf ("FAILED\n");
	  return (0);
	}

      SHMInfo.readOnly = False;
      dbg_message(DBG_NORMAL, "OK\n    Attaching SHM...");
      dbg_message(DBG_NORMAL, "OK\n    SHM bpl=%d bpp=%d...", ximage->bytes_per_line, ximage->bits_per_pixel  );
      
      if (!XShmAttach (display, &SHMInfo))
	{
	  if (Verbose)
	    fprintf (stderr, "FAILED attaching shared memory\n");
	  return (0);
	}
    }
  else
#endif
    {
      /* If we can't use shared memory because we are running over the */
      /* network then use a bog standard ximage */
      dbg_message(DBG_NORMAL, "OK\n  Allocating screen buffer...");

      vscreen = (BYTE *) malloc (sizeof (BYTE)*(tv_depth/8) *
				 vheight * vwidth);

      if (!vscreen)
	{
	  fprintf (stderr, "FAILED to allocate screen buffer\n");
	  return (0);
	}


      dbg_message(DBG_NORMAL, "OK\n  Creating image...");
      ximage = XCreateImage (
			     display, visual, tv_depth,
			     ZPixmap, 0, vscreen, vwidth, vheight, tv_depth, 0
			     );
      
      if (!ximage)
	{
	  fprintf (stderr, "FAILED to create ximage.\n");
	  return (0);
	}
    }
  return(0);
}


void
create_wide_pixel(void)
{
  int i;

  switch (tv_depth)
    {
    case 8:
      {	
	for(i=0;i<256;i++)
	  {
	    BYTE one_pixel;
	    if (app_data.private)
	      one_pixel=( (i & 0xff)  >> 1);
	    else
	      one_pixel=colors[( (i & 0xff) >> 1)].pixel;
	    wide_pixel[i]=(one_pixel << 24) | (one_pixel << 16) \
	      | (one_pixel << 8) | one_pixel;
	  }
	break;
      }
    case 16:
      {
	for(i=0;i<256;i++)
	{
	    unsigned short one_pixel = colors[( (i & 0xff) >> 1)].pixel;
	    wide_pixel[i]= ( one_pixel << 16) | one_pixel;
	  }
	break;
      }
    case 32:
      {
	for(i=0;i<256;i++)
	  {
	    unsigned int one_pixel = colors[( (i & 0xff) >> 1)].pixel;
	    wide_pixel[i]= one_pixel;
	  }
	break;
      }
    }
}


/* Turns on the television. */
/* argc: argument count */
/* argv: argument text */
/* returns: 1 for success, 0 for failure */
int
tv_on (int argc, char **argv)
{
  /* The GC drawing can be speeded up by turning off graphics exposures */
  /* NOT the same as not generating expose events! */
  XGCValues values;
  values.graphics_exposures = 0;

  /* Create the windows */
  create_window (argc, argv);

  /* Choose ximage type */
  if(base_opts.mitshm==0)
    app_data.mitshm=0;
  UseSHM = app_data.mitshm;
  dbg_message(DBG_NORMAL, "MIT %d\n", UseSHM);

  /* Get the basic colors */
  white = XWhitePixel (display, screen_num);
  black = XBlackPixel (display, screen_num);
  defaultGC = XDefaultGC (display, screen_num);

  /* Create a GC for the ximage copying */
  gc = XCreateGC (display, tv_win, GCGraphicsExposures, &values);
  XSetForeground (display, gc, white);
  XSetBackground (display, gc, black);

  /* Clear the tv screen */
  XClearWindow (display, tv_win);

  /* Turn off auto repeat. Allows for joystick like movement. */
  XAutoRepeatOff (display);

  /* Add event handlers to keep the keyboard sane */

  XtAddEventHandler (canvas, EnterWindowMask, 0, process_enter, NULL);
  XtAddEventHandler (canvas, LeaveWindowMask, 0, process_leave, NULL);

  create_image();

  create_wide_pixel();

  XtAddEventHandler (toplevel, ResizeRedirectMask, 0, process_config, NULL);
  
  dbg_message( DBG_NORMAL, "OK\n");

  return (1);
}

static void
destroy_image(void)
{
#ifdef HAVE_XSHMATTACH
      if (app_data.mitshm)
	{
	  XShmDetach (display, &SHMInfo);
	  if (SHMInfo.shmaddr)
	    shmdt (SHMInfo.shmaddr);
	  if (SHMInfo.shmid >= 0)
	    shmctl (SHMInfo.shmid, IPC_RMID, 0);
	}
      else
#endif /* HAVE_XSHMATTACH */
      if (ximage)
	XDestroyImage (ximage);
}

/* Turn off the tv. Closes the X connection and frees the shared memory */
void
tv_off (void)
{
  dbg_message(DBG_USER, "Switching off display...\n");

  if (display && tv_win)
    {
      destroy_image();
      /* Restore the keyboard to normal */
      XAutoRepeatOn (display);
    }

#if HAVE_LIBXXF86VM
  modeswitch_off();
#endif
  
  if (display)
    XtCloseDisplay (XtDisplay (toplevel));
}


/* Translates a 2600 color value into an X11 pixel value */
/* b: byte containing 2600 colour value */
/* returns: X11 pixel value */
unsigned int
tv_color (BYTE b)
{
  return wide_pixel[b];
}

/* Displays the tv screen */
void
tv_display (void)
{
  /* Only display if the frame is a valid one. */
  if (tv_counter % base_opts.rr == 0)
    {
      dbg_message(DBG_LOTS, "displaying...\n");
      put_image ();
      XFlush (display);
    }
  tv_counter++;
}

/* The Xt Event code. */
void
tv_event (void)
{
  XEvent ev;
  if (XtAppPending (app_context))
    {
      XtAppNextEvent (app_context, &ev);
      XtDispatchEvent (&ev);
    }
}


/* Single pixel plotting function. Used for debugging,  */
/* not in production code  */
/* x: horizontal position */
/* y: vertical position */
/* value: pixel value */
void
tv_putpixel (int x, int y, BYTE value)
{
  BYTE *p;

  switch (base_opts.magstep)
    {
    case 1:
      x = x << 1;
      p = vscreen + x + y * vwidth;
      *(p) = value;
      *(p + 1) = value;
    }
}





