/*
 * Copyright (c) Tony Bybell 2008.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if WAVE_USE_GTK2
#include <glib/gconvert.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "gtk12compat.h"
#include "analyzer.h"
#include "tree.h"
#include "symbol.h"
#include "vcd.h"
#include "lx2.h"
#include "busy.h"
#include "debug.h"
#include "hierpack.h"
#include "menu.h"
#include "tcl_helper.h"

#if !defined __MINGW32__ && !defined _MSC_VER
#include <sys/types.h>
#include <unistd.h>
#endif

#if defined(HAVE_LIBTCL)
#include <tcl.h>
#endif

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif


/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/* XXX functions for embedding TCL interpreter XXX */
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

#if defined(HAVE_LIBTCL)

static int gtkwavetcl_badNumArgs(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], int expected)
{
Tcl_Obj *aobj;
char reportString[1024];

sprintf(reportString, "* wrong number of arguments for '%s': %d expected, %d encountered", Tcl_GetString(objv[0]), expected, objc-1);

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);
return(TCL_ERROR);
}

static int gtkwavetcl_nop(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
/* nothing, this is simply to call gtk's main loop */
gtkwave_gtk_main_iteration();
return(TCL_OK);
}

static int gtkwavetcl_printInteger(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], int intVal)
{
Tcl_Obj *aobj;
char reportString[33];

sprintf(reportString, "%d", intVal);

aobj = Tcl_NewStringObj(reportString, -1); 
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_printTimeType(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], TimeType ttVal)
{
Tcl_Obj *aobj;
char reportString[65];

sprintf(reportString, TTFormat, ttVal);

aobj = Tcl_NewStringObj(reportString, -1); 
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_printDouble(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], double dVal)
{
Tcl_Obj *aobj;
char reportString[65];

sprintf(reportString, "%e", dVal);

aobj = Tcl_NewStringObj(reportString, -1); 
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_printString(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], char *reportString)
{
Tcl_Obj *aobj;

aobj = Tcl_NewStringObj(reportString, -1); 
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

/* tcl interface functions */

static int gtkwavetcl_getNumFacs(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->numfacs;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getLongestName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->longestname;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getFacName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
Tcl_Obj *aobj;

if(objc == 2)
	{
	char *s = Tcl_GetString(objv[1]);
	int which = atoi(s);

	if((which >= 0) && (which < GLOBALS->numfacs))
		{
		int was_packed;
        	char *hfacname = NULL;

        	hfacname = hier_decompress_flagged(GLOBALS->facs[which]->name, &was_packed);

		aobj = Tcl_NewStringObj(hfacname, -1); 
		Tcl_SetObjResult(interp, aobj);
		if(was_packed) free_2(hfacname);
		}
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_getMinTime(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
TimeType value = GLOBALS->min_time;
return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getMaxTime(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
TimeType value = GLOBALS->max_time;
return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getTimeDimension(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
Tcl_Obj *aobj;
char reportString[2] = { GLOBALS->time_dimension, 0 };

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_getBaselineMarker(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
TimeType value = GLOBALS->tims.baseline;
return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getMarker(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
TimeType value = GLOBALS->tims.marker;
return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getWindowStartTime(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
TimeType value = GLOBALS->tims.start;
return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getWindowEndTime(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
TimeType value = GLOBALS->tims.end;
return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getDumpType(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
Tcl_Obj *aobj;
char *reportString = "UNKNOWN";

if(GLOBALS->is_vcd) 
        {
        if(GLOBALS->partial_vcd)
                {
                reportString = "PVCD";
                }  
                else
                {
                reportString = "VCD";
                }
        }
else
if(GLOBALS->is_lxt)
        {
	reportString = "LXT";
        }
else
if(GLOBALS->is_ghw)
        {
	reportString = "GHW";
        }
else
if(GLOBALS->is_lx2)
        {
        switch(GLOBALS->is_lx2)
                {
                case LXT2_IS_LXT2: reportString = "LXT2"; break;
                case LXT2_IS_AET2: reportString = "AET2"; break;
                case LXT2_IS_VZT:  reportString = "VZT"; break;
                case LXT2_IS_VLIST:reportString = "VCD"; break;
                }
        }

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}


static int gtkwavetcl_getNamedMarker(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
	{
	char *s = Tcl_GetString(objv[1]);
	int which;

	if((s[0]>='A')&&(s[0]<='Z'))
		{
		TimeType value = GLOBALS->named_markers[s[0] - 'A'];
		return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
		}
	else
	if((s[0]>='a')&&(s[0]<='z'))
		{
		TimeType value = GLOBALS->named_markers[s[0] - 'a'];
		return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
		}

	which = atoi(s);
	if((which >= 0) && (which < 26))
		{
		TimeType value = GLOBALS->named_markers[which];
		return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
		}
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_getWaveHeight(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->waveheight;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getWaveWidth(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->wavewidth;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getPixelsUnitTime(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
double value = GLOBALS->pxns;
return(gtkwavetcl_printDouble(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getUnitTimePixels(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
double value = GLOBALS->nspx;
return(gtkwavetcl_printDouble(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getZoomFactor(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
double value = GLOBALS->tims.zoom;
return(gtkwavetcl_printDouble(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getDumpFileName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
char *value = GLOBALS->loaded_file_name;
return(gtkwavetcl_printString(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getVisibleNumTraces(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->traces.visible;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getTotalNumTraces(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->traces.total;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getTraceNameFromIndex(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
	{
	char *s = Tcl_GetString(objv[1]);
	int which = atoi(s);

	if((which >= 0) && (which < GLOBALS->traces.total))
		{
		Trptr t = GLOBALS->traces.first;
		int i = 0;
		while(t)
			{
			if(i == which)
				{
				if(t->name)
					{
					return(gtkwavetcl_printString(clientData, interp, objc, objv, t->name));
					}
					else
					{
					break;
					}
				}

			i++;
			t = t->t_next;
			}
		}
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 2));
        }

return(TCL_OK);
}

static int gtkwavetcl_getTraceFlagsFromIndex(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
	{
	char *s = Tcl_GetString(objv[1]);
	int which = atoi(s);

	if((which >= 0) && (which < GLOBALS->traces.total))
		{
		Trptr t = GLOBALS->traces.first;
		int i = 0;
		while(t)
			{
			if(i == which)
				{
				return(gtkwavetcl_printInteger(clientData, interp, objc, objv, t->flags));
				}

			i++;
			t = t->t_next;
			}
		}
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_getTraceValueAtMarkerFromIndex(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
	{
	char *s = Tcl_GetString(objv[1]);
	int which = atoi(s);

	if((which >= 0) && (which < GLOBALS->traces.total))
		{
		Trptr t = GLOBALS->traces.first;
		int i = 0;
		while(t)
			{
			if(i == which)
				{
				if(t->asciivalue)
					{
					char *pnt = t->asciivalue;
					if(*pnt == '=') pnt++;

					return(gtkwavetcl_printString(clientData, interp, objc, objv, pnt));
					}
					else
					{
					break;
					}
				}

			i++;
			t = t->t_next;
			}
		}
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_getHierMaxLevel(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->hier_max_level;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getFontHeight(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->fontheight;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getLeftJustifySigs(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = (GLOBALS->left_justify_sigs != 0);
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getSaveFileName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
char *value = GLOBALS->filesel_writesave;
if(value)
	{
	return(gtkwavetcl_printString(clientData, interp, objc, objv, value));
	}

return(TCL_OK);
}

static int gtkwavetcl_getStemsFileName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
char *value = GLOBALS->stems_name;
if(value)
	{
	return(gtkwavetcl_printString(clientData, interp, objc, objv, value));
	}

return(TCL_OK);
}

static int gtkwavetcl_getTraceScrollbarRowValue(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
GtkAdjustment *wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
int value = (int)wadj->value;

return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}



static int gtkwavetcl_setMarker(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = Tcl_GetString(objv[1]);
        TimeType mrk = unformat_time(s, GLOBALS->time_dimension);

	if((mrk >= GLOBALS->min_time) && (mrk <= GLOBALS->max_time))
		{
		GLOBALS->tims.marker = mrk;
		}
		else
		{
		GLOBALS->tims.marker = LLDescriptor(-1);
		}

        update_markertime(GLOBALS->tims.marker);
        GLOBALS->signalwindow_width_dirty=1;
        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);

	gtkwave_gtk_main_iteration();
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_setWindowStartTime(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = Tcl_GetString(objv[1]);

	if(s)
	        {
	        TimeType gt;
	        char timval[40];
	        GtkAdjustment *hadj;
	        TimeType pageinc;

	        gt=unformat_time(s, GLOBALS->time_dimension);    
          
	        if(gt<GLOBALS->tims.first) gt=GLOBALS->tims.first;
	        else if(gt>GLOBALS->tims.last) gt=GLOBALS->tims.last;
 
	        hadj=GTK_ADJUSTMENT(GLOBALS->wave_hslider);
	        hadj->value=gt;

	        pageinc=(TimeType)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
	        if(gt<(GLOBALS->tims.last-pageinc+1))
	                GLOBALS->tims.timecache=gt;
	                else
	                {
	                GLOBALS->tims.timecache=GLOBALS->tims.last-pageinc+1;
	                if(GLOBALS->tims.timecache<GLOBALS->tims.first) GLOBALS->tims.timecache=GLOBALS->tims.first;
	                }
	
	        reformat_time(timval,GLOBALS->tims.timecache,GLOBALS->time_dimension);
	        
	        time_update();
	        }

        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);
	gtkwave_gtk_main_iteration();
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_setZoomFactor(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = Tcl_GetString(objv[1]);
        float f;
         
        sscanf(s, "%f", &f);
        if(f>0.0)
                {
                f=0.0; /* in case they try to go out of range */
                }
        else
        if(f<-62.0)
                {
                f=-62.0; /* in case they try to go out of range */
                } 
                
        GLOBALS->tims.prevzoom=GLOBALS->tims.zoom; 
        GLOBALS->tims.zoom=(gdouble)f;
        calczoom(GLOBALS->tims.zoom);
        fix_wavehadj();

        gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed");
        gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed");

	gtkwave_gtk_main_iteration();
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_setLeftJustifySigs(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = Tcl_GetString(objv[1]);
        TimeType val = atoi_64(s);
	GLOBALS->left_justify_sigs = (val != LLDescriptor(0)) ? ~0 : 0;

        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);

	gtkwave_gtk_main_iteration();
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_setNamedMarker(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 3)
        {
        char *s = Tcl_GetString(objv[1]);
	int which = -1;

        if((s[0]>='A')&&(s[0]<='Z'))  
                {
                which = s[0] - 'A';
                }
        else
        if((s[0]>='a')&&(s[0]<='z'))  
                {
                which = s[0] - 'a';
                }
	else                
		{
	        which = atoi(s);
		}

        if((which >= 0) && (which < 26))
                {
	        char *t = Tcl_GetString(objv[2]);
		TimeType gt=unformat_time(t, GLOBALS->time_dimension);

                GLOBALS->named_markers[which] = gt;
	        wavearea_configure_event(GLOBALS->wavearea, NULL);
		gtkwave_gtk_main_iteration();
                } 
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 2));
        }

return(TCL_OK);
}


static int gtkwavetcl_setTraceScrollbarRowValue(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = Tcl_GetString(objv[1]);
        int target = atoi(s);
        GtkAdjustment *wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);

        int num_traces_displayable=(GLOBALS->signalarea->allocation.height)/(GLOBALS->fontheight);
        num_traces_displayable--;   /* for the time trace that is always there */

	if(target > GLOBALS->traces.visible - num_traces_displayable) target = GLOBALS->traces.visible - num_traces_displayable;

	if(target < 0) target = 0;

	wadj->value = target;

        gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed"); /* force bar update */
        gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /* force text update */
	gtkwave_gtk_main_iteration();
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}


static int gtkwavetcl_addSignalsFromList(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int i;
char *one_entry = NULL, *mult_entry = NULL;
unsigned int mult_len = 0;
int num_found = 0;
char reportString[33];
Tcl_Obj *aobj;

if(objc==2)
	{
        char *s = Tcl_GetString(objv[1]);
	char** elem = NULL;
	int l = 0;

	elem = zSplitTclList(s, &l);
 
	if(elem)
        	{
		for(i=0;i<l;i++)
			{
			one_entry = make_single_tcl_list_name(elem[i], NULL, 0);
			WAVE_OE_ME
			}
                free_2(elem);
                elem = NULL;
		num_found = process_tcl_list(mult_entry, FALSE);
		free_2(mult_entry);
		if(num_found)
        		{
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
        		wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_gtk_main_iteration();
        		}
                }
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

sprintf(reportString, "%d", num_found);

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}


static int gtkwavetcl_setTraceHighlightFromIndex(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 3)
	{
	char *s = Tcl_GetString(objv[1]);
	int which = atoi(s);
	char *t = Tcl_GetString(objv[2]);
	int onoff = atoi_64(t);

	if((which >= 0) && (which < GLOBALS->traces.total))
		{
		Trptr t = GLOBALS->traces.first;
		int i = 0;
		while(t)
			{
			if(i == which)
				{
				if(onoff)
					{
					t->flags |= TR_HIGHLIGHT;
					}
					else
					{
					t->flags &= (~TR_HIGHLIGHT);
					}
				gtkwave_gtk_main_iteration();
				break;
				}

			i++;
			t = t->t_next;
			}
		}
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 2));
        }

return(TCL_OK);
}

static int gtkwavetcl_setTraceHighlightFromNameMatch(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 3)
	{
	char *s = Tcl_GetString(objv[1]);
	int which = atoi(s);
	char *ts = Tcl_GetString(objv[2]);
	int onoff = atoi_64(ts);
	int mat = 0;

	if((which >= 0) && (which < GLOBALS->traces.total))
		{
		Trptr t = GLOBALS->traces.first;
		int i = 0;
		while(t)
			{
			if(t->name && !strcmp(t->name, s))
				{
				if(onoff)
					{
					t->flags |= TR_HIGHLIGHT;
					}
					else
					{
					t->flags &= (~TR_HIGHLIGHT);
					}
				mat++;
				}

			i++;
			t = t->t_next;
			}

		if(mat)
			{
			gtkwave_gtk_main_iteration();
			}

		return(gtkwavetcl_printInteger(clientData, interp, objc, objv, mat));
		}
	}
	else
	{
	return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 2));
	}

return(TCL_OK);
}


static int gtkwavetcl_findNextEdge(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
edge_search(STRACE_FORWARD);
gtkwave_gtk_main_iteration();
return(gtkwavetcl_getMarker(clientData, interp, objc, objv));
}


static int gtkwavetcl_findPrevEdge(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
edge_search(STRACE_BACKWARD);
gtkwave_gtk_main_iteration();
return(gtkwavetcl_getMarker(clientData, interp, objc, objv));
}


tcl_cmdstruct gtkwave_commands[] =
	{
	{"addSignalsFromList",			gtkwavetcl_addSignalsFromList},
	{"findNextEdge",			gtkwavetcl_findNextEdge},
	{"findPrevEdge",			gtkwavetcl_findPrevEdge},
	{"getBaselineMarker",			gtkwavetcl_getBaselineMarker},
	{"getDumpFileName",			gtkwavetcl_getDumpFileName},
	{"getDumpType", 			gtkwavetcl_getDumpType},
	{"getFacName", 				gtkwavetcl_getFacName},
	{"getFontHeight",			gtkwavetcl_getFontHeight},
	{"getHierMaxLevel",			gtkwavetcl_getHierMaxLevel},
	{"getLeftJustifySigs",			gtkwavetcl_getLeftJustifySigs},
	{"getLongestName", 			gtkwavetcl_getLongestName},
	{"getMarker",				gtkwavetcl_getMarker},
	{"getMaxTime", 				gtkwavetcl_getMaxTime},
	{"getMinTime", 				gtkwavetcl_getMinTime},
	{"getNamedMarker", 			gtkwavetcl_getNamedMarker},
	{"getNumFacs", 				gtkwavetcl_getNumFacs},
	{"getPixelsUnitTime", 			gtkwavetcl_getPixelsUnitTime},
	{"getSaveFileName",			gtkwavetcl_getSaveFileName},
	{"getStemsFileName",			gtkwavetcl_getStemsFileName},
	{"getTimeDimension", 			gtkwavetcl_getTimeDimension},
	{"getTotalNumTraces",  			gtkwavetcl_getTotalNumTraces},
	{"getTraceFlagsFromIndex", 		gtkwavetcl_getTraceFlagsFromIndex},
	{"getTraceNameFromIndex", 		gtkwavetcl_getTraceNameFromIndex},
	{"getTraceScrollbarRowValue", 		gtkwavetcl_getTraceScrollbarRowValue},
	{"getTraceValueAtMarkerFromIndex", 	gtkwavetcl_getTraceValueAtMarkerFromIndex},
	{"getUnitTimePixels", 			gtkwavetcl_getUnitTimePixels},
	{"getVisibleNumTraces", 		gtkwavetcl_getVisibleNumTraces},
	{"getWaveHeight", 			gtkwavetcl_getWaveHeight},
	{"getWaveWidth", 			gtkwavetcl_getWaveWidth},
	{"getWindowEndTime", 			gtkwavetcl_getWindowEndTime},
	{"getWindowStartTime", 			gtkwavetcl_getWindowStartTime},
	{"getZoomFactor",			gtkwavetcl_getZoomFactor},
   	{"nop", 				gtkwavetcl_nop},
	{"setLeftJustifySigs",			gtkwavetcl_setLeftJustifySigs},
	{"setMarker",				gtkwavetcl_setMarker},
	{"setNamedMarker",			gtkwavetcl_setNamedMarker},
	{"setTraceHighlightFromIndex",		gtkwavetcl_setTraceHighlightFromIndex},
	{"setTraceHighlightFromNameMatch",	gtkwavetcl_setTraceHighlightFromNameMatch},
	{"setTraceScrollbarRowValue", 		gtkwavetcl_setTraceScrollbarRowValue},
	{"setWindowStartTime",			gtkwavetcl_setWindowStartTime},
	{"setZoomFactor",			gtkwavetcl_setZoomFactor},
   	{"", 					NULL} /* sentinel */
	};

#else

static void dummy_function(void)
{
/* nothing */
}

#endif


/*
 * $Id$
 * $Log$
 * Revision 1.6  2008/11/24 03:26:52  gtkwave
 * warnings cleanups
 *
 * Revision 1.5  2008/11/24 02:55:10  gtkwave
 * use TCL_INCLUDE_SPEC to fix ubuntu compiles
 *
 * Revision 1.4  2008/11/19 18:15:35  gtkwave
 * add HAVE_LIBTCL to ifdefs which have HAVE_TCL_H
 *
 * Revision 1.3  2008/11/17 16:49:38  gtkwave
 * convert net object to netBus when encountering stranded bits in
 * signal search and tree search window
 *
 * Revision 1.2  2008/10/26 02:36:06  gtkwave
 * added netValue and netBusValue tcl list values from sigwin drag
 *
 * Revision 1.1  2008/10/17 18:05:27  gtkwave
 * split tcl command extensions out into their own separate file
 *
 * Revision 1.1  2008/10/17 18:22:01  gtkwave
 * file creation
 *
 */