
/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996-98 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for Details.
   
   $Id: motif_ui.c,v 1.3 1998/03/15 01:48:57 ahornby Exp ahornby $
******************************************************************************/

/*
   The fancy lesstif/motif User Interface code.
   Fancy? Well you try writing it!
 */

#include "config.h"
#include "version.h"

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>

#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/FileSB.h>
#include <Xm/Scale.h>
#include <Xm/Separator.h>
#include <Xm/Protocols.h>
#include <Xm/MessageB.h>

/*** Local Includes ***/
#include "types.h"
#include "display.h"
#include "limiter.h"
#include "files.h"
#include "dbg_mess.h"
#include "vidmode.h"

#define XmStringCreateLocalized XmStringCreateSimple

#define MAX_ARGS 20

/* Widgets from display.c */
extern Widget canvas, toplevel;


/* Widgets needed in callbacks etc */
Widget filesel, fileload, rateShell, skipShell, skipScroll,
    optionRate, optionSkip, achievedRate, setRate,
    aboutDlg, fileNotFoundDlg;

/* The current filename */
char fselbuffer[256];

/*** Callback Routines ***/

/* Opens debugger window */
/* w: Xt widget callback triggered from */
/* clientData: not used */
/* callData: not used */
static void
DebugCb (Widget w, XtPointer clientData, XtPointer callData)
{
  XtCallActionProc (canvas, "debugRealize", NULL, NULL, 0);
}

/* Exits x2600 */
/* w: Xt widget callback triggered from */
/* clientData: not used */
/* callData: not used */
static void
QuitCb (Widget w, XtPointer clientData, XtPointer callData)
{
  exit (0);
}

/* Updates frame rate */
void
rate_update (void)
{
  int temp;
  char buf[20];
  XmString str;

  temp = limiter_getFrameRate ();
  sprintf (buf, "%d", temp);

  str = XmStringCreateLocalized(buf);
  XtVaSetValues (achievedRate, 
		 XmNlabelString, str, 
		 NULL);
}

/* Updates fps */
/* w: Xt widget callback triggered from */
/* clientData: not used */
/* callData: not used */
static void
UpdateRateButtonCb (Widget w, XtPointer clientData, XtPointer callData)
{
  rate_update ();
}

/* Pops up the rate shell */
/* w: Xt widget callback triggered from */
/* clientData: not used */
/* callData: not used */
static void
RateCb (Widget w, XtPointer clientData, XtPointer callData)
{
  int temp;
  extern int tv_hertz;
  char buf[20];

  XtManageChild(rateShell);
  rate_update ();
  temp = tv_hertz;
  sprintf (buf, "%d", temp);
  XtVaSetValues (setRate, XtNlabel, buf, NULL);
}

/* The file selector cancel call back */
/* w: Xt widget callback triggered from */
/* clientData: not used */
/* callData: not used */
static void
fselCancelCb (Widget w, XtPointer clientData, XtPointer callData)
{
  XtUnmanageChild (w);
}

/* The file selector load procedure */
/* returns: 0 on success, -1 on failure */
#include "options.h"
extern struct BaseOptions base_opts;
int
fselLoad (void)
{
    int res;

    strcpy(base_opts.filename, fselbuffer);
    res = loadCart();

    if (res)
	fprintf (stderr, "File %s not found\n", fselbuffer);
    return res;
}

/* The file selector OK callback */
/* w: Xt widget callback triggered from */
/* clientData: not used */
/* callData: not used */
static void
fselOkayCb (Widget w, XtPointer clientData, XtPointer calldata)
{
  extern int reset_flag;
  char *text;
  FILE *ftest;
  
  XmFileSelectionBoxCallbackStruct *fselret = 
      (XmFileSelectionBoxCallbackStruct *) calldata;

  XmStringGetLtoR(fselret->value, XmSTRING_DEFAULT_CHARSET, &text);
  ftest = fopen(text, "r");
  if(ftest) {
      fclose(ftest);
      strcpy (fselbuffer, text);
      XtUnmanageChild(w);
      XtFree(text);
      reset_flag = 1;
  } else {
      XtManageChild(fileNotFoundDlg);      
  }  
}

/* Places the menus in widget parent */
/* parent: widget to contain menus */

static void
file_cb(
    Widget menu_item,
    XtPointer c,
    XtPointer d)
{
    int item_num = (int) c;
    
    switch (item_num)
    {
    case 0:
	XtManageChild(filesel);
	break;
    case 1:
	DebugCb(menu_item, c, d);
	break;
    case 2:
	QuitCb(menu_item, c, d);
	break;
    default:
	printf("why are we here\n");
    }
}

static void
options_cb(
    Widget menu_item,
    XtPointer c,
    XtPointer d)
{
    int item_num = (int) c;
    
    switch (item_num) {
	case 0:
	    XtManageChild(rateShell);
	    break;
	case 1:
	    XtManageChild(skipShell);
	    break; 
    }
}

/* switch to a specified videomode */
static void
vidmode_cb(
    Widget menu_item,
    XtPointer c,
    XtPointer d)
{
    extern Widget toplevel;
    extern Display *display;

    int item_num = (int) c;
    
    modeswitch(item_num);
    modeswitch_flag=c;
    XtMoveWidget(toplevel,0,0);
}

/* Simple message box callback */
static void
mbox_cb( Widget w,
	 XtPointer client_data,
	 XtPointer call_data)
{
    XtUnmanageChild((Widget)client_data);
}

static void
help_cb( Widget menu_item,
	 XtPointer c,
	 XtPointer d)
{
    int item_num = (int) c;
    
    switch (item_num) {
	case 0: 
	    XtManageChild(aboutDlg);
	    break;
    }
}

static void
create_vidmode_menu(Widget parent)
{
    Widget pane, vidmode_button, mode_button;
    Arg args[MAX_ARGS];
    int num_args, i;
    XmString str;
    
    num_args=0;
    pane = XmCreatePulldownMenu(parent,"vidmode_menu",args,num_args);

    for(i=0; i<get_nummodes();i++){
	char bname[80];
	sprintf(bname,"mode_button%d",i);
	str=XmStringCreateLocalized(describe_mode(i));	

	num_args=0;
	XtSetArg(args[num_args], XmNlabelString, str); num_args++; 
	mode_button = XmCreatePushButton(pane, bname,args,num_args);
	XtAddCallback (mode_button, XmNactivateCallback, vidmode_cb, i);
	XtManageChild(mode_button);
	XmStringFree(str);
    }
    
    num_args=0;
    XtSetArg(args[num_args], XmNsubMenuId, pane); num_args++;
    str=XmStringCreateLocalized("VidMode");
    XtSetArg(args[num_args], XmNlabelString, str); num_args++;
    vidmode_button=
	XmCreateCascadeButton(parent, "vidmode_button", args, num_args);
    XtManageChild(vidmode_button);
    XmStringFree(str);	
}

static void
init_menu (Widget parent)
{

  Widget menubar;
  XmString file = XmStringCreateLocalized("File");
  XmString options = XmStringCreateLocalized("Options");
  XmString help = XmStringCreateLocalized("Help");
  XmString load = XmStringCreateLocalized("Load");
  XmString debug = XmStringCreateLocalized("Debug");
  XmString quit = XmStringCreateLocalized("Quit");
  XmString rate = XmStringCreateLocalized("Rate");
  XmString skip = XmStringCreateLocalized("Skip");
  XmString about = XmStringCreateLocalized("About");
  ArgList args[MAX_ARGS];
  Cardinal argcount;
  
  menubar = XmVaCreateSimpleMenuBar(parent, "menu", 
			       XmVaCASCADEBUTTON, file, 'F',
			       XmVaCASCADEBUTTON, options, 'O',
    			       XmVaCASCADEBUTTON, help   , 'H',
			       NULL);

  XmVaCreateSimplePulldownMenu(menubar, "file_menu", 0, file_cb,
			       XmVaPUSHBUTTON, load, 'L', NULL, NULL,
			       XmVaPUSHBUTTON, debug, 'D', NULL, NULL,
			       XmVaPUSHBUTTON, quit, 'Q', NULL, NULL,
			       NULL);


  XmVaCreateSimplePulldownMenu(menubar, "option_menu", 1, options_cb,
			       XmVaPUSHBUTTON, rate, 'R', NULL, NULL,
			       XmVaPUSHBUTTON, skip, 'S', NULL, NULL,
			       NULL);


  XmVaCreateSimplePulldownMenu(menubar, "help_menu", 2, help_cb,
			       XmVaPUSHBUTTON, about, 'A', NULL, NULL,
			       NULL);

  create_vidmode_menu(menubar);

  XtManageChild(menubar);
  XtVaSetValues(parent,
		XmNmenuBar, menubar,
		NULL);

}


/* Initializes the rate shell */
static void
init_rateShell (void)
{
    Widget vbox, label, button, sep;
    XmString str;


    vbox = rateShell = 
	XmCreateFormDialog(toplevel, "rateShell", NULL, 0);

    str = XmStringCreateLocalized("Set     :");
    label = XtVaCreateManagedWidget ("label", xmLabelWidgetClass, vbox,
				     XmNlabelString, str,
				     XmNleftAttachment, XmATTACH_FORM,
				     XmNtopAttachment, XmATTACH_FORM,
				     NULL);
    XmStringFree(str);

    str = XmStringCreateLocalized("0");
    setRate = 
	XtVaCreateManagedWidget ("setRate", xmLabelWidgetClass, vbox,
				 XmNlabelString, str,
				 XmNleftAttachment, XmATTACH_WIDGET,
				 XmNleftWidget, label,
				 XmNtopAttachment, XmATTACH_FORM,
				 NULL);
    XmStringFree(str);

    str = XmStringCreateLocalized("Achieved:");

    label = 
      XtVaCreateManagedWidget ("label", xmLabelWidgetClass, vbox,
			       XmNlabelString, str,
			       XmNleftAttachment, XmATTACH_WIDGET,
			       XmNleftWidget, setRate,
			       XmNtopAttachment, XmATTACH_FORM,
			       NULL);
    XmStringFree(str);

    str = XmStringCreateLocalized("0");
    achievedRate = 
      XtVaCreateManagedWidget ("achievedRate", xmLabelWidgetClass,
			       vbox, 
			       XmNlabelString, str,
			       XmNtopAttachment, XmATTACH_FORM,
			       XmNleftAttachment, XmATTACH_WIDGET,
			       XmNleftWidget, label,
			       NULL);
    XmStringFree(str);

    
    button = 
	XtVaCreateManagedWidget ("close", xmPushButtonWidgetClass, vbox,
				 XmNbottomAttachment, XmATTACH_FORM,
				 XmNleftAttachment, XmATTACH_FORM,
				 NULL);    

    button = XtVaCreateManagedWidget ("update", xmPushButtonWidgetClass, vbox,
				      XmNbottomAttachment, XmATTACH_FORM,
				      XmNrightAttachment, XmATTACH_FORM,
				      NULL);

    XtRemoveAllCallbacks (button, XmNactivateCallback);
    XtAddCallback (button, XmNactivateCallback, UpdateRateButtonCb, NULL);

    sep = 
	XtVaCreateManagedWidget ("close", xmSeparatorWidgetClass, vbox,
				 XmNbottomAttachment, XmATTACH_WIDGET,
				 XmNbottomWidget, button,
				 XmNleftAttachment, XmATTACH_FORM,
				 XmNrightAttachment, XmATTACH_FORM,
				 XmNtopAttachment, XmATTACH_WIDGET,
				 XmNtopWidget, label,
				 NULL); 

}


/* Frame skipping drag bar callback */
static void
skipScrollProc (Widget bar, XtPointer client, XtPointer ptr)
{
	int value, newrate = 0;

	XtVaGetValues(bar,
		XmNvalue, &value,
		NULL);	
	newrate = (value * 7) + 1;
	base_opts.rr = value;
}

/* Initializes the frame skipping shell */
static void
init_skipShell (void)
{
  Widget vbox, label, button;
  XmString str;

  vbox = skipShell = 
	XmCreateFormDialog(toplevel, "skipShell", NULL, 0);

    
  str = XmStringCreateLocalized("1:1");
  /* Create a label for the dialog */
  label = XtVaCreateManagedWidget ("label1", xmLabelWidgetClass, vbox,
				   XmNlabelString, str, 
				   XmNleftAttachment, XmATTACH_FORM,
				   XmNtopAttachment, XmATTACH_FORM,
                                   NULL);
  XmStringFree(str);

  skipScroll = XtVaCreateManagedWidget ("skipScroll", xmScaleWidgetClass,
                                     vbox,
                                     XmNorientation, XmHORIZONTAL,
				     XmNshowValue, True,
				     XmNminimum, 1,
				     XmNmaximum, 8,
				     XmNvalue, 1,
				     XmNleftAttachment, XmATTACH_WIDGET,
				     XmNleftWidget, label,
				     XmNtopAttachment, XmATTACH_FORM,
                                     NULL);

  XtAddCallback (skipScroll, XmNvalueChangedCallback, skipScrollProc, NULL);

  str = XmStringCreateLocalized("1:8");
  /* Create a label for the dialog */
  label = XtVaCreateManagedWidget ("label2", xmLabelWidgetClass, vbox,
				   XmNlabelString, str, 
				   XmNleftAttachment, XmATTACH_WIDGET,
				   XmNleftWidget, skipScroll,
				   XmNtopAttachment, XmATTACH_FORM,
                                   NULL);
  XmStringFree(str);

  button = XtVaCreateManagedWidget ("close", xmPushButtonWidgetClass, vbox,
				     XmNleftAttachment, XmATTACH_FORM,
				     XmNtopAttachment, XmATTACH_WIDGET,
				     XmNtopWidget, skipScroll,
                                    NULL);
}

/* Set up the help/about dialog */
void
init_aboutDlg(Widget parent)
{
    if(aboutDlg==NULL) {
	aboutDlg = XmCreateInformationDialog(parent, "aboutDlg",0   ,  0 );
	XtAddCallback(aboutDlg, XmNokCallback, mbox_cb,
		      (XtPointer)aboutDlg);
	XtDestroyWidget(XmMessageBoxGetChild(aboutDlg,XmDIALOG_HELP_BUTTON));
	XtDestroyWidget(XmMessageBoxGetChild(aboutDlg,XmDIALOG_CANCEL_BUTTON));
    }
}

/* Set up the file not found dialog */
void
init_fileNotFoundDlg(Widget parent)
{
    if(fileNotFoundDlg==NULL) {
	fileNotFoundDlg =
	    XmCreateWarningDialog(parent, "fileNotFoundDlg", 0, 0 );
	XtAddCallback(fileNotFoundDlg, XmNokCallback, mbox_cb,
		      (XtPointer)fileNotFoundDlg);
	XtDestroyWidget(XmMessageBoxGetChild(fileNotFoundDlg,
					     XmDIALOG_HELP_BUTTON));
	XtDestroyWidget(XmMessageBoxGetChild(fileNotFoundDlg,
					     XmDIALOG_CANCEL_BUTTON));
    }
}

/* Main start up routine for GUI */
/* parent: widget to contain menus */
void
fancy_widgets (
    Widget parent)
{
    /* create file selection widget */
    filesel = XmCreateFileSelectionDialog(toplevel, "filesel", NULL, 0);
    XtAddCallback(filesel, XmNcancelCallback, fselCancelCb, (XtPointer) 0);
    XtAddCallback(filesel, XmNokCallback, fselOkayCb, (XtPointer) 0);
    
    init_rateShell();
    init_skipShell();
    init_aboutDlg(toplevel); 
    init_fileNotFoundDlg(toplevel);
    init_menu(parent);
}
