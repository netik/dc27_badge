#include "config.h"
#include "version.h"

/* Standard Includes */
#include <stdio.h>
#include <stdlib.h>

/* XFree86 vidmode stuff */
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#if HAVE_LIBXXF86VM
#include <X11/extensions/xf86vmode.h>
#endif

/* My headers */
#include "dbg_mess.h"
#include "options.h"

int modeswitch_flag=0;

extern Display *display;
extern int screen_num;

static XF86VidModeModeInfo **modelist;
static char modedesc[80];
static int nummodes=0;

static int original_mode=0;
extern Widget toplevel;

void
position_port(int x, int y)
{
    XF86VidModeSetViewPort(display,screen_num,x,y);
}

int
get_nummodes(void)
{
    return nummodes;
}


char *
describe_mode(int m)
{
    sprintf(modedesc,"%dx%d", modelist[m]->hdisplay,
		    modelist[m]->vdisplay);
    return modedesc;
}

static void
get_vidmodes(void)
{
    int i;
    XF86VidModeGetAllModeLines(display,screen_num,&nummodes,&modelist);
    for(i=0; i< nummodes; i++){
	XF86VidModeModeInfo *currmode;
	currmode=modelist[i];
	dbg_message(DBG_NORMAL, "Mode %d %dx%d\n",i,currmode->hdisplay,
		    currmode->vdisplay);
    }    
}

void
modeswitch(int mode)
{
    XF86VidModeSwitchToMode(display, screen_num, modelist[mode]);
}

static void
move_event(Widget w, XtPointer client_data, XEvent * ev, Boolean * disp)
{
    XConfigureEvent *cev=(XResizeRequestEvent *)ev;
    extern Widget game_pane;
    extern Window root;
    int x,y,x_ret,y_ret;
    Window root_ret;
    
    if(modeswitch_flag){
	XWarpPointer(display,None,XtWindow(game_pane),0,0,0,0,0,0);
	XtVaGetValues(game_pane,XtNx,&x,NULL);
	XtVaGetValues(game_pane,XtNy,&y,NULL);
	XTranslateCoordinates(display, XtWindow(game_pane),root,x, 
			      y, &x_ret, &y_ret, &root_ret);
	
	dbg_message(DBG_NORMAL,"vidmode_cb %d %d %d %d\n",x,y, x_ret,y_ret);
	position_port(x_ret,y_ret);
	modeswitch_flag=0;
    }
}

/* Turn on vidmode extension if present */
int
modeswitch_on(void)
{
    int event_base_return, error_base_return;
    int does_vidmode=0;
    dbg_message(DBG_NORMAL, "Checking for vidmode extension\n");
    does_vidmode=XF86VidModeQueryExtension(display,
					   &event_base_return,
					   &error_base_return);
    if(does_vidmode){
	dbg_message(DBG_NORMAL, "Listing video modes\n");
	get_vidmodes();
	XtAddEventHandler(toplevel,
			  StructureNotifyMask, 0, move_event, NULL);
  
    }

    return does_vidmode;
}

void
modeswitch_off(void)
{
    if(modelist)
	free(modelist);
}
