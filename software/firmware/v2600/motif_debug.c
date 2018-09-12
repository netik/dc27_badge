

/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for Details.
   
   $Id: motif_debug.c,v 1.3 1998/02/13 01:37:27 ahornby Exp $
******************************************************************************/

/*
 * $Id: motif_debug.c,v 1.3 1998/02/13 01:37:27 ahornby Exp $
 *
 * This was is part of the x64 Commodore 64 emulator.
 * See README for copyright notice
 *
 * X11-based 65xx tracer/debugger interface.
 * Also the tree of X resources is built here.
 *
 *
 * Originally Written by
 *   Jarkko Sonninen (sonninen@lut.fi)
 *
 */

#ifdef __hpux
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif
#endif


#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>	/* Get standard string definitions. */

#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/FileSB.h>
#include <Xm/Scale.h>
#include <Xm/Separator.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/DialogS.h>

/* From X64
 * And then the same again by hand...
 * If you are unfortunate enough not to have XPointer etc defined by your
 * system, you have to define by hand all types that are missing:
 */

#ifdef X_NOT_STDC_ENV
typedef char *XPointer;
#define XtOffsetOf(s_type,field) XtOffset(s_type*,field)
#endif

#ifndef XtNjumpProc
#define XtNjumpProc "jumpProc"
#endif


#include "cpu.h"
#include "macro.h"
#include "vmachine.h"		/* I_SPECX */
#include "extern.h"
#include "misc.h"
#include "memory.h"
#include "display.h"
#include "config.h"
#include "address.h"
#include "wmdelete.h"
#include "raster.h"

/*
 * Variables
 */

extern XtAppContext app_context;
extern Display *display;
extern Widget canvas, toplevel, game_pane;

/* Widgets added */
Widget dshell, memshell, hshell;
Widget MEMbar, memview;
Widget applyButton, rasterButton, asmtext;

/* Number of hardware registers to display */
#define N_HREG 13
Widget hregW[N_HREG], happlyButton;

/* Debugging flags */
int debugf_on = 0, debugf_halt = 0, debugf_trace = 0, debugf_raster = 0;

/* current window position */
static ADDRESS current_point;
static char buf[256];

enum Register
  {
    R_AC, R_XR, R_YR, R_SP, R_PC, R_SR, R_EFF, R_DL, R_CLK, NO_REGISTERS
  };

static char *registerLabels[] =
{
  "AC:", "XR:", "YR:", "SP:", "PC:", "SR:", "EA>", "DL>", "Clk"
};


enum DState
  {
    DS_Run, DS_Halt, DS_Trace, DS_Step, DS_Break, DS_Mon
  };
enum DState state;

static char *StatusLabels[] =
{
"Running.", "Halted.", "Trace Mode.", "Execute.", "Break.", "Monitor Break."
};


/*
 * Functions
 */

extern void cleanup (void);
extern void debug_main (void);
extern void toggle_cntl_win (void);

static Widget  MenuBar (Widget parent);
static void RegisterBar (Widget parent);

static void UpdateRegisters (void);
static void UpdateHardware (void);	/* New */
static void UpdateStatus (enum DState);
static void UpdateButtons (void);

static void draw_asm (ADDRESS);

/*
 * Widgets
 */

Widget registers[NO_REGISTERS];
Widget stopButton, traceButton, nextButton;
Widget view;
Widget ASMbar;
Widget status1;
Widget shell;

/* ------------------------------------------------------------------------- */

/* Realize the debugger shell */
/* Parameters not used */
void
debugRealizeAction (Widget w, XEvent * ev)
{
  debugf_on = 1;
  XtRealizeWidget (dshell);
  UpdateRegisters ();
  draw_asm (PC);
}

/* Sets raster stepping to ON */
/* Parameters not used */
static void
rasterAction (Widget w, XEvent * ev)
{
  /* Set up a wait for the next raster line to be drawn */
  /* Will require some code in tv_raster */
  debugf_raster = 1;
  debugf_halt = 0;
  UpdateButtons ();
}

/* Unrealize the debugger shell */
/* Parameters not used */
void
debugUnrealizeAction (Widget w, XEvent * ev)
{
  debugf_on = 0;
  XtUnrealizeWidget (dshell);
}

/* The X debugger's part of mainloop() */
void
x_loop (void)
{
  XEvent report;
  if (debugf_halt || debugf_trace)
    {
      UpdateRegisters ();
      UpdateHardware ();
      draw_asm (PC);
    }

  if (redraw_flag)
    {
      tv_display ();
      redraw_flag = 0;
    }

  while (debugf_halt && !debugf_trace)
    {
      if (XtAppPending (app_context))
	{
	  XtAppNextEvent (app_context, &report);
	  XtDispatchEvent (&report);
	}
      state = (debugf_halt ? DS_Halt : debugf_trace ? DS_Trace : DS_Run);
      UpdateStatus (state);
    }

  if (debugf_trace)
    {
      if (debugf_halt)		/* single step excution */
	debugf_trace = 0;
      state = (debugf_halt ? DS_Halt : debugf_trace ? DS_Trace : DS_Run);
      UpdateStatus (state);
      tv_event ();
    }
}

/* Update the hardware window */
static void
UpdateHardware (void)
{
  char buf[20];
  int temp;

  sprintf (buf, "%04d", ebeamx);
  XmTextFieldSetString (hregW[0], buf);
  sprintf (buf, "%04d", ebeamy);
  XmTextFieldSetString (hregW[1], buf);
  sprintf (buf, "%04d", tiaWrite[CTRLPF]);
  XmTextFieldSetString (hregW[2], buf);
  sprintf (buf, "%04d", timer_count);
  XmTextFieldSetString (hregW[3], buf);
  sprintf (buf, "%06d", riotRead[INTIM]);
  XmTextFieldSetString (hregW[4], buf);
  sprintf (buf, "%06d", timer_res);
  XmTextFieldSetString (hregW[5], buf);
  sprintf (buf, "%06d", timer_clks);
  XmTextFieldSetString (hregW[6], buf);
  temp = colour_table[0];
  sprintf (buf, "%06d", temp);
  XmTextFieldSetString (hregW[7], buf);
  temp = colour_table[1];
  sprintf (buf, "%06d", temp);
  XmTextFieldSetString (hregW[8], buf);
  temp = pl[0].grp;
  sprintf (buf, "%02x", temp);
  XmTextFieldSetString (hregW[9], buf);
  temp = pl[1].grp;
  sprintf (buf, "%02x", temp);
  XmTextFieldSetString (hregW[10], buf);
  temp = pl[0].x;
  sprintf (buf, "%03d", temp);
  XmTextFieldSetString (hregW[11], buf);
  temp = pl[1].x;
  sprintf (buf, "%03d", temp);
  XmTextFieldSetString (hregW[12], buf);
}

/* Update the registers in the main debugging window */
static void
UpdateRegisters (void)
{
  int eff;

  sprintf (buf, "$%02X", AC);
  XmTextFieldSetString (registers[R_AC],
		  buf
		 );

  sprintf (buf, "$%02X", XR);
  XmTextFieldSetString (registers[R_XR],
		  buf
		 );

  sprintf (buf, "$%02X", YR);
  XmTextFieldSetString (registers[R_YR],
		  buf
		 );

  sprintf (buf, "$%02X", SP);
  XmTextFieldSetString (registers[R_SP],
		  buf
		 );

  sprintf (buf, "$%04X", PC);
  XmTextFieldSetString (registers[R_PC],
		  buf
		 );

  sprintf (buf, "$%02X", GET_SR ());
  XmTextFieldSetString (registers[R_SR],
		  buf
		 );

  /* Memory */

  eff = eff_address (PC, 1);
  if (eff >= 0)
    sprintf (buf, "$%04X", eff);
  else
    sprintf (buf, "----");

  XmTextFieldSetString (registers[R_EFF],
		  buf
		 );

  if (eff >= 0)
    sprintf (buf, "$%02X", DLOAD (eff));
  else
    sprintf (buf, "--");

  XmTextFieldSetString (registers[R_DL],
		  buf
		 );

  /* Clock */

  sprintf (buf, "%ld", clk);
  XmTextFieldSetString (registers[R_CLK],
		  buf
		 );
}


/* Update the debugger Status */
static void
UpdateStatus (enum DState dst)
{
  static int s = -1;
  XmString xmstr;

  if (s == (int) dst)
    return;

  xmstr = XmStringCreateLocalized(StatusLabels[dst]);
  XtVaSetValues (status1,
		 XmNlabelString, xmstr,
		 NULL);
  XmStringFree(xmstr);
  s = dst;
}


/* Update the button labels */
static void
UpdateButtons (void)
{
  XmString xmstr;
  /*
   * Box widget rezises buttons by default
   */
  xmstr = 
      XmStringCreateLocalized((debugf_halt || debugf_raster) ? 
			      "Continue" : "Stop");
  XtVaSetValues (stopButton,
		 XmNlabelString, xmstr,
		 NULL);
  XmStringFree(xmstr);
  xmstr = 
      XmStringCreateLocalized((debugf_halt || debugf_raster) ? "Step" :
		 debugf_trace ? "Run" : "Trace");
  XtVaSetValues (traceButton,
		 XmNlabelString, xmstr,
		 NULL);
  XmStringFree(xmstr);
}


/* Called upon selecting Close */
/* Parameters not used */
static void
QuitCb (Widget w, XtPointer clientData, XtPointer callData)
{
  XtCallActionProc (canvas, "debugUnrealize", NULL, NULL, 0);
}

/* Used when opening memory pane. */
/* Memory browser not implemented. */
static void
MemoryCb (Widget w, XtPointer clientData, XtPointer callData)
{
  static int memorystate = 0;
  Dimension width, height;
  Dimension swidth, sheight;

  Position x, y;

  if ((memorystate = !memorystate))
    {
      XtVaGetValues (dshell,
		     XtNwidth, &width,
		     XtNheight, &height,
		     NULL);
      XtVaGetValues (memshell,
		     XtNwidth, &swidth,
		     XtNheight, &sheight,
		     NULL);
      XtTranslateCoords (dshell,
			 (Position) 100,
			 (Position) 100,
			 &x, &y);

      XtVaSetValues (memshell, XtNx, x, XtNy, y, NULL);

      XtPopup (memshell, XtGrabNone);
      /*set_mem(PC); */
    }
  else
    {
      XtPopdown (memshell);
    }
}

/* Pops up/down hardware pane */
/* Parameters not used */
static void
HardwareCb (Widget w, XtPointer clientData, XtPointer callData)
{
  static int hardstate = 0;

  if ((hardstate = !hardstate)) {
      XtRealizeWidget (hshell);
      /*set_mem(PC); */
  } else {
      XtUnrealizeWidget (hshell);
  }
}


/* ------------------------------------------------------------------------- */

/* Called when stop button is pressed */
/* Parameters not used */
static void
StopCb (Widget w, XtPointer clientData, XtPointer callData)
{
  debugf_halt = !debugf_halt;
  debugf_raster = 0;

  UpdateButtons ();
  UpdateRegisters ();
  draw_asm (PC);
}

/* Called when stop button is pressed */
/* Parameters not used */
static void
RasterCb (Widget w, XtPointer clientData, XtPointer callData)
{
  /* Set up a wait for the next raster line to be drawn */
  /* Will require some code in tv_raster */
  XtCallActionProc (dshell, "raster", NULL, NULL, 0);
}

/* Set the registers to the values entered */
static void
SetRegisters (void)
{
  Arg arg;
  char *ptr;
  int temp;

/* Commented out due to incompatibility on SUN
   XtSetArg(arg, XtNstring, &ptr);
   XtGetValues (registers [R_AC],&arg, 1);
   sscanf(ptr,"$%x",(unsigned int *)(&AC));

   XtSetArg(arg, XtNstring, &ptr);
   XtGetValues (registers [R_XR],&arg, 1);
   sscanf(ptr,"$%x",(unsigned int *)&XR);

   XtSetArg(arg, XtNstring, &ptr);
   XtGetValues (registers [R_YR],&arg, 1);
   sscanf(ptr,"$%x",(unsigned int *)&YR);

   XtSetArg(arg, XtNstring, &ptr);
   XtGetValues (registers [R_SP],&arg, 1);
   sscanf(ptr,"$%x",(unsigned int *)&SP);

 */

  XtSetArg (arg, XtNstring, &ptr);
  XtGetValues (registers[R_PC], &arg, 1);
  sscanf (ptr, "$%x", &temp);
  PC = temp;
}

/* Set the hardware registers to the values entered */
/* Not implemented, as not needed */
static void
SetHardware (void)
{
  Arg arg;
  char *ptr;

  XtSetArg (arg, XtNstring, &ptr);
  XtGetValues (hregW[0], &arg, 1);
  sscanf (ptr, "%d", (unsigned int *) (&ebeamx));
}

/* Call back for Apply button */
/* Parameters not used */
static void
ApplyCb (Widget w, XtPointer clientData, XtPointer callData)
{
  SetRegisters ();
  draw_asm (PC);
}

/* Call back for Trace button */
/* Parameters not used */
static void
TraceCb (Widget w, XtPointer clientData, XtPointer callData)
{
  debugf_trace = !debugf_trace;
  UpdateButtons ();
  UpdateRegisters ();
  draw_asm (PC);
}

/* ------------------------------------------------------------------------- */

/*
 * Assembly Window
 */



/* Fill in the assembler window */
/* p: address for start of window */
void
draw_asm (ADDRESS p)
{
  char buf2[8192];
  short lines=0, height;		/* NOT ints */
  int i;
  int plen, len, start, finish;

  buf2[0] = '\0';
                        /* show 10 previous addresses */
  p = p - 20;		/* fixme: does this really work? */

  XtVaGetValues (asmtext,
		 XmNrows, &lines,
		 NULL);
  if (lines > 1024) lines = 1024;

  len = 0;
  for (i = 0; i <= lines*2; i++)
    {
      p &= 0xffff;
      len += sprintf (buf, "%04X %s  %-s\n", p,
	       sprint_ophex (p), sprint_opcode (p, 1));
      strcat (buf2, buf);
      p += clength[lookup[DLOAD (p)].addr_mode];
      if (i == 9) {
         /* this is where we save the selection */
         finish = len;
         start = plen;
      } else {
         plen = len;
      }
    }

  XmTextSetString (asmtext, buf2);
  XmTextSetHighlight(asmtext, start, finish, XmHIGHLIGHT_SELECTED);
}


/* ------------------------------------------------------------------------- */

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
	QuitCb(menu_item, c, d);
	break;
    default:
	printf("why are we here?\n");
    }
}


static void
tools_cb(
    Widget menu_item,
    XtPointer c,
    XtPointer d)
{
    int item_num = (int) c;
    
    switch (item_num)
    {
    case 0:
	MemoryCb(menu_item, c, d);
	break;
    case 1:
	HardwareCb(menu_item, c, d);
	break;
    default:
	printf("why are we here?\n");
    }
}


/* Create the menu bar */
/* parent: widget to contain menu bar */
static Widget
MenuBar (Widget parent)
{
  Widget menubar;
  XmString file = XmStringCreateLocalized("File");
  XmString tools = XmStringCreateLocalized("Tools");
  XmString quit = XmStringCreateLocalized("Quit");
  XmString memory = XmStringCreateLocalized("Memory");
  XmString hardware = XmStringCreateLocalized("Hardware");
  
  menubar = XmVaCreateSimpleMenuBar(
      parent, "menu", 
      XmVaCASCADEBUTTON, file, 'F',
      XmVaCASCADEBUTTON, tools, 'T',
      XmNtopAttachment, XmATTACH_FORM,
      XmNleftAttachment, XmATTACH_FORM,
      XmNrightAttachment, XmATTACH_FORM,
      NULL);
  
  XmVaCreateSimplePulldownMenu(menubar, "file_menu", 0, file_cb,
			       XmVaPUSHBUTTON, quit, 'Q', NULL, NULL,
			       NULL);

  XmVaCreateSimplePulldownMenu(menubar, "tools_menu", 1, tools_cb,
			       XmVaPUSHBUTTON, memory, 'M', NULL, NULL,
			       XmVaPUSHBUTTON, hardware, 'H', NULL, NULL,
			       NULL);
  XtManageChild(menubar);
  XtVaSetValues(parent,
		XmNmenuBar, menubar,
		NULL);
  return menubar;
}


/* Creates the register boxes */
/* parent: rowcolumn widget to contain register boxes */
static void
RegisterBar (Widget parent)
{
  int i;
  Widget label, lastlabel, subform;
  XmString xmstr;

  /*
   * Registers
   */

  for (lastlabel = NULL, i = 0; i < NO_REGISTERS; i++, lastlabel = label)
    {
      xmstr = XmStringCreateLocalized(registerLabels[i]);
      subform = XtVaCreateManagedWidget(
	  "subform", xmFormWidgetClass, parent,
	  XmNtopAttachment, lastlabel ? XmATTACH_WIDGET : XmATTACH_FORM,
	  XmNtopWidget, subform,
	  XmNleftAttachment, XmATTACH_FORM,
	  XmNrightAttachment, XmATTACH_FORM,
	  NULL);
      label =  XtVaCreateManagedWidget (
	  "register", xmLabelWidgetClass, subform,
	  XmNlabelString, xmstr,
	  XmNleftAttachment, XmATTACH_FORM,
	  XmNrightAttachment, XmATTACH_POSITION,
	  XmNrightPosition, 20,
	  XmNbottomAttachment, XmATTACH_FORM,
	  XmNtopAttachment, XmATTACH_FORM,
	  NULL);

      registers[i] = 
	  XtVaCreateManagedWidget ("value", xmTextFieldWidgetClass, subform,
				   XmNcolumns, 8,
				   XmNleftAttachment, XmATTACH_WIDGET,
				   XmNleftWidget, label,
				   XmNrightAttachment, XmATTACH_FORM,
				   XmNtopAttachment, XmATTACH_FORM,
				   XmNbottomAttachment, XmATTACH_FORM,
				   NULL);
      XmStringFree(xmstr);
    }

  /*
   * Debugger Status
   */


  xmstr = XmStringCreateLocalized(StatusLabels[DS_Run]);
  status1 = XtVaCreateManagedWidget ("status", xmLabelWidgetClass, parent,
				     XmNlabelString, xmstr,
				     XmNtopAttachment, XmATTACH_WIDGET,
				     XmNtopWidget, subform,
				     XmNleftAttachment, XmATTACH_FORM,
				     XmNleftOffset, 20,
				     NULL);

  /*
   * Buttons
   */

  stopButton = 
      XtVaCreateManagedWidget (
	  "stop", xmPushButtonWidgetClass, parent,
	  XmNtopAttachment, XmATTACH_WIDGET,
	  XmNtopWidget, status1,
	  XmNleftOffset, 20,
	  XmNleftAttachment, XmATTACH_FORM,
	  XmNrightAttachment, XmATTACH_POSITION,
	  XmNrightPosition, 70,
	  NULL);

  XtAddCallback (stopButton, XmNactivateCallback, StopCb, NULL);

  traceButton = XtVaCreateManagedWidget (
      "trace", xmPushButtonWidgetClass, parent,
      XmNtopAttachment, XmATTACH_WIDGET,
      XmNtopWidget, stopButton,
      XmNleftOffset, 20,
      XmNleftAttachment, XmATTACH_FORM,
	  XmNrightAttachment, XmATTACH_POSITION,
	  XmNrightPosition, 70,
      NULL);
  XtAddCallback (traceButton, XmNactivateCallback, TraceCb, NULL);

  applyButton = 
      XtVaCreateManagedWidget (
	  "apply", xmPushButtonWidgetClass, parent,
	  XmNtopAttachment, XmATTACH_WIDGET,
	  XmNtopWidget, traceButton,
	  XmNleftOffset, 20,
	  XmNleftAttachment, XmATTACH_FORM,
	  NULL);
  XtAddCallback (applyButton, XmNactivateCallback, ApplyCb, NULL);

  rasterButton = XtVaCreateManagedWidget (
      "raster", xmPushButtonWidgetClass, parent,
      XmNtopAttachment, XmATTACH_WIDGET,
      XmNtopWidget, applyButton,
      XmNleftOffset, 20,
      XmNleftAttachment, XmATTACH_FORM,
      NULL);
  XtAddCallback (rasterButton, XmNactivateCallback, RasterCb, NULL);
}


/* ------------------------------------------------------------------------- */

/* Sets up the debugger pane */
void
debug_main (void)
{
  Widget form, rc, menu;
  Arg arglist[20];
  int n;

  /* Vertical Pane of widgets */
  form = XmCreateForm (dshell, "pane", NULL, 0);

  /* Menu bar */
  menu=MenuBar (form);
  /* Assembly listing */

  n = 0;
  XtSetArg(arglist[n], XmNcolumns, 30); n++;
  XtSetArg(arglist[n], XmNrows, 40); n++;
  XtSetArg(arglist[n], XmNeditable, False); n++;
  XtSetArg(arglist[n], XmNcursorPositionVisible, False); n++;
  XtSetArg(arglist[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
  asmtext = XmCreateScrolledText(form, "view", arglist, n);
  XtVaSetValues(XtParent(asmtext),
		XmNwidth, 300,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, menu,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 60,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 30,
		NULL);
  XtManageChild(asmtext);

  rc = XtVaCreateManagedWidget ("form", xmFormWidgetClass, form, 
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, menu,
				XmNrightAttachment, XmATTACH_FORM,
				XmNleftAttachment, XmATTACH_WIDGET, 
				XmNleftWidget, asmtext,
				XmNbottomOffset, 30,
				XmNleftOffset, 20,
				XmNbottomAttachment, XmATTACH_FORM,
				NULL);
  
  RegisterBar (rc);

  /* Memory Pane */
#if 0
  /* these don't seem to be used */
  MEMbar = XtVaCreateManagedWidget ("membar", scrollbarWidgetClass, form, NULL);
  memview = XtVaCreateManagedWidget ("memview", asciiTextWidgetClass, memshell,
				     XtNfromHoriz, MEMbar,
				     NULL);
#endif
  XtManageChild(form);
}

/* Sets up the hardware window */
void
hardware_main (void)
{
  Widget vbox;
  Widget label, lastlabel;
  char *hlabelText[] =
  {"Ebeam X",
   "Ebeam Y",
   "CTRLPF ",
   "Timer C",
   "Tim cnt",
   "Tim res",
   "Tim clks",
   "pl[0].col",
   "pl[1].col",
   "pl[0].grp",
   "pl[1].grp",
   "pl[0].x",
   "pl[1].x"
  };
  int i;
  XmString xmstr;

  /* Vertical Pane of widgets */
  vbox = XtVaCreateManagedWidget ("vbox",
				  xmRowColumnWidgetClass, hshell, NULL);

  /* Hardware values boxes */
  lastlabel = NULL;
  for (i = 0; i < N_HREG; i++) {
      xmstr = XmStringCreateLocalized(hlabelText[i]);
      label = XtVaCreateManagedWidget ("register", xmLabelWidgetClass, vbox,
				       XmNlabelString, xmstr,
				       NULL);
      hregW[i] = XtVaCreateManagedWidget ("value", xmTextFieldWidgetClass,
					  vbox,
					  NULL);
  }

  /* Apply changes button */
  happlyButton = XtVaCreateManagedWidget (
      "apply", xmPushButtonWidgetClass, vbox,
      NULL);

#if 0
  /* why is this commented out? */
  /*XtAddCallback(happlyButton, XtNcallback, HapplyCb, NULL); */
#endif
}


/* Create actions, and call initialisers for each window */
void
init_debugger (void)
{
  XtActionsRec act[] =
  {
    {"debugRealize", (XtActionProc) debugRealizeAction},
    {"debugUnrealize", (XtActionProc) debugUnrealizeAction},
    {"raster", (XtActionProc) rasterAction}
  };

  if (Verbose)
    printf ("Initing debugger\n");

  dshell = XtVaAppCreateShell ("Debug", "Debug", topLevelShellWidgetClass,
    display, NULL, 0);

/*  dshell = XmCreateDialogShell (game_pane,"Debug", 0, 0);
    memshell = XmCreateDialogShell (dshell, "memory", 0, 0);
    hshell = XmCreateDialogShell (dshell,"Hardware", 0, 0);*/
  memshell = XtVaCreatePopupShell ("memory", transientShellWidgetClass,
 				   dshell, NULL);
  
  hshell = XtVaAppCreateShell ("Hardware", "Hardware",
 			       topLevelShellWidgetClass,
 			       display, NULL, 0);
 
  XtAppAddActions (app_context, act, 3);

  debug_main ();
  hardware_main ();
}
