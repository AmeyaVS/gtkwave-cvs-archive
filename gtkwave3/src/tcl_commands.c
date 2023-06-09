/*
 * Copyright (c) Tony Bybell 2008-2009.
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
#include "tcl_support_commands.h"

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
gtkwave_main_iteration();
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

static int gtkwavetcl_printString(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], const char *reportString)
{
Tcl_Obj *aobj;

aobj = Tcl_NewStringObj(reportString, -1); 
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static char *extractFullTraceName(Trptr t)
{
char *name = NULL;

 if(HasWave(t))
   {
     if (HasAlias(t))
       {
	 name = strdup_2(t->name_full);
       }
     else if (t->vector)
       {
	 name = strdup_2(t->n.vec->bvname);
       }
     else
       {
	 int flagged = 0;

	 name = hier_decompress_flagged(t->n.nd->nname, &flagged);
	 if(!flagged)
	   {
	     name = strdup_2(name);
	   }
       }
   }
 return(name);
}


/* tcl interface functions */

char *get_Tcl_string(Tcl_Obj *obj) {
  char *s = Tcl_GetString(obj) ;
  if (*s == '{') {		/* braced string */
    char *p = strrchr(s, '}') ;
    if(p) {
      if(GLOBALS->previous_braced_tcl_string)
		{
		free_2(GLOBALS->previous_braced_tcl_string);
		}

      GLOBALS->previous_braced_tcl_string = strdup_2(s);
      GLOBALS->previous_braced_tcl_string[p-s] = 0;
      s = GLOBALS->previous_braced_tcl_string + 1;
    }
  }
  return s ;
}

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
	char *s = get_Tcl_string(objv[1]);
	int which = atoi(s);

	if((which >= 0) && (which < GLOBALS->numfacs))
		{
		int was_packed = 0;
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
char reportString[2];

reportString[0] = GLOBALS->time_dimension;
reportString[1] = 0;

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_getArgv(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(GLOBALS->argvlist)
	{
	Tcl_Obj *aobj = Tcl_NewStringObj(GLOBALS->argvlist, -1);
	Tcl_SetObjResult(interp, aobj);
	}

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
                case LXT2_IS_FST:  reportString = "FST"; break;
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
	char *s = get_Tcl_string(objv[1]);
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
	char *s = get_Tcl_string(objv[1]);
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
	char *s = get_Tcl_string(objv[1]);
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
	char *s = get_Tcl_string(objv[1]);
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

static int gtkwavetcl_getTraceValueAtMarkerFromName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
	{
	char *s = get_Tcl_string(objv[1]);
	Trptr t = GLOBALS->traces.first;

	while(t)
		{
		if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
			{
			char *name = extractFullTraceName(t);
			if(!strcmp(name, s))
				{
				free_2(name);
				break;
				}
			free_2(name);
			}
		t = t-> t_next;
		}

	if(t)
		{
		if(t->asciivalue)
			{
			char *pnt = t->asciivalue;
			if(*pnt == '=') pnt++;
			return(gtkwavetcl_printString(clientData, interp, objc, objv, pnt));
			}
		}
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}


static int gtkwavetcl_getTraceValueAtNamedMarkerFromName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 3)
	{
	char *sv = get_Tcl_string(objv[1]);
	int which = -1;
	TimeType oldmarker = GLOBALS->tims.marker;
	TimeType value = LLDescriptor(-1);

	if((sv[0]>='A')&&(sv[0]<='Z'))
		{
		which = sv[0] - 'A';
		}
	else
	if((sv[0]>='a')&&(sv[0]<='z'))
		{
		which = sv[0] - 'a';
		}
	else
		{
		which = atoi(sv);
		}

	if((which >= 0) && (which < 26))
		{
		char *s = get_Tcl_string(objv[2]);
		Trptr t = GLOBALS->traces.first;

		value = GLOBALS->named_markers[which];

		while(t)
			{
			if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
				{
				char *name = extractFullTraceName(t);
				if(!strcmp(name, s))
					{
					free_2(name);
					break;
					}
				free_2(name);
				}
			t = t-> t_next;
			}

		if(t && (value >= LLDescriptor(0)))
			{
			GLOBALS->tims.marker = value;
		        GLOBALS->signalwindow_width_dirty=1;
		        MaxSignalLength();
		        signalarea_configure_event(GLOBALS->signalarea, NULL);
		        wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();

			if(t->asciivalue)
				{
				Tcl_Obj *aobj;
				char *pnt = t->asciivalue;
				if(*pnt == '=') pnt++;

				aobj = Tcl_NewStringObj(pnt, -1); 
				Tcl_SetObjResult(interp, aobj);

				GLOBALS->tims.marker = oldmarker;
				update_markertime(GLOBALS->tims.marker);
			        GLOBALS->signalwindow_width_dirty=1;
			        MaxSignalLength();
			        signalarea_configure_event(GLOBALS->signalarea, NULL);
			        wavearea_configure_event(GLOBALS->wavearea, NULL);
				gtkwave_main_iteration();

				return(TCL_OK);
				}

			GLOBALS->tims.marker = oldmarker;
			update_markertime(GLOBALS->tims.marker);
		        GLOBALS->signalwindow_width_dirty=1;
		        MaxSignalLength();
		        signalarea_configure_event(GLOBALS->signalarea, NULL);
		        wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();
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
        char *s = get_Tcl_string(objv[1]);
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

	gtkwave_main_iteration();
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}


static int gtkwavetcl_setBaselineMarker(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);
        TimeType mrk = unformat_time(s, GLOBALS->time_dimension);

	if((mrk >= GLOBALS->min_time) && (mrk <= GLOBALS->max_time))
		{
		GLOBALS->tims.baseline = mrk;
		}
		else
		{
		GLOBALS->tims.baseline = LLDescriptor(-1);
		}

        update_markertime(GLOBALS->tims.marker);
        GLOBALS->signalwindow_width_dirty=1;
        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);

	gtkwave_main_iteration();
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
        char *s = get_Tcl_string(objv[1]);

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
	gtkwave_main_iteration();
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
        char *s = get_Tcl_string(objv[1]);
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

	gtkwave_main_iteration();
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_setZoomRangeTimes(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 3)
        {
        char *s, *t;
	TimeType time1, time2;
	TimeType oldmarker = GLOBALS->tims.marker;

	s = get_Tcl_string(objv[1]);
	time1 = unformat_time(s, GLOBALS->time_dimension);
        t = get_Tcl_string(objv[2]);
	time2 = unformat_time(t, GLOBALS->time_dimension);

	if(time1 < GLOBALS->tims.first) { time1 = GLOBALS->tims.first; }
	if(time1 > GLOBALS->tims.last)  { time1 = GLOBALS->tims.last; }
	if(time2 < GLOBALS->tims.first) { time2 = GLOBALS->tims.first; }
	if(time2 > GLOBALS->tims.last)  { time2 = GLOBALS->tims.last; }

	service_dragzoom(time1, time2);
	GLOBALS->tims.marker = oldmarker;

        GLOBALS->signalwindow_width_dirty=1;
        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);

	gtkwave_main_iteration();
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
        char *s = get_Tcl_string(objv[1]);
        TimeType val = atoi_64(s);
	GLOBALS->left_justify_sigs = (val != LLDescriptor(0)) ? ~0 : 0;

        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);

	gtkwave_main_iteration();
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_setNamedMarker(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if((objc == 3)||(objc == 4))
        {
        char *s = get_Tcl_string(objv[1]);
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
	        char *t = get_Tcl_string(objv[2]);
		TimeType gt=unformat_time(t, GLOBALS->time_dimension);

                GLOBALS->named_markers[which] = gt;

		if(GLOBALS->marker_names[which]) 
			{
			free_2(GLOBALS->marker_names[which]);
			GLOBALS->marker_names[which] = NULL;
			}

		if(objc == 4)
			{
			char *u = get_Tcl_string(objv[3]);

			GLOBALS->marker_names[which] = strdup_2(u);
			}

	        wavearea_configure_event(GLOBALS->wavearea, NULL);
		gtkwave_main_iteration();
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
        char *s = get_Tcl_string(objv[1]);
        int target = atoi(s);

 	SetTraceScrollbarRowValue(target, 0);

/*         GtkAdjustment *wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider); */

/*         int num_traces_displayable=(GLOBALS->signalarea->allocation.height)/(GLOBALS->fontheight); */
/*         num_traces_displayable--;   /\* for the time trace that is always there *\/ */

/* 	if(target > GLOBALS->traces.visible - num_traces_displayable) target = GLOBALS->traces.visible - num_traces_displayable; */

/* 	if(target < 0) target = 0; */

/* 	wadj->value = target; */

/*         gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed"); /\* force bar update *\/ */
/*         gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /\* force text update *\/ */
/*	gtkwave_main_iteration(); */
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
        char *s = get_Tcl_string(objv[1]);
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
		if(mult_entry)
			{
			num_found = process_tcl_list(mult_entry, FALSE);
			free_2(mult_entry);
			}
		if(num_found)
        		{
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
        		wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();
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

static int gtkwavetcl_deleteSignalsFromList(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int i;
int num_found = 0;
char reportString[33];
Tcl_Obj *aobj;

if(objc==2)
	{
        char *s = get_Tcl_string(objv[1]);
	char** elem = NULL;
	int l = 0;

	elem = zSplitTclList(s, &l);
 
	if(elem)
        	{
		Trptr t = GLOBALS->traces.first;
		while(t)
			{
			t->cached_flags = t->flags;
			t->flags &= (~TR_HIGHLIGHT);	
			t = t->t_next;
			}

		for(i=0;i<l;i++)
			{
			t = GLOBALS->traces.first;
			while(t)
				{
				if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH|TR_HIGHLIGHT)))
					{
					char *name = extractFullTraceName(t);
					if(name)
						{
						int len_name = strlen(name);
						int len_elem = strlen(elem[i]);
						int brackmatch = (len_name > len_elem) && (name[len_elem] == '[');

						if(((len_name == len_elem) && (!strcmp(name, elem[i])))
							|| (brackmatch && !strncmp(name, elem[i], len_elem)))
							{
							t->flags |= TR_HIGHLIGHT;
							num_found++;
							break;
							}
						free_2(name);
						}
					}
				t = t->t_next;
				}
			}

                free_2(elem);
                elem = NULL;

		if(num_found)
        		{
			CutBuffer();
			}

		t = GLOBALS->traces.first;
		while(t)
			{
			t->flags = t->cached_flags;
			t->cached_flags = 0;
			t = t-> t_next;
			}

		if(num_found)
        		{
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
        		wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();
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

static int gtkwavetcl_deleteSignalsFromListIncludingDuplicates(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int i;
int num_found = 0;
char reportString[33];
Tcl_Obj *aobj;

if(objc==2)
	{
        char *s = get_Tcl_string(objv[1]);
	char** elem = NULL;
	int l = 0;

	elem = zSplitTclList(s, &l);
 
	if(elem)
        	{
		Trptr t = GLOBALS->traces.first;
		while(t)
			{
			t->cached_flags = t->flags;
			t->flags &= (~TR_HIGHLIGHT);	
		
			if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
				{
				char *name = extractFullTraceName(t);
				if(name)
					{
					for(i=0;i<l;i++)
						{
						int len_name = strlen(name);
						int len_elem = strlen(elem[i]);
						int brackmatch = (len_name > len_elem) && (name[len_elem] == '[');

						if(((len_name == len_elem) && (!strcmp(name, elem[i])))
							|| (brackmatch && !strncmp(name, elem[i], len_elem)))
							{
							t->flags |= TR_HIGHLIGHT;
							num_found++;
							break;
							}
						}
					free_2(name);
					}
				}

			t = t-> t_next;
			}

                free_2(elem);
                elem = NULL;

		if(num_found)
        		{
			CutBuffer();
			}

		t = GLOBALS->traces.first;
		while(t)
			{
			t->flags = t->cached_flags;
			t->cached_flags = 0;
			t = t-> t_next;
			}

		if(num_found)
        		{
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
        		wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();
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

static int gtkwavetcl_highlightSignalsFromList(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int i;
int num_found = 0;
char reportString[33];
Tcl_Obj *aobj;

if(objc==2)
	{
        char *s = get_Tcl_string(objv[1]);
	char** elem = NULL;
	int l = 0;

	elem = zSplitTclList(s, &l);
 
	if(elem)
        	{
		Trptr t = GLOBALS->traces.first;
		while(t)
			{
			if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
				{
				char *name = extractFullTraceName(t);
				if(name)
					{
					for(i=0;i<l;i++)
						{
						if(!strcmp(name, elem[i]))
							{
							t->flags |= TR_HIGHLIGHT;
							num_found++;
							break;
							}
						}
					free_2(name);
					}
				}

			t = t-> t_next;
			}

                free_2(elem);
                elem = NULL;

		if(num_found)
        		{
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
        		wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();
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

static int gtkwavetcl_unhighlightSignalsFromList(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int i;
int num_found = 0;
char reportString[33];
Tcl_Obj *aobj;

if(objc==2)
	{
        char *s = get_Tcl_string(objv[1]);
	char** elem = NULL;
	int l = 0;

	elem = zSplitTclList(s, &l);
 
	if(elem)
        	{
		Trptr t = GLOBALS->traces.first;
		while(t)
			{
			if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
				{
				char *name = extractFullTraceName(t);
				if(name)
					{
					for(i=0;i<l;i++)
						{
						if(!strcmp(name, elem[i]))
							{
							t->flags &= (~TR_HIGHLIGHT);
							num_found++;
							break;
							}
						}
					free_2(name);
					}
				}

			t = t-> t_next;
			}

                free_2(elem);
                elem = NULL;

		if(num_found)
        		{
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
        		wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();
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
	char *s = get_Tcl_string(objv[1]);
	int which = atoi(s);
	char *ts = get_Tcl_string(objv[2]);
	int onoff = atoi_64(ts);

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
	        		signalarea_configure_event(GLOBALS->signalarea, NULL);
				gtkwave_main_iteration();
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
	char *s = get_Tcl_string(objv[1]);
	int which = atoi(s);
	char *ts = get_Tcl_string(objv[2]);
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
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
			gtkwave_main_iteration();
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


static int gtkwavetcl_signalChangeList(ClientData clientData, Tcl_Interp *interp,
				       int objc, Tcl_Obj *CONST objv[]) {
    int dir = STRACE_FORWARD ;
    TimeType start_time = 0 ;
    TimeType end_time = MAX_HISTENT_TIME ;
    int max_elements = 0x7fffffff ;
    char *sig_name = NULL ;
    int i ;
    char *str_p, *str1_p ;
    int error = 0 ;
    llist_p *l_head ;
    Tcl_Obj *l_obj ;
    Tcl_Obj *obj ;
    llist_p *p, *p1 ;

    for(i=1; i<objc; i++) {
      str_p = Tcl_GetStringFromObj(objv[i], NULL) ;
      if (*str_p != '-') {
	sig_name = str_p ;
      } else {
        if(i == (objc-1)) { /* loop overflow on i check */
	  error++;
	  break;
	  }
	str1_p = Tcl_GetStringFromObj(objv[++i], NULL) ;
	switch(str_p[1]) {
	case 's': /* start time */
	  if(!strstr("-start_time", str_p))
	    error++ ;
	  else {
	    if((start_time = atoi_64(str1_p)) < 0)
	      start_time = 0 ;
	    dir = (start_time > end_time) ? STRACE_BACKWARD : STRACE_FORWARD ;
	  }
	  break ;
	case 'e': /* end time */
	  if(!strstr("-end_time", str_p))
	    error++ ;
	  else {
	    end_time = atoi_64(str1_p) ;
	    dir = (start_time > end_time) ? STRACE_BACKWARD : STRACE_FORWARD ;
	  }
	  break ;
	case 'm': /* max */
	  if(!strstr("-max", str_p))
	    error++ ;
	  else {
	    max_elements = atoi(str1_p) ;
	  }
	  break ;
	case 'd': /* dir */
	  if(!strstr("-dir", str_p))
	    error++ ;
	  else {
	    if(strstr("forward", str1_p))
	      dir = STRACE_FORWARD ;
	    else 
	      if(strstr("backward", str1_p))
		dir = STRACE_BACKWARD ;
	      else
		error++ ;
	  }
	  break ;
	default:
	  error++ ;
	}
      }
    }
    /* consistancy check */
    if(dir == STRACE_FORWARD) {
      if(start_time > end_time) error++ ;
    } else {
      if(start_time < end_time) {
	if(end_time == MAX_HISTENT_TIME)
	  end_time = 0 ;
	else
	  error ++ ;
      }
    }
    if(error) {
      Tcl_SetObjResult
	(interp, 
	 Tcl_NewStringObj("Usage: signal_change_list ?name? ?-start time? ?-end time? ?-max size? ?-dir forward|backward?", -1)) ;
      return TCL_ERROR;
    } 
    l_head = signal_change_list(sig_name, dir, start_time, end_time, max_elements) ;
    l_obj = Tcl_NewListObj(0, NULL) ;
    p = l_head;
    while(p) {
      obj = Tcl_NewWideIntObj((Tcl_WideInt )p->u.tt) ;
      p1= p->next ;
      free_2(p) ;
      Tcl_ListObjAppendElement(interp, l_obj, obj) ;
      obj = Tcl_NewStringObj(p1->u.str,-1) ;
      Tcl_ListObjAppendElement(interp, l_obj, obj) ;
      p = p1->next ;
      free_2(p1->u.str) ;
      free_2(p1) ;
    }
    Tcl_SetObjResult(interp, l_obj) ;
    return TCL_OK ;
}

static int gtkwavetcl_findNextEdge(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
edge_search(STRACE_FORWARD);
gtkwave_main_iteration();
return(gtkwavetcl_getMarker(clientData, interp, objc, objv));
}


static int gtkwavetcl_findPrevEdge(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
edge_search(STRACE_BACKWARD);
gtkwave_main_iteration();
return(gtkwavetcl_getMarker(clientData, interp, objc, objv));
}


int SST_open_node(char *name) ;
static int gtkwavetcl_forceOpenTreeNode(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int rv = -100;		/* Tree does not exist */
  char *s = NULL ;
  if(objc == 2)
    s = get_Tcl_string(objv[1]);
  
  if(s && (strlen(s) > 1)) {	/* exclude empty strings */
    int len = strlen(s);
    if(s[len-1]!=GLOBALS->hier_delimeter)
      {
#ifdef WAVE_USE_GTK2
	rv = SST_open_node(s);
#endif
      }
    else {
#ifdef WAVE_USE_GTK2
      rv = SST_open_node(s);
#endif
    }
  } else {
    if (GLOBALS->selected_hierarchy_name) {
      rv = SST_NODE_CURRENT ;
    }
    gtkwave_main_iteration(); /* check if this is needed */
  }
  if (rv == -100) {
    return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
  }
  Tcl_SetObjResult(GLOBALS->interp, (rv == SST_NODE_CURRENT) ?
		   Tcl_NewStringObj(GLOBALS->selected_hierarchy_name,
				    strlen(GLOBALS->selected_hierarchy_name)) :
		   Tcl_NewIntObj(rv)) ;
  
  return(TCL_OK);
}

static int gtkwavetcl_setFromEntry(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);

	if(s)
		{
		gtk_entry_set_text(GTK_ENTRY(GLOBALS->from_entry),s);
		from_entry_callback(NULL, GLOBALS->from_entry);		
		}

	gtkwave_main_iteration();
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}


static int gtkwavetcl_setToEntry(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);

	if(s)
		{
		gtk_entry_set_text(GTK_ENTRY(GLOBALS->to_entry),s);
		to_entry_callback(NULL, GLOBALS->to_entry);		
		}

	gtkwave_main_iteration();
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}


static int gtkwavetcl_getFromEntry(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
const char *value = gtk_entry_get_text(GTK_ENTRY(GLOBALS->from_entry));
if(value)
        {
        return(gtkwavetcl_printString(clientData, interp, objc, objv, value));
        }

return(TCL_OK);
}       


static int gtkwavetcl_getToEntry(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
const char *value = gtk_entry_get_text(GTK_ENTRY(GLOBALS->to_entry));
if(value)
        {
        return(gtkwavetcl_printString(clientData, interp, objc, objv, value));
        }

return(TCL_OK);
}       


static int gtkwavetcl_getDisplayedSignals(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 1)
        {
	char *rv = add_traces_from_signal_window(TRUE);
	int rc = gtkwavetcl_printString(clientData, interp, objc, objv, rv);

	free_2(rv);
	return(rc);
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }
}


static int gtkwavetcl_getTraceFlagsFromName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
	{
	char *s = get_Tcl_string(objv[1]);
	Trptr t = GLOBALS->traces.first;

	while(t)
		{
		if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
			{
			char *name = extractFullTraceName(t);
			if(!strcmp(name, s))
				{
				free_2(name);
				break;
				}
			free_2(name);
			}
		t = t-> t_next;
		}

	if(t)
		{
		return(gtkwavetcl_printInteger(clientData, interp, objc, objv, t->flags));
		}
	}
        else  
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_loadFile(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  if(objc == 2)
    {
      char *s = get_Tcl_string(objv[1]);
	
      if(!GLOBALS->in_tcl_callback)                          
        {
        /*	read_save_helper(s); */
        process_url_file(s);
        /*	process_url_list(s); */
        /*	gtkwave_main_iteration(); */
        }
	else
	{
	gtkwavetcl_setvar_nonblocking(WAVE_TCLCB_ERROR,"gtkwave::loadFile prohibited in callback",WAVE_TCLCB_ERROR_FLAGS);
	}
    }
  else  
    {
      return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
    }

  return(TCL_OK);
}

static int gtkwavetcl_reLoadFile(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{

  if(objc == 1)
    {
      if(!GLOBALS->in_tcl_callback)
	{
      	reload_into_new_context();
	}
	else
	{
	gtkwavetcl_setvar_nonblocking(WAVE_TCLCB_ERROR,"gtkwave::reLoadFile prohibited in callback",WAVE_TCLCB_ERROR_FLAGS);
	}
    }
  else  
    {
      return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 0));
    }
  return(TCL_OK);
}

static int gtkwavetcl_presentWindow(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{

  if(objc == 1)
    {
#ifdef WAVE_USE_GTK2 
      gtk_window_present(GTK_WINDOW(GLOBALS->mainwindow));
#else
      gdk_window_raise(GTK_WIDGET(GLOBALS->mainwindow)->window);
#endif
    }
  else  
    {
      return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 0));
    }
  return(TCL_OK);
}

static int gtkwavetcl_showSignal(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  if(objc == 3)
    {
      char *s0 = get_Tcl_string(objv[1]);
      char *s1;
      int row;
      unsigned location;

      sscanf(s0, "%d", &row);
      if (row < 0) { row = 0; };

      s1 = get_Tcl_string(objv[2]);
      sscanf(s1, "%u", &location);

      SetTraceScrollbarRowValue(row, location);
    }
  else  
    {
      return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 2));
    }

  return(TCL_OK);
}


/* 
 * swap to a given context based on tab number (from Tcl)
 */
static gint switch_to_tab_number(unsigned int i)
{       
if(i < GLOBALS->num_notebook_pages)  
        {
        struct Global *g_old = GLOBALS;
        /* printf("Switching to: %d\n", i); */

        set_GLOBALS((*GLOBALS->contexts)[i]);
        
        GLOBALS->lxt_clock_compress_to_z = g_old->lxt_clock_compress_to_z;
        GLOBALS->autoname_bundles = g_old->autoname_bundles;
        GLOBALS->autocoalesce_reversal = g_old->autocoalesce_reversal;
        GLOBALS->autocoalesce = g_old->autocoalesce;
        GLOBALS->hier_grouping = g_old->hier_grouping;
        GLOBALS->wave_scrolling = g_old->wave_scrolling;
        GLOBALS->constant_marker_update = g_old->constant_marker_update;
        GLOBALS->do_zoom_center = g_old->do_zoom_center;
        GLOBALS->use_roundcaps = g_old->use_roundcaps;
        GLOBALS->do_resize_signals = g_old->do_resize_signals;
	GLOBALS->initial_signal_window_width = g_old->initial_signal_window_width;
        GLOBALS->use_full_precision = g_old->use_full_precision;
        GLOBALS->show_base = g_old->show_base;
        GLOBALS->display_grid = g_old->display_grid;
        GLOBALS->disable_mouseover = g_old->disable_mouseover;
        GLOBALS->zoom_pow10_snap = g_old->zoom_pow10_snap;
                                         
        GLOBALS->scale_to_time_dimension = g_old->scale_to_time_dimension;
        GLOBALS->zoom_dyn = g_old->zoom_dyn;
        GLOBALS->zoom_dyne = g_old->zoom_dyne;
                                                         
        gtk_notebook_set_current_page(GTK_NOTEBOOK(GLOBALS->notebook), GLOBALS->this_context_page);
        return(TRUE);
        }
                
return(FALSE);
}                     


static int gtkwavetcl_setTabActive(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {       
	gint rc;

	if(!GLOBALS->in_tcl_callback)
		{
        	char *s = get_Tcl_string(objv[1]);
        	unsigned int tabnum = atoi(s);
		rc = switch_to_tab_number(tabnum);
        
        	MaxSignalLength();
        	signalarea_configure_event(GLOBALS->signalarea, NULL);
        
        	gtkwave_main_iteration();
		}
		else
		{
		gtkwavetcl_setvar_nonblocking(WAVE_TCLCB_ERROR,"gtkwave::setTabActive prohibited in callback",WAVE_TCLCB_ERROR_FLAGS);
		rc = -1;
		}

	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, rc));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }
       
}


static int gtkwavetcl_getNumTabs(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->num_notebook_pages;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}


static int gtkwavetcl_installFileFilter(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {       
        char *s = get_Tcl_string(objv[1]);
        unsigned int which = atoi(s);
	gint rc = install_file_filter(which);
        
        gtkwave_main_iteration();
	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, rc));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }
       
}


static int gtkwavetcl_setCurrentTranslateFile(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {       
        char *s = get_Tcl_string(objv[1]);
	set_current_translate_file(s);

	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, GLOBALS->current_translate_file));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }
       
}


static int gtkwavetcl_setCurrentTranslateEnums(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {       
        char *s = get_Tcl_string(objv[1]);
	set_current_translate_enums(s);

	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, GLOBALS->current_translate_file));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }
       
}


static int gtkwavetcl_installProcFilter(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {       
        char *s = get_Tcl_string(objv[1]);
        unsigned int which = atoi(s);
	gint rc = install_file_filter(which);
        
        gtkwave_main_iteration();
	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, rc));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }
       
}


static int gtkwavetcl_setCurrentTranslateProc(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {       
        char *s = get_Tcl_string(objv[1]);
	set_current_translate_proc(s);

	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, GLOBALS->current_translate_proc));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }
       
}


static int gtkwavetcl_installTransFilter(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {       
        char *s = get_Tcl_string(objv[1]);
        unsigned int which = atoi(s);
	gint rc = install_ttrans_filter(which);
        
        gtkwave_main_iteration();
	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, rc));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }
       
}


static int gtkwavetcl_setCurrentTranslateTransProc(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {       
        char *s = get_Tcl_string(objv[1]);
	set_current_translate_ttrans(s);

	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, GLOBALS->current_translate_ttrans));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }
       
}


tcl_cmdstruct gtkwave_commands[] =
	{
	{"addSignalsFromList",			gtkwavetcl_addSignalsFromList},
	{"deleteSignalsFromList",		gtkwavetcl_deleteSignalsFromList},
	{"deleteSignalsFromListIncludingDuplicates", gtkwavetcl_deleteSignalsFromListIncludingDuplicates},
	{"findNextEdge",			gtkwavetcl_findNextEdge},
	{"findPrevEdge",			gtkwavetcl_findPrevEdge},
	{"forceOpenTreeNode",			gtkwavetcl_forceOpenTreeNode},
	{"getArgv",				gtkwavetcl_getArgv},
	{"getBaselineMarker",			gtkwavetcl_getBaselineMarker},
	{"getDisplayedSignals",			gtkwavetcl_getDisplayedSignals},
	{"getDumpFileName",			gtkwavetcl_getDumpFileName},
	{"getDumpType", 			gtkwavetcl_getDumpType},
	{"getFacName", 				gtkwavetcl_getFacName},
	{"getFontHeight",			gtkwavetcl_getFontHeight},
	{"getFromEntry",			gtkwavetcl_getFromEntry},
	{"getHierMaxLevel",			gtkwavetcl_getHierMaxLevel},
	{"getLeftJustifySigs",			gtkwavetcl_getLeftJustifySigs},
	{"getLongestName", 			gtkwavetcl_getLongestName},
	{"getMarker",				gtkwavetcl_getMarker},
	{"getMaxTime", 				gtkwavetcl_getMaxTime},
	{"getMinTime", 				gtkwavetcl_getMinTime},
	{"getNamedMarker", 			gtkwavetcl_getNamedMarker},
	{"getNumFacs", 				gtkwavetcl_getNumFacs},
	{"getNumTabs",				gtkwavetcl_getNumTabs},
	{"getPixelsUnitTime", 			gtkwavetcl_getPixelsUnitTime},
	{"getSaveFileName",			gtkwavetcl_getSaveFileName},
	{"getStemsFileName",			gtkwavetcl_getStemsFileName},
	{"getTimeDimension", 			gtkwavetcl_getTimeDimension},
	{"getToEntry",				gtkwavetcl_getToEntry},
	{"getTotalNumTraces",  			gtkwavetcl_getTotalNumTraces},
	{"getTraceFlagsFromIndex", 		gtkwavetcl_getTraceFlagsFromIndex},
	{"getTraceFlagsFromName",		gtkwavetcl_getTraceFlagsFromName},
	{"getTraceNameFromIndex", 		gtkwavetcl_getTraceNameFromIndex},
	{"getTraceScrollbarRowValue", 		gtkwavetcl_getTraceScrollbarRowValue},
	{"getTraceValueAtMarkerFromIndex", 	gtkwavetcl_getTraceValueAtMarkerFromIndex},
	{"getTraceValueAtMarkerFromName",	gtkwavetcl_getTraceValueAtMarkerFromName},
	{"getTraceValueAtNamedMarkerFromName",	gtkwavetcl_getTraceValueAtNamedMarkerFromName},
	{"getUnitTimePixels", 			gtkwavetcl_getUnitTimePixels},
	{"getVisibleNumTraces", 		gtkwavetcl_getVisibleNumTraces},
	{"getWaveHeight", 			gtkwavetcl_getWaveHeight},
	{"getWaveWidth", 			gtkwavetcl_getWaveWidth},
	{"getWindowEndTime", 			gtkwavetcl_getWindowEndTime},
	{"getWindowStartTime", 			gtkwavetcl_getWindowStartTime},
	{"getZoomFactor",			gtkwavetcl_getZoomFactor},
	{"highlightSignalsFromList",		gtkwavetcl_highlightSignalsFromList},
	{"installFileFilter",			gtkwavetcl_installFileFilter},
	{"installProcFilter",			gtkwavetcl_installProcFilter},
	{"installTransFilter",			gtkwavetcl_installTransFilter},
   	{"nop", 				gtkwavetcl_nop},
	{"setBaselineMarker",			gtkwavetcl_setBaselineMarker},
	{"setCurrentTranslateEnums",		gtkwavetcl_setCurrentTranslateEnums},
	{"setCurrentTranslateFile",		gtkwavetcl_setCurrentTranslateFile},
	{"setCurrentTranslateProc",		gtkwavetcl_setCurrentTranslateProc},
	{"setCurrentTranslateTransProc",	gtkwavetcl_setCurrentTranslateTransProc},
	{"setFromEntry",			gtkwavetcl_setFromEntry},
	{"setLeftJustifySigs",			gtkwavetcl_setLeftJustifySigs},
	{"setMarker",				gtkwavetcl_setMarker},
	{"setNamedMarker",			gtkwavetcl_setNamedMarker},
   	{"setTabActive",			gtkwavetcl_setTabActive},
	{"setToEntry",				gtkwavetcl_setToEntry},
	{"setTraceHighlightFromIndex",		gtkwavetcl_setTraceHighlightFromIndex},
	{"setTraceHighlightFromNameMatch",	gtkwavetcl_setTraceHighlightFromNameMatch},
	{"setTraceScrollbarRowValue", 		gtkwavetcl_setTraceScrollbarRowValue},
	{"setWindowStartTime",			gtkwavetcl_setWindowStartTime},
	{"setZoomFactor",			gtkwavetcl_setZoomFactor},
	{"setZoomRangeTimes",			gtkwavetcl_setZoomRangeTimes},
	{"loadFile",			        gtkwavetcl_loadFile},
	{"reLoadFile",			        gtkwavetcl_reLoadFile},
	{"presentWindow",			gtkwavetcl_presentWindow},
	{"showSignal",         			gtkwavetcl_showSignal},
	{"unhighlightSignalsFromList",		gtkwavetcl_unhighlightSignalsFromList},
	{"signalChangeList",                    gtkwavetcl_signalChangeList},	/* changed from signal_change_list for consistency! */
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
 * Revision 1.45  2010/10/26 17:37:35  gtkwave
 * added initial_signal_window_width rc variable
 *
 * Revision 1.44  2010/08/17 01:51:35  gtkwave
 * added missing global in error message
 *
 * Revision 1.43  2010/08/15 18:54:49  gtkwave
 * fixes to SST open node to support tcl bug workaround (?)
 *
 * Revision 1.42  2010/07/28 19:56:27  gtkwave
 * locking down callbacks from calling context changing events in viewer
 *
 * Revision 1.41  2010/07/19 22:32:31  gtkwave
 * added gtkwave::setCurrentTranslateEnums
 *
 * Revision 1.40  2010/07/19 21:12:19  gtkwave
 * added file/proc/trans access functions to Tcl script interpreter
 *
 * Revision 1.39  2010/05/21 19:47:58  gtkwave
 * fixes to tcl string handling on { ... } string case.
 *
 * Revision 1.38  2010/04/04 19:09:57  gtkwave
 * rename name->bvname in struct BitVector for easier grep tracking
 *
 * Revision 1.37  2010/03/18 17:12:37  gtkwave
 * pedantic warning cleanups
 *
 * Revision 1.36  2010/02/18 23:06:04  gtkwave
 * change name of main iteration loop calls
 *
 * Revision 1.35  2009/12/24 20:55:27  gtkwave
 * warnings cleanups
 *
 * Revision 1.34  2009/11/29 19:16:13  gtkwave
 * added gtkwave::setTabActive and gtkwave::getNumTabs
 *
 * Revision 1.33  2009/11/29 18:38:20  gtkwave
 * added FST to return types for gtkwave::getDumpType
 *
 * Revision 1.32  2009/11/06 04:15:17  gtkwave
 * gtk+-1.2 compile fixes
 *
 * Revision 1.31  2009/10/26 22:44:01  gtkwave
 * output style fixes, remove double init for bluespec
 *
 * Revision 1.30  2009/10/08 17:40:49  gtkwave
 * removed casting on llist_new, use union instead as arg
 *
 * Revision 1.29  2009/10/07 21:13:10  gtkwave
 * 64-bit fixes on signal list generation
 *
 * Revision 1.28  2009/09/29 02:45:19  gtkwave
 * warnings cleanups
 *
 * Revision 1.27  2009/09/28 05:58:05  gtkwave
 * changes to support signal_change_list
 *
 * Revision 1.26  2009/09/20 21:45:50  gtkwave
 * tree force open node handling changed for tcl
 *
 * Revision 1.25  2009/09/14 03:00:08  gtkwave
 * bluespec code integration
 *
 * Revision 1.24  2009/03/26 20:57:42  gtkwave
 * added MISSING_FILE support for bringing up gtkwave without a dumpfile
 *
 * Revision 1.23  2009/02/16 05:24:32  gtkwave
 * added setBaselineMarker command
 *
 * Revision 1.22  2009/02/02 16:10:42  gtkwave
 * added gtkwavetcl_getTraceFlagsFromName
 *
 * Revision 1.21  2009/01/21 19:52:13  gtkwave
 * allow brackets to be optional on signal delete
 *
 * Revision 1.20  2009/01/21 16:23:25  gtkwave
 * fixed delete behavior so it deletes only the 1st instance appropriately
 *
 * Revision 1.19  2009/01/21 02:24:15  gtkwave
 * gtk1 compile fixes, ensure ctree_main is available for force_open_tree_node
 *
 * Revision 1.18  2009/01/20 06:11:48  gtkwave
 * added gtkwave::getDisplayedSignals command
 *
 * Revision 1.17  2009/01/16 19:27:00  gtkwave
 * added more tcl commands
 *
 * Revision 1.16  2009/01/05 03:24:02  gtkwave
 * fixes for calling configure for updated areas
 *
 * Revision 1.15  2009/01/04 21:48:24  gtkwave
 * setNamedMarker fix for a string set followed by a non-string one
 *
 * Revision 1.14  2009/01/02 06:24:28  gtkwave
 * bumped copyright to 2009
 *
 * Revision 1.13  2009/01/02 06:11:00  gtkwave
 * needed to clone GLOBALS->interp from one instance to the next in maketabs
 *
 * Revision 1.12  2009/01/02 06:01:51  gtkwave
 * added getArgv for tcl commands
 *
 * Revision 1.11  2009/01/01 03:55:12  gtkwave
 * more tcl command adds...value retrieval
 *
 * Revision 1.10  2008/12/31 22:20:12  gtkwave
 * adding more tcl commands
 *
 * Revision 1.9  2008/12/25 03:28:55  gtkwave
 * -Wshadow warning fixes
 *
 * Revision 1.8  2008/12/16 18:21:02  gtkwave
 * can now set named marker user names through Tcl scripts
 *
 * Revision 1.7  2008/11/25 18:07:32  gtkwave
 * added cut copy paste functionality that survives reload and can do
 * multiple pastes on the same cut buffer
 *
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
