/* 
 * Copyright (c) Tony Bybell 1999-2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/*
 * note: any functions which add/remove traces must first look at
 * the global "straces".  if it's active, complain to the status
 * window and don't do the op. same for "dnd_state".
 */

#include "globals.h"
#include <config.h>
#include <string.h>
#include "gtk12compat.h"
#include "main.h"
#include "menu.h"
#include "vcd.h"
#include "vcd_saver.h"
#include "translate.h"
#include "ptranslate.h"
#include "ttranslate.h"
#include "lx2.h"
#include "hierpack.h"
#include "tcl_helper.h"

#if !defined __MINGW32__ && !defined _MSC_VER
#include <unistd.h>
#include <sys/mman.h>
#else
#include <windows.h>
#include <io.h>
#endif

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

#undef WAVE_USE_MENU_BLACKOUTS

static GtkItemFactoryEntry menu_items[WV_MENU_NUMITEMS];

/* marker locking */

static void lock_marker_left(GtkWidget *widget, gpointer data)
{
int ent_idx = GLOBALS->named_marker_lock_idx;
int i;
int success = 0;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nLock to Lesser Named Marker");
        help_text(
                " locks the primary marker to a named marker."
		" If no named marker is currently selected, the last defined one is used,"
		" otherwise the marker selected will be one lower in the alphabet, scrolling"
		" through to the end of the alphabet on wrap."
		" If no named marker exists, one is dropped down for 'A' and the primary"
		" marker is locked to it."
        );

	return;
	}

if(ent_idx < 0) ent_idx = 26;
ent_idx--;
if(ent_idx < 0) ent_idx = 25;

for(i=0;i<26;i++)
	{
	if(GLOBALS->named_markers[ent_idx] >= 0)
		{
		success = 1;
		break;
		}
	
	ent_idx--;
	if(ent_idx < 0) ent_idx = 25;
	}

if(!success)
	{
	ent_idx = 0;
	GLOBALS->named_markers[ent_idx] = GLOBALS->tims.marker;
	}

GLOBALS->named_marker_lock_idx = ent_idx;
GLOBALS->tims.marker = GLOBALS->named_markers[ent_idx];

update_markertime(GLOBALS->tims.marker);
GLOBALS->signalwindow_width_dirty=1;
MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}

static void lock_marker_right(GtkWidget *widget, gpointer data)
{
int ent_idx = GLOBALS->named_marker_lock_idx;
int i;
int success = 0;

if(ent_idx < 0) ent_idx = -1; /* not really necessary */
ent_idx++;
if(ent_idx > 25) ent_idx = 0;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nLock to Greater Named Marker");
        help_text(
                " locks the primary marker to a named marker."
		" If no named marker is currently selected, the first defined one is used,"
		" otherwise the marker selected will be one higher in the alphabet, scrolling"
		" through to the beginning of the alphabet on wrap."
		" If no named marker exists, one is dropped down for 'A' and the primary"
		" marker is locked to it."
        );

	return;
	}

for(i=0;i<26;i++)
	{
	if(GLOBALS->named_markers[ent_idx] >= 0)
		{
		success = 1;
		break;
		}
	
	ent_idx++;
	if(ent_idx > 25) ent_idx = 0;
	}

if(!success)
	{
	ent_idx = 0;
	GLOBALS->named_markers[ent_idx] = GLOBALS->tims.marker;
	}

GLOBALS->named_marker_lock_idx = ent_idx;
GLOBALS->tims.marker = GLOBALS->named_markers[ent_idx];

update_markertime(GLOBALS->tims.marker);

GLOBALS->signalwindow_width_dirty=1;
MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}


static void unlock_marker(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nUnlock from Named Marker");
        help_text(
                " unlocks the primary marker from the currently selected named marker."
        );

	return;
	}

GLOBALS->named_marker_lock_idx = -1;

GLOBALS->signalwindow_width_dirty=1;
MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}


/* toggles for time dimension conversion */

void menu_scale_to_td_x(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nScale To Time Dimension: None");
        help_text(
		" turns off time dimension conversion."
        );
        }
	else
	{
	GLOBALS->scale_to_time_dimension = 0;
	set_scale_to_time_dimension_toggles();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}

void menu_scale_to_td_s(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nScale To Time Dimension: sec");
        help_text(
		" changes the time dimension conversion value to seconds."
        );
        }
	else
	{
	GLOBALS->scale_to_time_dimension = 's';
	set_scale_to_time_dimension_toggles();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}

void menu_scale_to_td_m(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nScale To Time Dimension: ms");
        help_text(
		" changes the time dimension conversion value to milliseconds."
        );
        }
	else
	{
	GLOBALS->scale_to_time_dimension = 'm';
	set_scale_to_time_dimension_toggles();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}


void menu_scale_to_td_u(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nScale To Time Dimension: us");
        help_text(
		" changes the time dimension conversion value to microseconds."
        );
        }
	else
	{
	GLOBALS->scale_to_time_dimension = 'u';
	set_scale_to_time_dimension_toggles();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}


void menu_scale_to_td_n(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nScale To Time Dimension: ns");
        help_text(
		" changes the time dimension conversion value to nanoseconds."
        );
        }
	else
	{
	GLOBALS->scale_to_time_dimension = 'n';
	set_scale_to_time_dimension_toggles();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}


void menu_scale_to_td_p(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nScale To Time Dimension: ps");
        help_text(
		" changes the time dimension conversion value to picoseconds."
        );
        }
	else
	{
	GLOBALS->scale_to_time_dimension = 'p';
	set_scale_to_time_dimension_toggles();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}


void menu_scale_to_td_f(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nScale To Time Dimension: fs");
        help_text(
		" changes the time dimension conversion value to femtoseconds."
        );
        }
	else
	{
	GLOBALS->scale_to_time_dimension = 'f';
	set_scale_to_time_dimension_toggles();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}


/********** transaction procsel filter install ********/

void menu_dataformat_xlate_ttrans_1(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nTransaction Filter Process");
        help_text(
                " will enable transaction filtering on marked traces using a filter process.  A requester will appear to get the filter filename."
        );
        return;
        }

ttrans_searchbox("Select Transaction Filter Process");
}

void menu_dataformat_xlate_ttrans_0(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nTransaction Filter Process Disable");
        help_text(
                " will remove transaction filtering."
        );
        return;
        }

install_ttrans_filter(0); /* disable, 0 is always NULL */
}

/********** procsel filter install ********/

void menu_dataformat_xlate_proc_1(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nTranslate Filter Process");
        help_text(
                " will enable translation on marked traces using a filter process.  A requester will appear to get the filter filename."
        );
        return;
        }

ptrans_searchbox("Select Signal Filter Process");
}

void menu_dataformat_xlate_proc_0(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nTranslate Filter Process Disable");
        help_text(
                " will remove translation filtering used to reconstruct"
                " enums for marked traces."
        );
        return;
        }

install_proc_filter(0); /* disable, 0 is always NULL */
}

/********** filesel filter install ********/

void menu_dataformat_xlate_file_1(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nTranslate Filter File");
        help_text(
                " will enable translation on marked traces using a filter file.  A requester will appear to get the filter filename."
        );
        return;
        }

trans_searchbox("Select Signal Filter");
}


void menu_dataformat_xlate_file_0(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nTranslate Filter File Disable");
        help_text(
                " will remove translation filtering used to reconstruct"
                " enums for marked traces."
        );
        return;
        }

install_file_filter(0); /* disable, 0 is always NULL */
}


/******************************************************************/

void menu_write_lxt_file_cleanup(GtkWidget *widget, gpointer data)
{
int rc;

if(!GLOBALS->filesel_ok)
        {
        return;
        }               
                                
if(GLOBALS->lock_menu_c_1 == 1) return; /* avoid recursion */
GLOBALS->lock_menu_c_1 = 1;

status_text("Saving LXT...\n");
gtkwave_main_iteration(); /* make requester disappear requester */

rc = save_nodes_to_export(*GLOBALS->fileselbox_text, WAVE_EXPORT_LXT);

GLOBALS->lock_menu_c_1 = 0;

switch(rc)
	{
	case VCDSAV_EMPTY:	status_text("No traces onscreen to save!\n");
				break;

	case VCDSAV_FILE_ERROR:	status_text("Problem writing LXT: ");
				status_text(strerror(errno));
				break;

	case VCDSAV_OK:		status_text("LXT written successfully.\n");
	default:		break;
	}
}

void
menu_write_lxt_file(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
	{
	help_text_bold("\n\nWrite LXT File As");
	help_text(
		" will open a file requester that will ask for the name"
		" of an LXT dumpfile.  The contents of the dumpfile"
		" generated will be the vcd representation of the traces onscreen"
 		" that can be seen by manipulating the signal and wavewindow scrollbars."
		" The data saved corresponds to the trace information needed"
		" to allow viewing when used in tandem with the corresponding GTKWave save file."
	);
	return;
	}

if(GLOBALS->traces.first)
	{
	if((GLOBALS->is_ghw)&&(0))
		{
		status_text("LXT export not supported for GHW.\n");
		}
		else
		{
		fileselbox("Write LXT File As",&GLOBALS->filesel_lxt_writesave,GTK_SIGNAL_FUNC(menu_write_lxt_file_cleanup), GTK_SIGNAL_FUNC(NULL),"*.lxt", 1);
		}
	}
	else
	{
	status_text("No traces onscreen to save!\n");
	}
}


/******************************************************************/

void menu_write_vcd_file_cleanup(GtkWidget *widget, gpointer data)
{
int rc;

if(!GLOBALS->filesel_ok)
        {
        return;
        }               
                                
if(GLOBALS->lock_menu_c_2 == 1) return; /* avoid recursion */
GLOBALS->lock_menu_c_2 = 1;

status_text("Saving VCD...\n");
gtkwave_main_iteration(); /* make requester disappear requester */

rc = save_nodes_to_export(*GLOBALS->fileselbox_text, WAVE_EXPORT_VCD);

GLOBALS->lock_menu_c_2 = 0;

switch(rc)
	{
	case VCDSAV_EMPTY:	status_text("No traces onscreen to save!\n");
				break;

	case VCDSAV_FILE_ERROR:	status_text("Problem writing VCD: ");
				status_text(strerror(errno));
				break;

	case VCDSAV_OK:		status_text("VCD written successfully.\n");
	default:		break;
	}
}

void
menu_write_vcd_file(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
	{
	help_text_bold("\n\nWrite VCD File As");
	help_text(
		" will open a file requester that will ask for the name"
		" of a VCD dumpfile.  The contents of the dumpfile"
		" generated will be the vcd representation of the traces onscreen"
 		" that can be seen by manipulating the signal and wavewindow scrollbars."
		" The data saved corresponds to the trace information needed"
		" to allow viewing when used in tandem with the corresponding GTKWave save file."
	);
	return;
	}

if(GLOBALS->traces.first)
	{
	fileselbox("Write VCD File As",&GLOBALS->filesel_vcd_writesave,GTK_SIGNAL_FUNC(menu_write_vcd_file_cleanup), GTK_SIGNAL_FUNC(NULL),"*.vcd", 1);
	}
	else
	{
	status_text("No traces onscreen to save!\n");
	}
}

/******************************************************************/

void menu_write_tim_file_cleanup(GtkWidget *widget, gpointer data)
{
int rc;

if(!GLOBALS->filesel_ok)
        {
        return;
        }               
                                
if(GLOBALS->lock_menu_c_2 == 1) return; /* avoid recursion */
GLOBALS->lock_menu_c_2 = 1;

status_text("Saving TIM...\n");
gtkwave_main_iteration(); /* make requester disappear requester */

rc = save_nodes_to_export(*GLOBALS->fileselbox_text, WAVE_EXPORT_TIM);

GLOBALS->lock_menu_c_2 = 0;

switch(rc)
	{
	case VCDSAV_EMPTY:	status_text("No traces onscreen to save!\n");
				break;

	case VCDSAV_FILE_ERROR:	status_text("Problem writing TIM: ");
				status_text(strerror(errno));
				break;

	case VCDSAV_OK:		status_text("TIM written successfully.\n");
	default:		break;
	}
}

void
menu_write_tim_file(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
	{
	help_text_bold("\n\nWrite TIM File As");
	help_text(
		" will open a file requester that will ask for the name"
		" of a TimingAnalyzer .tim file.  The contents of the file"
		" generated will be the representation of the traces onscreen."
		" If the baseline and primary marker are set, the time range"
		" written to the file will be between the two markers, otherwise"
		" it will be the entire time range."
	);
	return;
	}

if(GLOBALS->traces.first)
	{
	fileselbox("Write TIM File As",&GLOBALS->filesel_tim_writesave,GTK_SIGNAL_FUNC(menu_write_tim_file_cleanup), GTK_SIGNAL_FUNC(NULL),"*.tim", 1);
	}
	else
	{
	status_text("No traces onscreen to save!\n");
	}
}


/******************************************************************/

void menu_unwarp_traces_all(GtkWidget *widget, gpointer data)
{
Trptr t;
int found=0;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nUnwarp All");
        help_text(
                " unconditionally removes all offsets on all traces."
        );
        return;
        }

t=GLOBALS->traces.first;
while(t)
	{
	if(t->shift)
		{
		t->shift=LLDescriptor(0);
		found++;
		}
	t=t->t_next;
	}

if(found)
	{
	GLOBALS->signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}

void menu_unwarp_traces(GtkWidget *widget, gpointer data)
{
Trptr t;
int found=0;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nUnwarp Marked");
        help_text(
                " removes all offsets on all highlighted traces."
        );
        return;
        }

t=GLOBALS->traces.first;
while(t)
	{
	if(t->flags&TR_HIGHLIGHT)
		{
		t->shift=LLDescriptor(0);
		t->flags&=(~TR_HIGHLIGHT);
		found++;
		}
	t=t->t_next;
	}

if(found)
	{
	GLOBALS->signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}

void warp_cleanup(GtkWidget *widget, gpointer data)
{
if(GLOBALS->entrybox_text)
	{
	TimeType gt, delta;
	Trptr t;

	gt=unformat_time(GLOBALS->entrybox_text, GLOBALS->time_dimension);
	free_2(GLOBALS->entrybox_text);
	GLOBALS->entrybox_text=NULL;

	if(gt<0)
		{
		delta=GLOBALS->tims.first-GLOBALS->tims.last;
		if(gt<delta) gt=delta;
		}
	else
	if(gt>0)
		{
		delta=GLOBALS->tims.last-GLOBALS->tims.first;
		if(gt>delta) gt=delta;
		}

	t=GLOBALS->traces.first;
	while(t)
	  {
	    if(t->flags&TR_HIGHLIGHT)
	      {
		if(HasWave(t)) /* though note if a user specifies comment warping in a .sav file we will honor it.. */
		  {
		    t->shift=gt;
		  }
		else
		  {
		    t->shift=LLDescriptor(0);
				}
			t->flags&=(~TR_HIGHLIGHT);
			}
		t=t->t_next;
		}
	}

	GLOBALS->signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
}

void menu_warp_traces(GtkWidget *widget, gpointer data)
{
char gt[32];
Trptr t;
int found=0;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nWarp Marked");
        help_text(
                " offsets all highlighted traces by the amount of"
                " time entered in the requester.  (Positive values"
		" will shift traces to the right.)"
		" Attempting to shift greater than the absolute value of total simulation"
		" time will cap the shift magnitude at the length of simulation."
		" Note that you can also warp traces dynamically by holding"
		" down CTRL and dragging a group of highlighted traces to"
		" the left or right with the left mouse button pressed.  When you release"
		" the mouse button, if CTRL is pressed, the drag warp commits, else"
		" it reverts to its pre-drag condition."
        );
        return;
        }


t=GLOBALS->traces.first;
while(t)
	{
	if(t->flags&TR_HIGHLIGHT)
		{
		found++;
		break;
		}
	t=t->t_next;
	}

if(found)
	{
	reformat_time(gt, LLDescriptor(0), GLOBALS->time_dimension);
	entrybox("Warp Traces",200,gt,NULL,20,GTK_SIGNAL_FUNC(warp_cleanup));
	}
}





void wave_scrolling_on(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nWave Scrolling");
        help_text(
		" allows movement of the primary marker beyond screen boundaries"
		" which causes the wave window to scroll when enabled."
		" When disabled, it"
		" disallows movement of the primary marker beyond screen boundaries."
        );
        }
	else
	{
	if(!GLOBALS->wave_scrolling)
		{
		status_text("Wave Scrolling On.\n");
		GLOBALS->wave_scrolling=1;
		}
		else
		{
		status_text("Wave Scrolling Off.\n");
		GLOBALS->wave_scrolling=0;
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_MWSON].path))->active=(GLOBALS->wave_scrolling)?TRUE:FALSE;
}
/**/

void menu_autocoalesce(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nAutocoalesce");
        help_text(
		" when enabled"
		" allows the wave viewer to reconstruct split vectors."
		" Split vectors will be indicated by a \"[]\""
		" prefix in the search requesters."
        );
        }
	else
	{
	if(!GLOBALS->autocoalesce)
		{
		status_text("Autocoalesce On.\n");
		GLOBALS->autocoalesce=1;
		}
		else
		{
		status_text("Autocoalesce Off.\n");
		GLOBALS->autocoalesce=0;
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_ACOL].path))->active=(GLOBALS->autocoalesce)?TRUE:FALSE;
}

void menu_autocoalesce_reversal(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nAutocoalesce Reversal");
        help_text(
		" causes split vectors to be reconstructed in reverse order (only if autocoalesce is also active).  This is necessary with some simulators."
		" Split vectors will be indicated by a \"[]\""
		" prefix in the search requesters."
        );
        }
	else
	{
	if(!GLOBALS->autocoalesce_reversal)
		{
		status_text("Autocoalesce Rvs On.\n");
		GLOBALS->autocoalesce_reversal=1;
		}
		else
		{
		status_text("Autocoalesce Rvs Off.\n");
		GLOBALS->autocoalesce_reversal=0;
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_ACOLR].path))->active=(GLOBALS->autocoalesce_reversal)?TRUE:FALSE;
}

void menu_autoname_bundles_on(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nAutoname Bundles");
        help_text(
		" when enabled"
		" modifies the bundle up/down operations in the hierarchy"
		" and tree searches such that a NULL bundle name is"
		" implicitly created which informs GTKWave to create bundle"
		" and signal names based on the position in the hierarchy."
		" When disabled, it"
		" modifies the bundle up/down operations in the hierarchy"
		" and tree searches such that a NULL bundle name is"
		" not implicitly created.  This informs GTKWave to create bundle"
		" and signal names based on the position in the hierarchy"
		" only if the user enters a zero-length bundle name.  This"
		" behavior is the default."
        );
        }
	else
	{
	if(!GLOBALS->autoname_bundles)
		{
		status_text("Autoname On.\n");
		GLOBALS->autoname_bundles=1;
		}
		else
		{
		status_text("Autoname Off.\n");
		GLOBALS->autoname_bundles=0;
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_ABON].path))->active=(GLOBALS->autoname_bundles)?TRUE:FALSE;
}


void menu_hgrouping(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nSearch Hierarchy Grouping");
        help_text(
		" when enabled ensures that new members added to the ``Tree Search'' and"
		" ``Hierarchy Search'' widgets are added alphanumerically: first hierarchy names as a group followed by signal names as a group."
		" This is the default and is recommended.  When disabled, hierarchy names and signal names are interleaved together in"
		" strict alphanumerical ordering."
		" Note that due to the caching mechanism in ``Tree Search'', dynamically changing this flag when the widget is active "
		" may not produce immediately obvious results.  Closing the widget then opening it up again will ensure that it follows the"
		" behavior of this flag."
        );
        }
	else
	{
	if(!GLOBALS->hier_grouping)
		{
		status_text("Hier Grouping On.\n");
		GLOBALS->hier_grouping=1;
		}
		else
		{
		status_text("Hier Grouping Off.\n");
		GLOBALS->hier_grouping=0;
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_HTGP].path))->active=(GLOBALS->hier_grouping)?TRUE:FALSE;
}


void set_hier_cleanup(GtkWidget *widget, gpointer data, int level)
{
  char update_string[128];
  Trptr t;
  int i;

  GLOBALS->hier_max_level=level;
  if(GLOBALS->hier_max_level<0) GLOBALS->hier_max_level=0;

  for(i=0;i<2;i++)
    {
      if(i==0) t=GLOBALS->traces.first; else t=GLOBALS->traces.buffer;

      while(t)
	{
	  if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
	    {
	      if (HasAlias(t))
		{
		  t->name = t->name_full;
		  if(GLOBALS->hier_max_level)
		    t->name = hier_extract(t->name, GLOBALS->hier_max_level);
		}
	      else if(t->vector==TRUE)
		{
		  t->name = t->n.vec->bvname;
		  if(GLOBALS->hier_max_level)
		    t->name = hier_extract(t->name, GLOBALS->hier_max_level);
		}
	      else
		{
		  if(!GLOBALS->hier_max_level)
		      {
			int flagged = 0;

			if(t->name&&t->is_depacked) { free_2(t->name); }
			t->name = hier_decompress_flagged(t->n.nd->nname, &flagged);
			t->is_depacked = (flagged != 0);
		      }
		    else
		      {
			int flagged = 0;
			char *tbuff;

			if(t->name&&t->is_depacked) { free_2(t->name); }
			tbuff = hier_decompress_flagged(t->n.nd->nname, &flagged);
			t->is_depacked = (flagged != 0);

			if(!flagged)
			  {   
			    t->name = hier_extract(t->n.nd->nname, GLOBALS->hier_max_level);
			  }
			else
			  {   
			    t->name = strdup_2(hier_extract(tbuff, GLOBALS->hier_max_level));
			    free_2(tbuff);
			  }
		      } 
		}
	    }
	  t=t->t_next;
	}
    }

  GLOBALS->signalwindow_width_dirty=1;
  MaxSignalLength();
  signalarea_configure_event(GLOBALS->signalarea, NULL);
  wavearea_configure_event(GLOBALS->wavearea, NULL);
  sprintf(update_string, "Trace Hier Max Depth is now: %d\n", GLOBALS->hier_max_level);
  status_text(update_string);

}

void max_hier_cleanup(GtkWidget *widget, gpointer data)
{
if(GLOBALS->entrybox_text)
  {
    int i;

    i = atoi_64(GLOBALS->entrybox_text);
    set_hier_cleanup(widget, data, i);
    GLOBALS->hier_max_level_shadow = GLOBALS->hier_max_level; /* used for the toggle function */

    free_2(GLOBALS->entrybox_text);
    GLOBALS->entrybox_text=NULL;
  }
}


void menu_set_max_hier(GtkWidget *widget, gpointer data)
{
char za[32];

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nSet Max Hier");
        help_text(
		" sets the maximum hierarchy depth (counting from the right"
		" with bit numbers or ranges ignored) that is displayable"
		" for trace names.  Zero indicates that no truncation will"
		" be performed (default).  Note that any aliased signals"
		" (prefix of a \"+\") will not have truncated names." 
        );
        return;
        }


sprintf(za,"%d",GLOBALS->hier_max_level);

entrybox("Max Hier Depth",200,za,NULL,20,GTK_SIGNAL_FUNC(max_hier_cleanup));
}

void menu_toggle_hier(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nToggle Trace Hier");
        help_text(
		" toggles the maximum hierarchy depth from zero to whatever was previously set."
        );
        return;
        }

 if (GLOBALS->hier_max_level)
   set_hier_cleanup(widget, data, 0);
 else
   set_hier_cleanup(widget, data, GLOBALS->hier_max_level_shadow); /* instead of just '1' */
}


/**/
void menu_use_roundcaps(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nDraw Roundcapped Vectors");
        help_text(
		" draws vector transitions that have sloping edges when enabled."
		" Draws vector transitions that have sharp edges when disabled;"
		" this is the default."
        );
        }
	else
	{
	if(!GLOBALS->use_roundcaps)
		{
		status_text("Using roundcaps.\n");
		GLOBALS->use_roundcaps=1;
		}
		else
		{
		status_text("Using flatcaps.\n");
		GLOBALS->use_roundcaps=0;
		}
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VDRV].path))->active=(GLOBALS->use_roundcaps)?TRUE:FALSE;
}

/**/
void menu_lxt_clk_compress(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nLXT Clock Compress to Z");
        help_text(
		" reduces memory usage when active as clocks compressed in LXT format are"
		" kept at Z in order to save memory.  Traces imported with this are permanently"
		" kept at Z."
        );
        }
	else
	{
	if(GLOBALS->lxt_clock_compress_to_z)
		{
		GLOBALS->lxt_clock_compress_to_z=0;
		status_text("LXT CC2Z Off.\n");
		}
		else
		{
		GLOBALS->lxt_clock_compress_to_z=1;
		status_text("LXT CC2Z On.\n");
		}
	}

if(GLOBALS->loaded_file_type == LXT_FILE)
	{
	GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_LXTCC2Z].path))->active=(GLOBALS->lxt_clock_compress_to_z)?TRUE:FALSE;
	}
}
/**/
void menu_use_full_precision(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nFull Precision");
        help_text(
		" does not round time values when the number of ticks per pixel onscreen is greater than"
		" 10 when active.  The default is that this feature is disabled."
        );
        }
	else
	{
	if(GLOBALS->use_full_precision)
		{
		GLOBALS->use_full_precision=0;
		status_text("Full Prec Off.\n");
		}
		else
		{
		GLOBALS->use_full_precision=1;
		status_text("Full Prec On.\n");
		}

	calczoom(GLOBALS->tims.zoom);
	fix_wavehadj();
                        
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */
	update_maxmarker_labels();
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VFTP].path))->active=(GLOBALS->use_full_precision)?TRUE:FALSE;
}
/**/
void menu_remove_marked(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nRemove Pattern Marks");
        help_text(
		" removes any vertical traces on the display caused by the Mark"
		" feature in pattern search and reverts to the normal format."
        );
        }
	else
	{
	int i;

	WAVE_STRACE_ITERATOR(i)
		{
		GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = i];

		if(GLOBALS->strace_ctx->shadow_straces)
			{
			delete_strace_context();
			}
	
		strace_maketimetrace(0);
		}
  
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}
/**/
void menu_use_color(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nUse Color");
        help_text(
		" draws signal names and trace data in color.  This is normal operation."
        );
        }
	else
	{
	force_normal_gcs();
  
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}
/**/
void menu_use_bw(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nUse Black and White");
        help_text(
		" draws signal names and trace data in black and white.  This is intended for use in"
		" black and white screen dumps."
        );
        }
	else
	{
	force_screengrab_gcs();
  
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}
/**/
void menu_zoom10_snap(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nZoom Pow10 Snap");
        help_text(
		" snaps time values to a power of ten boundary when active.  Fractional zooms are"
		" internally stored, but what is actually displayed will be rounded up/down to the"
		" nearest power of 10.  This only works when the ticks per frame is greater than 100"
		" units."
        );
        }
	else
	{
	if(GLOBALS->zoom_pow10_snap)
		{
		GLOBALS->zoom_pow10_snap=0;
		status_text("Pow10 Snap Off.\n");
		}
		else
		{
		GLOBALS->zoom_pow10_snap=1;
		status_text("Pow10 Snap On.\n");
		}

	calczoom(GLOBALS->tims.zoom);
	fix_wavehadj();
                        
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VZPS].path))->active=(GLOBALS->zoom_pow10_snap)?TRUE:FALSE;
}

/**/
void menu_zoom_dynf(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nPartial VCD Dynamic Zoom Full");
        help_text(
		" causes the screen to be in full zoom mode while a VCD file is loading"
		" incrementally."
        );
        }
	else
	{
	if(GLOBALS->zoom_dyn)
		{
		GLOBALS->zoom_dyn=0;
		status_text("Dynamic Zoom Full Off.\n");
		}
		else
		{
		GLOBALS->zoom_dyn=1;
		status_text("Dynamic Zoom Full On.\n");
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VZDYN].path))->active=(GLOBALS->zoom_dyn)?TRUE:FALSE;
}

/**/
void menu_zoom_dyne(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nPartial VCD Dynamic Zoom To End");
        help_text(
		" causes the screen to zoom to the end while a VCD file is loading"
		" incrementally."
        );
        }
	else
	{
	if(GLOBALS->zoom_dyne)
		{
		GLOBALS->zoom_dyne=0;
		status_text("Dynamic Zoom To End Off.\n");
		}
		else
		{
		GLOBALS->zoom_dyne=1;
		status_text("Dynamic Zoom To End On.\n");
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VZDYNE].path))->active=(GLOBALS->zoom_dyne)?TRUE:FALSE;
}

/**/
void menu_left_justify(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nLeft Justify Signals");
        help_text(
		" draws signal names flushed to the left border of the signal window."
        );
        }
	else
	{
	status_text("Left Justification.\n");
	GLOBALS->left_justify_sigs=~0;
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	}
}

/**/
void menu_right_justify(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nRight Justify Signals");
        help_text(
		" draws signal names flushed to the right (\"equals\") side of the signal window."
        );
        }
	else
	{
	status_text("Right Justification.\n");
	GLOBALS->left_justify_sigs=0;
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	}
}

/**/
void menu_enable_constant_marker_update(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nConstant Marker Update");
        help_text(
		" when enabled,"
		" allows GTKWave to dynamically show the changing values of the"
		" traces under the primary marker while it is being dragged"
		" across the screen.  This works best with dynamic resizing disabled."
		" When disabled, it"
		" restricts GTKWave to only update the trace values when the"
		" left mouse button is initially pressed then again when it is released."
		" This is the default behavior."
        );
        }
	else
	{
	if(!GLOBALS->constant_marker_update)
		{
		status_text("Constant marker update enabled.\n");
		GLOBALS->constant_marker_update=~0;
		}
		else
		{
		status_text("Constant marker update disabled.\n");
		GLOBALS->constant_marker_update=0;
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VCMU].path))->active=(GLOBALS->constant_marker_update)?TRUE:FALSE;
}
/**/
void menu_enable_dynamic_resize(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nDynamic Resize");
        help_text(
		" allows GTKWave to dynamically resize the signal"
		" window for you when toggled active.  This can be helpful during numerous"
		" signal additions and/or deletions.  This is the default"
		" behavior."
        );
        }
	else
	{
	if(!GLOBALS->do_resize_signals)
		{
		status_text("Resizing enabled.\n");
		GLOBALS->do_resize_signals=~0;
		}
		else
		{
		status_text("Resizing disabled.\n");
		GLOBALS->do_resize_signals=0;
		}
	GLOBALS->signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VDR].path))->active=(GLOBALS->do_resize_signals)?TRUE:FALSE;
}
/**/
void menu_toggle_delta_or_frequency(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nToggle Delta-Frequency");
        help_text(
		" allows you to switch between the delta time and"
		" frequency display in the upper right corner"
		" of the main window when measuring distances between markers.  Default behavior is that the"
		" delta time is displayed."
        );
        }
	else
	{
	GLOBALS->use_frequency_delta=(GLOBALS->use_frequency_delta)?0:1;
	update_maxmarker_labels();
	}
}
/**/
void menu_toggle_max_or_marker(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nToggle Max-Marker");
        help_text(
		" allows you to switch between the maximum time and"
		" marker time for display in the upper right corner"
		" of the main window.  Default behavior is that the"
		" maximum time is displayed."
        );
        }
	else
	{
	GLOBALS->use_maxtime_display=(GLOBALS->use_maxtime_display)?0:1;
	update_maxmarker_labels();
	}
}
/**/
void menu_help(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nWave Help");
        help_text(
		" is already active.  It's this window."
        );
        return;
        }

helpbox("Wave Help",300,"Select any main window menu item");
}
/**/
void menu_version(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nWave Version");
        help_text(
                " merely brings up a requester which indicates the current"
		" version of this program."
        );
        return;
        }

simplereqbox("Wave Version",400,WAVE_VERSION_INFO,"OK", NULL, NULL, 0);
}
/**/
void menu_quit_callback(GtkWidget *widget, gpointer data)
{
char sstr[32];

if(data)
	{
#ifdef __CYGWIN__
	kill_stems_browser();
#endif	
	g_print("Exiting.\n");	
	sprintf(sstr, "%d", GLOBALS->this_context_page);
	gtkwavetcl_setvar(WAVE_TCLCB_QUIT_PROGRAM, sstr, WAVE_TCLCB_QUIT_PROGRAM_FLAGS);

	gtk_exit(0);
	}
}
void menu_quit(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
	{
	help_text_bold("\n\nQuit");
	help_text(
		" closes GTKWave and exits immediately."
	);
	return;
	}

if(!GLOBALS->enable_fast_exit)
	{
	simplereqbox("Quit Program",300,"Do you really want to quit?","Yes", "No", GTK_SIGNAL_FUNC(menu_quit_callback), 1);
	}
	else
	{
	menu_quit_callback(NULL, &GLOBALS->enable_fast_exit); /* nonzero dummy arg */
	}
}

/**/

void menu_quit_close_callback(GtkWidget *widget, gpointer dummy_data)
{
unsigned int i, j=0;
unsigned int this_page = GLOBALS->this_context_page;
unsigned np = GLOBALS->num_notebook_pages;
unsigned int new_page = (this_page != np-1) ? this_page : (this_page-1);
GtkWidget *n = GLOBALS->notebook;
struct Global *old_g = NULL, *saved_g;
char sstr[32];

sprintf(sstr, "%d", this_page);
gtkwavetcl_setvar(WAVE_TCLCB_CLOSE_TAB_NUMBER, sstr, WAVE_TCLCB_CLOSE_TAB_NUMBER_FLAGS);

kill_stems_browser_single(GLOBALS);
dead_context_sweep();

for(i=0;i<np;i++)
	{
	if(i!=this_page)
		{
		(*GLOBALS->contexts)[j] = (*GLOBALS->contexts)[i];
		(*GLOBALS->contexts)[j]->this_context_page = j;
		(*GLOBALS->contexts)[j]->num_notebook_pages--;

		j++;
		}
		else
		{
		old_g = (*GLOBALS->contexts)[j];
		}
	}
(*GLOBALS->contexts)[j] = old_g;
	
gtk_notebook_set_show_tabs(GTK_NOTEBOOK(n), (np>2)); 
gtk_notebook_set_show_border(GTK_NOTEBOOK(n), (np>2)); 

gtk_notebook_remove_page(GTK_NOTEBOOK(n), this_page);
gtk_notebook_set_current_page(GTK_NOTEBOOK(n), new_page);

set_GLOBALS((*GLOBALS->contexts)[new_page]);
saved_g = GLOBALS;

gtkwave_main_iteration();

set_GLOBALS(old_g);
free_and_destroy_page_context();
set_GLOBALS(saved_g);

/* need to do this if 2 pages -> 1 */
reformat_time(sstr, GLOBALS->tims.first, GLOBALS->time_dimension);
gtk_entry_set_text(GTK_ENTRY(GLOBALS->from_entry),sstr);
reformat_time(sstr, GLOBALS->tims.last, GLOBALS->time_dimension);
gtk_entry_set_text(GTK_ENTRY(GLOBALS->to_entry),sstr);
update_maxmarker_labels();
update_basetime(GLOBALS->tims.baseline);

gtk_window_set_title(GTK_WINDOW(GLOBALS->mainwindow), GLOBALS->winname);
 
MaxSignalLength();   
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}

void menu_quit_close(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nClose");
        help_text(
		" immediately closes the current tab if multiple tabs exist or"
                " exits GTKWave after an additional confirmation"
                " requester is given the OK to quit."
        );
        return;
        } 

if((GLOBALS->num_notebook_pages < 2) && (!GLOBALS->enable_fast_exit))
	{
	simplereqbox("Quit Program",300,"Do you really want to quit?","Yes", "No", GTK_SIGNAL_FUNC(menu_quit_callback), 1);
	}
	else
	{
	if(GLOBALS->num_notebook_pages < 2)
		{
		menu_quit_callback(NULL, &GLOBALS->num_notebook_pages); /* nonzero dummy arg */
		}
		else
		{
		menu_quit_close_callback(NULL, NULL); /* dummy arg, not needed to be nonzero */
		}
	}
}

/**/
void must_sel(void)
{
  status_text("Select one or more traces.\n");
}
static void must_sel_nb(void)
{
  status_text("Select one or more nonblank traces.\n");
}
static void must_sel_bg(void)
{
  status_text("Select a bundle or group.\n");
}
/**/

static void
menu_open_group(GtkWidget *widget, gpointer data)
{
  Trptr t;
  unsigned dirty = 0;

  /* currently only called by toggle menu option, so no help menu text */

 if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

 t=GLOBALS->traces.first;
 while(t)
   {
     if((t->flags&TR_HIGHLIGHT)&&(IsGroupBegin(t) || IsGroupEnd(t)))
       {
	 dirty=1;
	 break;
       }
     t=t->t_next;
   }

 if(dirty)
   {
     OpenTrace(t);
     GLOBALS->signalwindow_width_dirty=1;
     MaxSignalLength();
     signalarea_configure_event(GLOBALS->signalarea, NULL);
     wavearea_configure_event(GLOBALS->wavearea, NULL);
   }
 else
   {
     must_sel_bg();
   }
}


static void
menu_close_group(GtkWidget *widget, gpointer data)
{
  Trptr t;
  unsigned dirty = 0;

  /* currently only called by toggle menu option, so no help menu text */

 if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

 t=GLOBALS->traces.first;
 while(t)
   {
     if((t->flags&TR_HIGHLIGHT)&&(IsGroupBegin(t) || IsGroupEnd(t)))
       {
	 dirty=1;
	 break;
       }
     t=t->t_next;
   }

 if(dirty)
   {

     CloseTrace(t);
     GLOBALS->signalwindow_width_dirty=1;
     MaxSignalLength();
     signalarea_configure_event(GLOBALS->signalarea, NULL);
     wavearea_configure_event(GLOBALS->wavearea, NULL);
   }
 else
   {
     must_sel_bg();
   }
}




static unsigned create_group (char* name, Trptr t_composite)
{
  Trptr t, t_prev, t_begin, t_end;
  unsigned dirty = 0;

 if(!name) name = "Group"; /* generate anonymous name */

 t=GLOBALS->traces.first;
 while(t)
   {
     if(t->flags&TR_HIGHLIGHT)
       {
	 dirty=1;
	 break;
       }
     t=t->t_next;
   }

 if(dirty)
   {

     t_prev = t->t_prev;

     CutBuffer();

     if (t_composite)
       {
	 t_begin = t_composite;
	 t_begin->flags |=TR_GRP_BEGIN;
       }
     else 
       {
	 if( (t_begin = (Trptr) calloc_2( 1, sizeof( TraceEnt ))) == NULL )
	   {
	     fprintf( stderr, "Out of memory, can't add trace.\n");
	     return(0);
	   }
     
	 t_begin->flags = (TR_BLANK|TR_GRP_BEGIN);
	 t_begin->name = (char *)malloc_2(1+strlen(name));
	 strcpy(t_begin->name, name);
       }

     GLOBALS->traces.buffer->t_prev = t_begin;
     t_begin->t_next = GLOBALS->traces.buffer;
     GLOBALS->traces.buffer = t_begin;
     GLOBALS->traces.buffercount++;

     if( (t_end = (Trptr) calloc_2( 1, sizeof( TraceEnt ))) == NULL )
       {
	 fprintf( stderr, "Out of memory, can't add trace.\n");
	 return(0);
       }

     t_end->flags = (TR_BLANK|TR_GRP_END);
    
     if (t_composite)
       {
	 /* make the group end trace invisible */
	 t_end->flags |= TR_COLLAPSED;
	 t_end->name = (char *)malloc_2(1+strlen("group_end"));
	 strcpy(t_end->name, "group_end");
       }
     else
       {
	 t_end->name = (char *)malloc_2(1+strlen(name));
	 strcpy(t_end->name, name);
       }

     GLOBALS->traces.bufferlast->t_next = t_end;
     t_end->t_prev = GLOBALS->traces.bufferlast;
     GLOBALS->traces.bufferlast = t_end;
     GLOBALS->traces.buffercount++;

     t_begin->t_match = t_end;
     t_end->t_match   = t_begin;

     if (t_prev) 
       {
	 t_prev->flags |= TR_HIGHLIGHT;
	 PasteBuffer();
       }
     else
       {
	 PrependBuffer();
       }
   }
 return dirty;
}


static void 
create_group_cleanup(GtkWidget *widget, gpointer data)
{
  unsigned dirty = 0;

  dirty = create_group(GLOBALS->entrybox_text, NULL);

  if (!dirty)
    	{
      	must_sel_bg();
    	}
    	else
	{
      	GLOBALS->signalwindow_width_dirty=1;
      	MaxSignalLength();
      	signalarea_configure_event(GLOBALS->signalarea, NULL);
      	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}



void 
menu_create_group(GtkWidget *widget, gpointer data)
{

  Trptr t;
  unsigned dirty = 0;

  if(GLOBALS->helpbox_is_active)
    {
      help_text_bold("\n\nCreate Group");
      help_text(
		" creates a group of traces which may be opened or closed."
		" It is permitted for groups to be nested."
		);
      return;
    }

  t=GLOBALS->traces.first;
  while(t)
    {
      if(t->flags&TR_HIGHLIGHT)
       {
	 dirty=1;
	 break;
       }
      t=t->t_next;
    }

  if(dirty)
    {
      /* don't mess with sigs when dnd active */
      if(GLOBALS->dnd_state) { dnd_error(); return; } 
      entrybox("Create Group",300,"","Enter group name:",128,GTK_SIGNAL_FUNC(create_group_cleanup));
    }
  else
    {
      must_sel_bg();
    }
}


static unsigned expand_trace(Trptr t_top)
{

  Trptr t, tmp;
  int tmpi;
  unsigned dirty = 0;

  t = t_top;

  if(HasWave(t) && !IsGroupBegin(t) && !IsGroupEnd(t))
    {
      FreeCutBuffer();
      GLOBALS->traces.buffer=GLOBALS->traces.first;
      GLOBALS->traces.bufferlast=GLOBALS->traces.last;
      GLOBALS->traces.buffercount=GLOBALS->traces.total;

      GLOBALS->traces.first=GLOBALS->traces.last=NULL; GLOBALS->traces.total=0;

      if(t->vector)
	{
	  bptr bits;	
	  int i;
	  Trptr tfix;
	  TimeType otime = t->shift;

	  bits=t->n.vec->bits;
	  if(!(t->flags&TR_REVERSE))
	    {
	      for(i=0;i<bits->nnbits;i++)
		{
		  if(bits->nodes[i]->expansion) bits->nodes[i]->expansion->refcnt++;
		  AddNodeTraceReturn(bits->nodes[i],NULL, &tfix);
		  if(bits->attribs)
		    {
		      tfix->shift = otime + bits->attribs[i].shift;
		    }
		}
	    }
	  else
	    {
	      for(i=(bits->nnbits-1);i>-1;i--)
		{
		  if(bits->nodes[i]->expansion) bits->nodes[i]->expansion->refcnt++;
		  AddNodeTraceReturn(bits->nodes[i],NULL, &tfix);
		  if(bits->attribs)
		    {
		      tfix->shift = otime + bits->attribs[i].shift;
		    }
		}
	    }
	}
      else
	{
	  eptr e=ExpandNode(t->n.nd);
	  int i;
	  if(!e)
	    {
	      /* 		      if(t->n.nd->expansion) t->n.nd->expansion->refcnt++; */
	      /* 		      AddNode(t->n.nd,NULL); */
	    }
	  else
	    {
	      int dhc_sav = GLOBALS->do_hier_compress;
	      GLOBALS->do_hier_compress = 0;
	      for(i=0;i<e->width;i++)
		{
		  AddNode(e->narray[i], NULL);	
		}
	      GLOBALS->do_hier_compress = dhc_sav;
	      free_2(e->narray);
	      free_2(e);
	    }
	}

      tmp=GLOBALS->traces.buffer; GLOBALS->traces.buffer=GLOBALS->traces.first; GLOBALS->traces.first=tmp;
      tmp=GLOBALS->traces.bufferlast; GLOBALS->traces.bufferlast=GLOBALS->traces.last; GLOBALS->traces.last=tmp;
      tmpi=GLOBALS->traces.buffercount; GLOBALS->traces.buffercount=GLOBALS->traces.total;
      GLOBALS->traces.total=tmpi;

      if (GLOBALS->traces.buffercount > 0) 
	{

	  /* buffer now contains the created signals */
	  ClearTraces();

	  if (t_top->t_prev)
	    {
	      t_top->t_prev->flags |= TR_HIGHLIGHT;
	      RemoveTrace(t_top, 0);
	      PasteBuffer();
	      t_top->t_prev->flags &= ~TR_HIGHLIGHT;
	    }
	  else 
	    {
	      RemoveTrace(t_top, 0);
	      PrependBuffer();
	    }

	  dirty = create_group("unused_2", t_top);
	
	}
    }

  return dirty;
}

void
menu_expand(GtkWidget *widget, gpointer data)
{
  Trptr t, t_next;
  int dirty=0;
  int j;
  GtkAdjustment *wadj;

  if(GLOBALS->helpbox_is_active)
    {
      help_text_bold("\n\nExpand");
      help_text(
		" decomposes the highlighted signals into their individual bits."
		" The resulting bits are converted to traces and inserted after the"
		" last highlighted trace.  The original unexpanded traces will"
		" be placed in the cut buffer."
		" It will function seemingly randomly"
		" when used upon real valued single-bit traces."
		" When used upon multi-bit vectors that contain "
		" real valued traces, those traces will expand to their normal \"correct\" values,"
		" not individual bits."
		);
      return;
    }


  if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

  DEBUG(printf("Expand Traces\n"));

  t=GLOBALS->traces.first;
  while(t)
    {
      if(IsSelected(t) && HasWave(t))
	{
	  t->flags |= TR_COLLAPSED;
	  dirty=1;
	}
      t=t->t_next;
    }

  if(dirty)
    {
      ClearTraces();
      t=GLOBALS->traces.first;
      while(t)
	{
	  t_next = t->t_next;
	  if(HasWave(t) && (t->flags&TR_COLLAPSED))
	    {
	      if (!IsGroupBegin(t) && !IsGroupEnd(t))
		{
		  expand_trace(t);
		}
	      else
		{
		  OpenTrace(t);
		}
	    }
	  t=t_next;
	}
      t=GLOBALS->traces.first;
      t->t_grp = NULL;
      while(t)
	{
	  if(HasWave(t) && (t->flags&TR_COLLAPSED))
	    {
	      t->flags &= ~TR_COLLAPSED;
      	      t->flags |= TR_HIGHLIGHT;
	    }

	  updateTraceGroup(t);
	  t=t->t_next;
	}

      t=GLOBALS->traces.first;
      while(t)
	{
	  if (IsSelected(t)) { break; }
	  t=t->t_next;
	}

      j = GetTraceNumber(t);
      wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
      if (j < wadj->value)
	{
	  SetTraceScrollbarRowValue(j, 0);
	}

      GLOBALS->signalwindow_width_dirty=1;
      MaxSignalLength();
      signalarea_configure_event(GLOBALS->signalarea, NULL);
      wavearea_configure_event(GLOBALS->wavearea, NULL);
    }
  else
    {
      must_sel_nb();
    }
}

void
menu_toggle_group(GtkWidget *widget, gpointer data)
{
  Trptr t;
  unsigned dirty_group  = 0;
  unsigned dirty_signal = 0;

  if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nToggle Group");
        help_text(
		" toggles a group opened or closed."
        );
        return;
        }


 if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

 t=GLOBALS->traces.first;
 while(t)
   {
     if((t->flags&TR_HIGHLIGHT)&&(IsGroupBegin(t) || IsGroupEnd(t)))
       {
	 dirty_group=1;
	 break;
       }

     if((t->flags&TR_HIGHLIGHT)&&HasWave(t))
       {
	 dirty_signal=1;
	 break;
       }
     t=t->t_next;
   }

 if(dirty_group)
   {
     if(IsClosed(t))
       {
	 menu_open_group(widget, data);
	 gtkwavetcl_setvar(WAVE_TCLCB_OPEN_TRACE_GROUP, t->name, WAVE_TCLCB_OPEN_TRACE_GROUP_FLAGS);
       }
     else
       {
	 menu_close_group(widget, data);
	 gtkwavetcl_setvar(WAVE_TCLCB_CLOSE_TRACE_GROUP, t->name, WAVE_TCLCB_CLOSE_TRACE_GROUP_FLAGS);
       }
     return;
   }
 if(dirty_signal)
   {
     ClearTraces();
     t->flags |= TR_HIGHLIGHT;
     menu_expand(widget, data);
     gtkwavetcl_setvar(WAVE_TCLCB_OPEN_TRACE_GROUP, t->name, WAVE_TCLCB_OPEN_TRACE_GROUP_FLAGS);
     return;
   }

 must_sel_bg();
}

static void rename_cleanup(GtkWidget *widget, gpointer data)
{
  Trptr t = GLOBALS->trace_to_alias_menu_c_1;

  if(GLOBALS->entrybox_text)
    {
      char *efix;
      /* code to turn '{' and '}' into '[' and ']' */
      if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
	{
	  efix=GLOBALS->entrybox_text;
	  while(*efix)
	    {
	      if(*efix=='{')
		{
		  *efix='[';
		}
	      if(*efix=='}')
		{
		  *efix=']';
		}
	      efix++;
	    }
	}

      if(t->vector)
	{
	  if(t->name_full)
	    {
	      free_2(t->name_full);
	      t->name_full = NULL;
	    }
	  if (t->n.vec->bvname)
	    {
	      free_2(t->n.vec->bvname);
	    }

	  t->n.vec->bvname = (char *)malloc_2(1+strlen(GLOBALS->entrybox_text));
	  strcpy(t->n.vec->bvname, GLOBALS->entrybox_text);
	  t->name = t->n.vec->bvname;
	  if(GLOBALS->hier_max_level)
	    t->name = hier_extract(t->name, GLOBALS->hier_max_level);

	  t->flags&= ~TR_HIGHLIGHT;

	}

      GLOBALS->signalwindow_width_dirty=1;
      MaxSignalLength();
      signalarea_configure_event(GLOBALS->signalarea, NULL);
      wavearea_configure_event(GLOBALS->wavearea, NULL);
    }
}


static void menu_rename(GtkWidget *widget, gpointer data)
{
  Trptr t;
  /* currently only called by various combine menu options, so no help menu text */

  GLOBALS->trace_to_alias_menu_c_1=NULL;

  /* don't mess with sigs when dnd active */
  if(GLOBALS->dnd_state) { dnd_error(); return; } 

  t = GLOBALS->traces.first;
  while(t)
    {
      if(IsSelected(t)&&(t->vector==TRUE))
	{
	  GLOBALS->trace_to_alias_menu_c_1 = t;
	  break;
	}
      t=t->t_next;
    }

  if(GLOBALS->trace_to_alias_menu_c_1)
    {
      int was_packed = 0;
      char* current = GetFullName(GLOBALS->trace_to_alias_menu_c_1, &was_packed);
      ClearTraces();
      GLOBALS->trace_to_alias_menu_c_1->flags |= TR_HIGHLIGHT;
      entrybox("Trace Name",300,current,NULL,128,GTK_SIGNAL_FUNC(rename_cleanup));
      if(was_packed) { free_2(current); }
    }
  else
    {
      must_sel();
    }
}


bvptr combine_traces(int direction, Trptr single_trace_only)
{
  Trptr t, tmp;
  int tmpi,dirty=0, attrib_reqd=0;
  nptr bitblast_parent;
  int bitblast_delta=0;

  DEBUG(printf("Combine Traces\n"));

  t=single_trace_only ? single_trace_only : GLOBALS->traces.first;
  while(t)
    {
      if((t->flags&TR_HIGHLIGHT)&&(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH))))
	{
	  if(t->vector)
	    {
	      dirty+=t->n.vec->nbits;
	    }
	  else
	    {
	      if(t->n.nd->extvals)
		{
		  int msb, lsb, width;
		  msb = t->n.nd->msi;
		  lsb = t->n.nd->lsi;
		  if(msb>lsb) width = msb-lsb+1; else width = lsb-msb+1;
		  dirty += width;
		}
	      else
		{
		  dirty++;
		}
	    }
	}
      if(t == single_trace_only) break;
      t=t->t_next;
    }

  if(!dirty)
    {
      if(!single_trace_only) must_sel_nb();
      return NULL;
    }
  if(dirty>BITATTRIBUTES_MAX)
    {
      char buf[128];

      if(!single_trace_only)
		{
	      	sprintf(buf, "%d bits selected, please use <= %d.\n", dirty, BITATTRIBUTES_MAX);
	      	status_text(buf);
		}
      return NULL;
    }
  else
    {
      int i,nodepnt=0;
      struct Node *n[BITATTRIBUTES_MAX];
      struct BitAttributes ba[BITATTRIBUTES_MAX];
      struct Bits *b=NULL;
      bvptr v=NULL;

      if(!single_trace_only)
	{
      	FreeCutBuffer();
      	GLOBALS->traces.buffer=GLOBALS->traces.first;
      	GLOBALS->traces.bufferlast=GLOBALS->traces.last;
      	GLOBALS->traces.buffercount=GLOBALS->traces.total;

      	GLOBALS->traces.first=GLOBALS->traces.last=NULL; GLOBALS->traces.total=0;

      	t=GLOBALS->traces.buffer;
	}
	else
	{
	t = single_trace_only;
	}

      while(t)
	{
	  if(t->flags&TR_HIGHLIGHT)
	    {
	      if(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH))
		{
		  /* nothing */
		}
	      else
		{
		  if(t->vector)
		    {
		      int ix;
		      bptr bits = t->n.vec->bits;	
		      baptr oldba = bits ? bits->attribs : NULL;

		      bits=t->n.vec->bits;

		      if(!(t->flags&TR_REVERSE))
			{
			  for(ix=0;ix<bits->nnbits;ix++)
			    {
			      if(bits->nodes[ix]->expansion) bits->nodes[ix]->expansion->refcnt++;
			      ba[nodepnt].shift = t->shift + (oldba ? oldba[ix].shift : 0);
			      ba[nodepnt].flags = t->flags ^ (oldba ? oldba[ix].flags&TR_INVERT : 0);
			      n[nodepnt++]=bits->nodes[ix];
			    }
			}
		      else
			{
			  for(ix=(bits->nnbits-1);ix>-1;ix--)
			    {
			      if(bits->nodes[ix]->expansion) bits->nodes[ix]->expansion->refcnt++;
			      ba[nodepnt].shift = t->shift + (oldba ? oldba[ix].shift : 0);
			      ba[nodepnt].flags = t->flags ^ (oldba ? oldba[ix].flags&TR_INVERT : 0);
			      n[nodepnt++]=bits->nodes[ix];
			    }
			}
		    }
		  else
		    {
		      eptr e=ExpandNode(t->n.nd);
		      int ix;
		      if(!e)
			{
			  if(t->n.nd->expansion) t->n.nd->expansion->refcnt++;
			  ba[nodepnt].shift = t->shift;
			  ba[nodepnt].flags = t->flags;
			  n[nodepnt++]=t->n.nd;
			}
		      else
			{
			  for(ix=0;ix<e->width;ix++)
			    {
			      ba[nodepnt].shift = t->shift;
			      ba[nodepnt].flags = t->flags;
			      n[nodepnt++]=e->narray[ix];	
			      e->narray[ix]->expansion->refcnt++;
			    }
			  free_2(e->narray);
			  free_2(e);
			}
		    }
		}
	    }
	  if(nodepnt==dirty) break;
	  if(t == single_trace_only) break;
	  t=t->t_next;
	}

      b=(struct Bits *)calloc_2(1,sizeof(struct Bits)+(nodepnt-1)*
				sizeof(struct Node *));

      b->attribs = malloc_2(nodepnt * sizeof(struct BitAttributes));
      for(i=0;i<nodepnt;i++)	/* for up combine we need to reverse the attribs list! */
	{
	  if(direction)
	    {
	      memcpy(b->attribs+i, ba+i, sizeof(struct BitAttributes));
	    }
	  else
	    {
	      memcpy(b->attribs+i, ba+(nodepnt-1-i), sizeof(struct BitAttributes));
	    }

	  if((ba[i].shift)||(ba[i].flags&TR_INVERT))	/* timeshift/invert are only relevant flags */
	    {
	      attrib_reqd = 1;
	    }
	}

      if(!attrib_reqd)
	{
	  free_2(b->attribs);
	  b->attribs = NULL;
	}

      if(n[0]->expansion)
	{
	  bitblast_parent = n[0]->expansion->parent;
	}
      else
	{
	  bitblast_parent = NULL;
	}

      if(direction)
	{
	  for(i=0;i<nodepnt;i++)
	    {
	      b->nodes[i]=n[i];
	      if(n[i]->expansion)
		{
		  if(bitblast_parent != n[i]->expansion->parent) 
		    {
		      bitblast_parent=NULL;
		    }
		  else
		    {
		      if(i==1)
			{
			  bitblast_delta = n[1]->expansion->actual - n[0]->expansion->actual;
			  if(bitblast_delta<-1) bitblast_delta=0;
			  else if(bitblast_delta>1) bitblast_delta=0;
			}
		      else if((bitblast_delta)&&(i>1))
			{
			  if((n[i]->expansion->actual - n[i-1]->expansion->actual) != bitblast_delta) bitblast_delta=0;
			}
		    }
		}
	      else
		{
		  bitblast_parent = NULL;
		}
	    }
	}
      else
	{
	  int rev;
	  rev=nodepnt-1;
	  for(i=0;i<nodepnt;i++)
	    {
	      b->nodes[i]=n[rev--];
	      if(n[i]->expansion)
		{
		  if(bitblast_parent != n[i]->expansion->parent) 
		    {
		      bitblast_parent=NULL;
		    }
		  else
		    {
		      if(i==1)
			{
			  bitblast_delta = n[1]->expansion->actual - n[0]->expansion->actual;
			  if(bitblast_delta<-1) bitblast_delta=0;
			  else if(bitblast_delta>1) bitblast_delta=0;
			}
		      else if((bitblast_delta)&&(i>1))
			{
			  if((n[i]->expansion->actual - n[i-1]->expansion->actual) != bitblast_delta) bitblast_delta=0;
			}
		    }
		}
	      else
		{
		  bitblast_parent = NULL;
		}
	    }
	}

      b->nnbits=nodepnt;

      if(!bitblast_parent)
	{
	  char *aname;
	  int match_iter = 1;

	  if(direction)
	    {
	      aname = attempt_vecmatch(n[0]->nname, n[nodepnt-1]->nname);
	    }
	  else
	    {
	      aname = attempt_vecmatch(n[nodepnt-1]->nname, n[0]->nname);
	    }

	  if(aname)
	    {
	      int ix;

	      for(ix=0;ix<nodepnt-1;ix++)
		{
		  char *mat = attempt_vecmatch(n[0]->nname, n[ix]->nname);
		  if(!mat) { match_iter = 0; break; } else { free_2(mat); }
		}
	    }

	  if(!match_iter)
	    {
	      free_2(aname);
	      aname = NULL;
	    }

	  if(!b->attribs)
	    {
	      if(aname)
		{
		  b->name = aname;
		}
	      else
		{
		  strcpy(b->name=(char *)malloc_2(strlen("<Vector>")+1),"<Vector>");
		}
	    }
	  else
	    {
	      if(aname)
		{
		  b->name = aname;
		}
	      else
		{
		  strcpy(b->name=(char *)malloc_2(strlen("<ComplexVector>")+1),"<ComplexVector>");
		}
	    }
	}
      else
	{
	  int ix, offset;
	  char *nam;
	  char *namex;
	  int was_packed = 0;

	  namex = hier_decompress_flagged(n[0]->nname, &was_packed);

	  offset = strlen(namex);
	  for(ix=offset-1;ix>=0;ix--)
	    {
	      if(namex[ix]=='[') break;
	    }
	  if(ix>-1) offset=ix;
	
	  nam=(char *)wave_alloca(offset+40);
	  memcpy(nam, namex, offset);
	  if(was_packed) { free_2(namex); }

	  if(direction)
	    {
	      sprintf(nam+offset, "[%d%s%d]", n[0]->expansion->actual, (bitblast_delta!=0) ? ":" : "|", n[nodepnt-1]->expansion->actual);
	    }
	  else
	    {
	      sprintf(nam+offset, "[%d%s%d]", n[nodepnt-1]->expansion->actual,  (bitblast_delta!=0) ? ":" : "|", n[0]->expansion->actual);
	    }
	
	  strcpy(b->name=(char *)malloc_2(offset + strlen(nam+offset)+1), nam);
	  DEBUG(printf("Name is: '%s'\n", nam));
	}

      if((v=bits2vector(b)))
	{
 	  v->bits=b;      /* only needed for savefile function */
	}
      else
	{
	  free_2(b->name);
	  if(b->attribs) free_2(b->attribs);
	  free_2(b);
	  return NULL;
	}

      if(!single_trace_only)
	{
      	tmp=GLOBALS->traces.buffer; GLOBALS->traces.buffer=GLOBALS->traces.first; GLOBALS->traces.first=tmp;
      	tmp=GLOBALS->traces.bufferlast; GLOBALS->traces.bufferlast=GLOBALS->traces.last; GLOBALS->traces.last=tmp;
      	tmpi=GLOBALS->traces.buffercount; GLOBALS->traces.buffercount=GLOBALS->traces.total;
      	GLOBALS->traces.total=tmpi;
      	PasteBuffer();
	}

      return v;
    }
}

void
menu_combine_down(GtkWidget *widget, gpointer data)
{
  bvptr v;

  if(GLOBALS->helpbox_is_active)
    {
      help_text_bold("\n\nCombine Down");
      help_text(
                " coalesces the highlighted signals into a single vector named"
		" \"<Vector>\" in a top to bottom fashion"
                " placed after the last highlighted trace.  The original traces will"
                " be placed in the cut buffer."
		" It will function seemingly randomly"
		" when used upon real valued single-bit traces."
		);
      return;
    }

  if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */
  v = combine_traces(1, NULL); /* down */

  if (v)
    {
      Trptr t;

      AddVector(v, NULL);
      free_2(v->bits->name);
      v->bits->name=NULL;

      t = GLOBALS->traces.last;

      RemoveTrace(t, 0);

      /* t is now the composite signal trace */

      create_group("unused_0", t);

      GLOBALS->signalwindow_width_dirty=1;
      MaxSignalLength();
      signalarea_configure_event(GLOBALS->signalarea, NULL);
      wavearea_configure_event(GLOBALS->wavearea, NULL);

      menu_rename(widget, data);
    }
  else
    {
    }
}

void
menu_combine_up(GtkWidget *widget, gpointer data)
{
  bvptr v;

  if(GLOBALS->helpbox_is_active)
    {
      help_text_bold("\n\nCombine Up");
      help_text(
                " coalesces the highlighted signals into a single vector named"
                " \"<Vector>\" in a bottom to top fashion"
                " placed after the last highlighted trace.  The original traces will"
                " be placed in the cut buffer."
		" It will function seemingly randomly"
		" when used upon real valued single-bit traces."
		);
      return;
    }

  if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */
  v = combine_traces(0, NULL); /* up */

  if (v)
    {
      Trptr t;

      AddVector(v, NULL);
      free_2(v->bits->name);
      v->bits->name=NULL;

      t = GLOBALS->traces.last;

      RemoveTrace(t, 0);

      /* t is now the composite signal trace */

      create_group("unused_1", t);

      GLOBALS->signalwindow_width_dirty=1;
      MaxSignalLength();
      signalarea_configure_event(GLOBALS->signalarea, NULL);
      wavearea_configure_event(GLOBALS->wavearea, NULL);

      menu_rename(widget, data);
    }
  else
    {
    }
}

/**/

void menu_tracesearchbox_callback(GtkWidget *widget, gpointer data)
{
}

void menu_tracesearchbox(GtkWidget *widget, gpointer data)
{
Trptr t;

if(GLOBALS->helpbox_is_active)
        {  
        help_text_bold("\n\nPattern Search");
        help_text(
		" only works when at least one trace is highlighted. "
		" A requester will appear that lists all the selected"
		" traces (maximum of 500) and allows various criteria"
		" to be specified for each trace.  Searches can go forward"
		" or backward from the primary (unnamed) marker.  If the"
		" primary marker has not been set, the search starts at the"
		" beginning of the displayed data (\"From\") for a forwards"
		" search and starts at the end of the displayed data (\"To\")"
		" for a backwards search."
		" \"Mark\" and \"Clear\" are used to modify the normal time"
		" vertical markings such that they can be used to indicate"
		" all the times that a specific pattern search condition is"
		" true (e.g., every upclock of a specific signal).  The"
		" \"Mark Count\" field indicates how many times the specific"
		" pattern search condition was encountered."
		" The \"Marking Begins at\" and \"Marking Stops at\" fields are"
		" used to limit the time over which marking is applied"
		" (but they have no effect on searching)."
        );
        return;
        }

 for(t=GLOBALS->traces.first;t;t=t->t_next)
	{
	if ((t->flags&TR_HIGHLIGHT)&&HasWave(t))
	  {
	    /* at least one good trace, so do it */
	    /* data contains WV_MENU_SPS or WV_MENU_SPS2 or ... but the base is WV_MENU_SPS*/
	    char buf[128];
	    long which = ((long)data) - WV_MENU_SPS;

	    if((which < 0) || (which >= WAVE_NUM_STRACE_WINDOWS)) 
			{ /* should never happen unless menus are defined wrong */
			sprintf(buf, "Pattern search ID %d out of range of 1-%d available, ignoring.", (int)(which+1), WAVE_NUM_STRACE_WINDOWS);
			status_text(buf);
			}
			else
			{
			sprintf(buf, "Waveform Display Search (%d)", (int)(which+1));
			tracesearchbox(buf, GTK_SIGNAL_FUNC(menu_tracesearchbox_callback), (gpointer)which);
			}
	    return;
	  }
	}
 must_sel();
}

/**/
void
menu_new_viewer_cleanup(GtkWidget *widget, gpointer data)
{
pid_t pid;

if(GLOBALS->filesel_ok)
	{
#if !defined __MINGW32__ && !defined _MSC_VER
	/*
	 * for some reason, X won't let us double-fork in order to cleanup zombies.. *shrug*
         */
	pid=fork();
	if(((int)pid) < 0) { return; /* not much we can do about this.. */ }
	
	if(pid)         /* parent==original server_pid */
	        {
		return;
       		}

	execlp(GLOBALS->whoami, GLOBALS->whoami, *GLOBALS->fileselbox_text, NULL);
	exit(0);	/* control never gets here if successful */

#else

	BOOL bSuccess = FALSE;
	PROCESS_INFORMATION piProcInfo;
	TCHAR *szCmdline;
	STARTUPINFO si;

	memset(&piProcInfo, 0, sizeof(PROCESS_INFORMATION));
	memset(&si, 0, sizeof(STARTUPINFO));

        szCmdline = malloc_2(strlen(GLOBALS->whoami) + 1 + strlen(*GLOBALS->fileselbox_text) + 1);
        sprintf(szCmdline, "%s %s", GLOBALS->whoami, *GLOBALS->fileselbox_text);

	bSuccess = CreateProcess(NULL,
	      szCmdline,     /* command line */
	      NULL,          /* process security attributes */
	      NULL,          /* primary thread security attributes */
	      TRUE,          /* handles are inherited */
	      0,             /* creation flags */
	      NULL,          /* use parent's environment */
	      NULL,          /* use parent's current directory */
	      &si,           /* STARTUPINFO pointer */
	      &piProcInfo);  /* receives PROCESS_INFORMATION */
	
	free_2(szCmdline);
	
	if(!bSuccess)
	        {
	        /* failed */
	        }
#endif
	}
}

void
menu_new_viewer(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
	{
	help_text_bold("\n\nOpen New Window");
	help_text(
		" will open a file requester that will ask for the name"
		" of a VCD or AET file to view.  This will fork off a"
		" new viewer process."
	);
	return;
	}

fileselbox("Select a trace to view...",&GLOBALS->filesel_newviewer_menu_c_1,GTK_SIGNAL_FUNC(menu_new_viewer_cleanup), GTK_SIGNAL_FUNC(NULL), NULL, 0);
}

/**/

int
menu_new_viewer_tab_cleanup_2(char *fname)
{
	int rc = 0;
	char *argv[2];
	struct Global *g_old = GLOBALS;
	struct Global *g_now;

	argv[0] = "gtkwave";
	argv[1] = fname;

	GLOBALS->vcd_jmp_buf = calloc(1, sizeof(jmp_buf));

	set_window_busy(NULL);
	gtkwave_main_iteration();

	if(!setjmp(*(GLOBALS->vcd_jmp_buf)))
		{
		main_2(2, argv);

		g_now = GLOBALS;
		set_GLOBALS(g_old);

		clone_icon_pointers_across_contexts(g_now, GLOBALS);

		free(GLOBALS->vcd_jmp_buf); GLOBALS->vcd_jmp_buf = NULL;
		set_window_idle(NULL);
		set_GLOBALS(g_now);
		g_now->vcd_jmp_buf = NULL;

		/* copy old file req strings into new context */
		strcpy2_into_new_context(GLOBALS, &GLOBALS->fcurr_ttranslate_c_1, &g_old->fcurr_ttranslate_c_1);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->fcurr_ptranslate_c_1, &g_old->fcurr_ptranslate_c_1);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->fcurr_translate_c_2, &g_old->fcurr_translate_c_2);

#if 0
		/* disabled for now...these probably would be disruptive */
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_lxt_writesave, &g_old->filesel_lxt_writesave);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_vcd_writesave, &g_old->filesel_vcd_writesave);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_tim_writesave, &g_old->filesel_tim_writesave);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_writesave, &g_old->filesel_writesave);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->stems_name, &g_old->stems_name);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_logfile_menu_c_1, &g_old->filesel_logfile_menu_c_1);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_scriptfile_menu, &g_old->filesel_scriptfile_menu);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_print_pdf_renderopt_c_1, &g_old->filesel_print_pdf_renderopt_c_1);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_print_ps_renderopt_c_1, &g_old->filesel_print_ps_renderopt_c_1);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_print_mif_renderopt_c_1, &g_old->filesel_print_mif_renderopt_c_1);
#endif		

		if(g_old->loaded_file_type == MISSING_FILE) /* remove original "blank" page */
			{
                        if(g_old->missing_file_toolbar) gtk_widget_set_sensitive(g_old->missing_file_toolbar, TRUE);
			menu_set_sensitive();
			gtk_notebook_set_current_page(GTK_NOTEBOOK(g_old->notebook), g_old->this_context_page);
			menu_quit_close_callback(NULL, NULL);
			}

	
		/* not sure what's really needed here */
		/* for now, add back in repscript_name */
		/* if(GLOBALS->repscript_name) free_2(GLOBALS->repscript_name); */
/* 		GLOBALS->repscript_name = malloc_2(strlen(g_old->repscript_name)+1); */
/* 		strcpy(GLOBALS->repscript_name, g_old->repscript_name); */

		GLOBALS->strace_repeat_count = g_old->strace_repeat_count;

		GLOBALS->rpc_ctx = g_old->rpc_ctx;
		rc = 1;
		}
                else
                {
                if(GLOBALS->vcd_handle_vcd_c_1) { fclose(GLOBALS->vcd_handle_vcd_c_1); GLOBALS->vcd_handle_vcd_c_1 = NULL; }
                if(GLOBALS->vcd_handle_vcd_recoder_c_2) { fclose(GLOBALS->vcd_handle_vcd_recoder_c_2); GLOBALS->vcd_handle_vcd_recoder_c_2 =NULL; }
                if(GLOBALS->mm_lxt_mmap_addr)
                	{
                        munmap(GLOBALS->mm_lxt_mmap_addr, GLOBALS->mm_lxt_mmap_len);
                        GLOBALS->mm_lxt_mmap_addr = NULL;
                        }
                free_outstanding(); /* free anything allocated in loader ctx */
		free(GLOBALS);
	
		set_GLOBALS(g_old);
                free(GLOBALS->vcd_jmp_buf); GLOBALS->vcd_jmp_buf = NULL;
		set_window_idle(NULL);

		/* load failed */
		printf("GTKWAVE | File load failure, new tab not created.\n");
		rc = 0;
                }

return(rc);
}


void
menu_new_viewer_tab_cleanup(GtkWidget *widget, gpointer data)
{
if(GLOBALS->filesel_ok)
        { 
	menu_new_viewer_tab_cleanup_2(*GLOBALS->fileselbox_text);
	}
}


void
menu_new_viewer_tab(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
	{
	help_text_bold("\n\nOpen New Tab");
	help_text(
		" will open a file requester that will ask for the name"
		" of a VCD or AET file to view.  This will create a tabbed page."
	);
	return;
	}

#ifdef WAVE_USE_GTK2
if((!GLOBALS->socket_xid)&&(!GLOBALS->partial_vcd))
#else
if(!GLOBALS->partial_vcd)
#endif
	{
	fileselbox("Select a trace to view...",&GLOBALS->filesel_newviewer_menu_c_1,GTK_SIGNAL_FUNC(menu_new_viewer_tab_cleanup), GTK_SIGNAL_FUNC(NULL), NULL, 0);
	}
}

/**/ 

void
menu_reload_waveform(GtkWidget *widget, gpointer data)
{
 if(GLOBALS->helpbox_is_active)
	{
	help_text_bold("\n\nReload Current Waveform");
	help_text(
		" will reload the currently displayed waveform"
		" from a potentially updated file."
	);
	return;
	}

 if(GLOBALS->gt_splash_c_1)
	{
	return; /* don't attempt reload if splash screen is still active...that's pointless anyway */
	}

 /* XXX if there's no file (for some reason), this function shouldn't occur
    we should probably gray it out. */
 if(GLOBALS->loaded_file_type == DUMPLESS_FILE) {
   printf("GTKWAVE | DUMPLESS_FILE type cannot be reloaded\n");
   return;
 }

 reload_into_new_context();
}

/**/

void
menu_print(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
	{
	help_text_bold("\n\nPrint To File");
	help_text(
		" will open up a requester that will allow you to select"
		" print options (PS or MIF; Letter, A4, or Legal; Full or Minimal)."
		" After selecting the options you want,"
		" a file requester will ask for the name of the"
		" output file to generate"
		" that reflects the current main window display's contents. "
	);
	return;
	}

renderbox("Print Formatting Options");
}

/**/
void menu_markerbox_callback(GtkWidget *widget, gpointer data)
{
}

void menu_markerbox(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nShow-Change Marker Data");
        help_text(
                " displays and allows the modification of the times for"
		" all 26 named markers by filling in the leftmost entry boxes.  In addition, optional marker text"
		" rather than a generic single letter name may be specified by filling in the rightmost entry boxes."
		" Note that the time for each marker must be unique."
        );
        return;
        }

markerbox("Markers", GTK_SIGNAL_FUNC(menu_markerbox_callback));
}

/**/
void delete_unnamed_marker(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nDelete Primary Marker");
        help_text(
                " removes the primary marker from the display if present."
        );
        return;
        }        

DEBUG(printf("delete_unnamed marker()\n"));

if(GLOBALS->tims.marker!=-1)
	{
	Trptr t;

	for(t=GLOBALS->traces.first;t;t=t->t_next)
		{
		if(t->asciivalue) { free_2(t->asciivalue); t->asciivalue=NULL; }
		}

	for(t=GLOBALS->traces.buffer;t;t=t->t_next)
		{
		if(t->asciivalue) { free_2(t->asciivalue); t->asciivalue=NULL; }
		}

	update_markertime(GLOBALS->tims.marker=-1);
	GLOBALS->signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}

/**/
void collect_all_named_markers(GtkWidget *widget, gpointer data)
{
int i;
int dirty=0;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nCollect All Named Markers");
        help_text(
		" simply collects any and all named markers which have"
		" been dropped."
        );
        return;
        }

DEBUG(printf("collect_all_unnamed_markers()\n"));

for(i=0;i<26;i++)
	{
	if(GLOBALS->named_markers[i]!=-1)
		{
		GLOBALS->named_markers[i]=-1;
		dirty=1;
		}
	}

if(dirty)
	{
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}
/**/
void collect_named_marker(GtkWidget *widget, gpointer data)
{
int i;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nCollect Named Marker");
        help_text(
                " collects a named marker where the current primary (unnamed)"
                " marker is placed if there is a named marker at its position."
        );
        return;
        }

DEBUG(printf("collect_named_marker()\n"));

if(GLOBALS->tims.marker!=-1)
	{
	for(i=0;i<26;i++)
		{
		if(GLOBALS->named_markers[i]==GLOBALS->tims.marker)
			{
			GLOBALS->named_markers[i]=-1;
			signalarea_configure_event(GLOBALS->signalarea, NULL);
			wavearea_configure_event(GLOBALS->wavearea, NULL);
			/* return; */
			}
		}
	}
}
/**/
void drop_named_marker(GtkWidget *widget, gpointer data)
{
int i;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nDrop Named Marker");
        help_text(
		" drops a named marker where the current primary (unnamed)"
		" marker is placed.  A maximum of 26 named markers are allowed"
		" and the times for all must be different."
        );
        return;
        }


DEBUG(printf("drop_named_marker()\n"));

if(GLOBALS->tims.marker!=-1)
	{
/* only one per slot requirement removed...
#if 0
	for(i=0;i<26;i++)
		{
		if(GLOBALS->named_markers[i]==GLOBALS->tims.marker) return;
		}
#endif
...only one per slot requirement removed */

	for(i=0;i<26;i++)
		{
		if(GLOBALS->named_markers[i]==-1)
			{
			GLOBALS->named_markers[i]=GLOBALS->tims.marker;
			signalarea_configure_event(GLOBALS->signalarea, NULL);
			wavearea_configure_event(GLOBALS->wavearea, NULL);
			return;
			}
		}
	}
}
/**/
void menu_treesearch_cleanup(GtkWidget *widget, gpointer data)
{
MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
DEBUG(printf("menu_treesearch_cleanup()\n"));
}

void menu_treesearch(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nSignal Search Tree");
        help_text(
                " provides an easy means of adding traces to the display."
                " Various functions are provided in the Signal Search Tree requester"
                " which allow searching a treelike hierarchy and bundling"
                " (coalescing individual bits into a single vector)."
        );
        return;
        }

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

treebox("Signal Search Tree",GTK_SIGNAL_FUNC(menu_treesearch_cleanup), NULL);
}
/**/
void 
menu_showchangeall_cleanup(GtkWidget *widget, gpointer data)
{
Trptr t;
Ulong flags;

t=GLOBALS->showchangeall_menu_c_1;
if(t)
	{
	flags=t->flags;
	while(t)
		{
		if((t->flags&TR_HIGHLIGHT)&&(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))&&(t->name))
			{
			t->flags=flags;
			}	
		t=t->t_next;
		}
	}

GLOBALS->signalwindow_width_dirty=1;
MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
DEBUG(printf("menu_showchangeall_cleanup()\n"));
}

void 
menu_showchangeall(GtkWidget *widget, gpointer data)
{
Trptr t;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nShow-Change All Highlighted");   
        help_text(
                " provides an easy means of changing trace attributes en masse."
                " Various functions are provided in a Show-Change requester."
        );
        return;
        }

DEBUG(printf("menu_showchangeall()\n"));

GLOBALS->showchangeall_menu_c_1=NULL;
t=GLOBALS->traces.first;
while(t)
	{
	if((t->flags&TR_HIGHLIGHT)&&(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))&&(t->name))
		{
		showchange("Show-Change All", GLOBALS->showchangeall_menu_c_1=t, GTK_SIGNAL_FUNC(menu_showchangeall_cleanup));
		return;
		}
	t=t->t_next;
	}

must_sel();
}

/**/
void 
menu_showchange_cleanup(GtkWidget *widget, gpointer data)
{
GLOBALS->signalwindow_width_dirty=1;
MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
DEBUG(printf("menu_showchange_cleanup()\n"));
}

void 
menu_showchange(GtkWidget *widget, gpointer data)
{
Trptr t;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nShow-Change First Highlighted");
        help_text(
                " provides a means of changing trace attributes for the"
		" first highlighted trace. "
                " Various functions are provided in a Show-Change requester. "
  		" When a function is applied, the trace will be unhighlighted."
        );
        return;
        }

DEBUG(printf("menu_showchange()\n"));

t=GLOBALS->traces.first;
while(t)
	{
	if((t->flags&TR_HIGHLIGHT)&&(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))&&(t->name))
		{
		showchange("Show-Change", t, GTK_SIGNAL_FUNC(menu_showchange_cleanup));
		return;
		}
	t=t->t_next;
	}

must_sel();
}
/**/
void menu_remove_aliases(GtkWidget *widget, gpointer data)
{
  Trptr t;
  int dirty=0, none_selected = 1;

  if(GLOBALS->helpbox_is_active)
    {
      help_text_bold("\n\nRemove Highlighted Aliases");
      help_text(
                " only works when at least one trace has been highlighted. "
                " Any aliased traces will have their names restored to their"
		" original names.  As vectors get their names from aliases,"
		" vector aliases will not be removed."
		);
      return;
    }

  if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

  t=GLOBALS->traces.first;
  while(t)
    {
      if(HasAlias(t) && IsSelected(t))
	{
          char *name_full;
          int was_packed = 0;

	  free_2(t->name_full);
	  t->name_full = NULL;

	  if(t->vector)
		{
		name_full = t->n.vec->bvname;
		}
		else
		{
		name_full = hier_decompress_flagged(t->n.nd->nname, &was_packed);
		}

	  t->name = name_full;
	  if (GLOBALS->hier_max_level) 
		{
		if(!was_packed)
			{
		    	t->name = hier_extract(t->name, GLOBALS->hier_max_level);
			}
			else
			{
		    	t->name = strdup_2(hier_extract(name_full, GLOBALS->hier_max_level));
			free_2(name_full);
			}
		}

	  if(was_packed) t->is_depacked = 1;

	  dirty = 1;
	}
      if (IsSelected(t)) none_selected = 0;
      t=t->t_next;
    }

  if(dirty)
    {
      GLOBALS->signalwindow_width_dirty=1;
      MaxSignalLength();
      signalarea_configure_event(GLOBALS->signalarea, NULL);
      wavearea_configure_event(GLOBALS->wavearea, NULL);
      DEBUG(printf("menu_remove_aliases()\n"));
    }
  if (none_selected)
    {
      must_sel();
    }
}


static void alias_cleanup(GtkWidget *widget, gpointer data)
{
  Trptr t = GLOBALS->trace_to_alias_menu_c_1;

  if(GLOBALS->entrybox_text)
    {
      char *efix;
      /* code to turn '{' and '}' into '[' and ']' */
      if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
	{
	  efix=GLOBALS->entrybox_text;
	  while(*efix)
	    {
	      if(*efix=='{')
		{
		  *efix='[';
		}
	      if(*efix=='}')
		{
		  *efix=']';
		}
	      efix++;
	    }
	}

      if (CanAlias(t))
	{
	  if(HasAlias(t)) free_2(t->name_full);
	  t->name_full = (char *)malloc_2(1+strlen(GLOBALS->entrybox_text));
	  strcpy(t->name_full, GLOBALS->entrybox_text);
	  t->name = t->name_full;
	  if(GLOBALS->hier_max_level)
	    t->name = hier_extract(t->name, GLOBALS->hier_max_level);

	  t->flags&= ~TR_HIGHLIGHT;
	}

      GLOBALS->signalwindow_width_dirty=1;
      MaxSignalLength();
      signalarea_configure_event(GLOBALS->signalarea, NULL);
      wavearea_configure_event(GLOBALS->wavearea, NULL);
    }
}

void menu_alias(GtkWidget *widget, gpointer data)
{
  Trptr t;

  if(GLOBALS->helpbox_is_active)
    {
      help_text_bold("\n\nAlias Highlighted Trace");
      help_text(
                " only works when at least one trace has been highlighted. "
                " With this function, you will be prompted for an alias"
                " name for the first highlighted trace.  After successfully"
		" aliasing a trace, the aliased trace will be unhighlighted."
		" Single bits will be marked with a leading \"+\" and vectors"
		" will have no such designation.  The purpose of this is to"
		" provide a fast method of determining which trace names are"
		" real and which ones are aliases."
		);
      return;
    }

  GLOBALS->trace_to_alias_menu_c_1=NULL;

  /* don't mess with sigs when dnd active */
  if(GLOBALS->dnd_state) { dnd_error(); return; } 

  t = GLOBALS->traces.first;
  while(t)
    {
      if(IsSelected(t)&&CanAlias(t))
	{
	  GLOBALS->trace_to_alias_menu_c_1=t;
	  break;
	}
      t=t->t_next;
    }

  if(GLOBALS->trace_to_alias_menu_c_1)
    {
      int was_packed = 0;
      char* current = GetFullName(GLOBALS->trace_to_alias_menu_c_1, &was_packed);
      ClearTraces();
      GLOBALS->trace_to_alias_menu_c_1->flags |= TR_HIGHLIGHT;
      entrybox("Alias Highlighted Trace",300,current,NULL,128,GTK_SIGNAL_FUNC(alias_cleanup));
      if(was_packed) { free_2(current); }
    }
  else
    {
      must_sel();
    }
}




/**/
void menu_hiersearch_cleanup(GtkWidget *widget, gpointer data)
{
MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
DEBUG(printf("menu_hiersearch_cleanup()\n"));
}

void menu_hiersearch(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nHierarchy Search");
        help_text(
		" provides an easy means of adding traces to the display in a text based"
		" treelike fashion."
        );
        return;
        }

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

hier_searchbox("Hierarchy Search",GTK_SIGNAL_FUNC(menu_hiersearch_cleanup));
}
/**/
void menu_signalsearch_cleanup(GtkWidget *widget, gpointer data)
{
MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
DEBUG(printf("menu_signalsearch_cleanup()\n"));
}

void menu_signalsearch(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nSignal Search Regexp");
        help_text(
		" provides an easy means of adding traces to the display. "
		" Various functions are provided in the Signal Search requester"
		" which allow searching using POSIX regular expressions and bundling"
		" (coalescing individual bits into a single vector). "
        );
        return;
        }

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

searchbox("Signal Search",GTK_SIGNAL_FUNC(menu_signalsearch_cleanup));
}
/**/
static void 
regexp_highlight_generic(int mode)
{
if(GLOBALS->entrybox_text)
	{
	Trptr t;
	Ulong modebits;
	char dirty=0;

	modebits=(mode)?TR_HIGHLIGHT:0;

	strcpy(GLOBALS->regexp_string_menu_c_1, GLOBALS->entrybox_text);
	wave_regex_compile(GLOBALS->regexp_string_menu_c_1, WAVE_REGEX_SEARCH);
	free_2(GLOBALS->entrybox_text);
	t=GLOBALS->traces.first;
	while(t)
		{
		char *pnt;

		pnt=(t->name)?t->name:""; /* handle (really) blank lines */

		if(*pnt=='+')		  /* skip alias prefix if present */
			{
			pnt++;
			if(*pnt==' ')
				{
				pnt++;
				}
			}

		if(wave_regex_match(pnt, WAVE_REGEX_SEARCH))
			{
			t->flags=((t->flags&(~TR_HIGHLIGHT))|modebits);
			dirty=1;
			}

		t=t->t_next;
		}

	if(dirty)
		{
		signalarea_configure_event(GLOBALS->signalarea, NULL);
		wavearea_configure_event(GLOBALS->wavearea, NULL);
		}
	}
}

static void 
regexp_unhighlight_cleanup(GtkWidget *widget, gpointer data)
{
regexp_highlight_generic(0);
}

void 
menu_regexp_unhighlight(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nUnHighlight Regexp");
        help_text(
                " brings up a text requester that will ask for a"
                " regular expression that may contain text with POSIX regular expressions."
                " All traces meeting this criteria will be"
                " unhighlighted if they are currently highlighted."
        );
        return;
        }

entrybox("Regexp UnHighlight",300,GLOBALS->regexp_string_menu_c_1,NULL,128,GTK_SIGNAL_FUNC(regexp_unhighlight_cleanup));
}
/**/
static void 
regexp_highlight_cleanup(GtkWidget *widget, gpointer data)
{
regexp_highlight_generic(1);
}

void 
menu_regexp_highlight(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nHighlight Regexp");
        help_text(
                " brings up a text requester that will ask for a"
                " regular expression that may contain text with POSIX regular expressions."
		" All traces meeting this criteria will be"
		" highlighted."
        );
        return;
        }

entrybox("Regexp Highlight",300,GLOBALS->regexp_string_menu_c_1,NULL,128,GTK_SIGNAL_FUNC(regexp_highlight_cleanup));
}

/**/

char *append_array_row(nptr n)
{
int was_packed = 0;
char *hname = hier_decompress_flagged(n->nname, &was_packed);

if(!n->array_height)
	{
	strcpy(GLOBALS->buf_menu_c_1, hname);
	}
	else
	{
	sprintf(GLOBALS->buf_menu_c_1, "%s{%d}", hname, n->this_row);
	}

if(was_packed) free_2(hname);

return(GLOBALS->buf_menu_c_1);
}



void write_save_helper(FILE *wave) {
	Trptr t;
	int i;
	unsigned int def=0;
	int sz_x, sz_y;
	TimeType prevshift=LLDescriptor(0);
	int root_x, root_y;
        struct strace *st;
	int s_ctx_iter;

	DEBUG(printf("Write Save Fini: %s\n", *fileselbox_text));

	fprintf(wave, "[timestart] "TTFormat"\n", GLOBALS->tims.start);

	get_window_size (&sz_x, &sz_y);
	if(!GLOBALS->ignore_savefile_size) fprintf(wave,"[size] %d %d\n", sz_x, sz_y);

	get_window_xypos(&root_x, &root_y);

	if(!GLOBALS->ignore_savefile_pos) fprintf(wave,"[pos] %d %d\n", root_x + GLOBALS->xpos_delta, root_y + GLOBALS->ypos_delta);

	fprintf(wave,"*%f "TTFormat, (float)(GLOBALS->tims.zoom),GLOBALS->tims.marker);

	for(i=0;i<26;i++)
		{
		TimeType nm = GLOBALS->named_markers[i]; /* gcc compiler problem...thinks this is a 'long int' in printf format warning reporting */
		fprintf(wave," "TTFormat,nm);
		}
	fprintf(wave,"\n");

	for(i=0;i<26;i++)
		{
		if(GLOBALS->marker_names[i])
			{
			fprintf(wave, "[markername] %c%s\n", 'A'+i, GLOBALS->marker_names[i]);
			}	
		}

#if WAVE_USE_GTK2
	if(GLOBALS->open_tree_nodes)
		{
		dump_open_tree_nodes(wave, GLOBALS->open_tree_nodes);
		}
#endif

	t=GLOBALS->traces.first;
	while(t)
		{
		if((t->flags!=def)||(t==GLOBALS->traces.first))
			{
			if((t->flags & TR_PTRANSLATED) && (!t->p_filter)) t->flags &= (~TR_PTRANSLATED);
			if((t->flags & TR_FTRANSLATED) && (!t->f_filter)) t->flags &= (~TR_FTRANSLATED);
			if((t->flags & TR_TTRANSLATED) && (!t->t_filter)) t->flags &= (~TR_TTRANSLATED);
			fprintf(wave,"@%x\n",def=t->flags);
			}

		if((t->shift)||((prevshift)&&(!t->shift)))
			{
			fprintf(wave,">"TTFormat"\n", t->shift);
			}
		prevshift=t->shift;

		if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))	
			{
			if(t->flags & TR_FTRANSLATED)
				{
				if(t->f_filter && GLOBALS->filesel_filter[t->f_filter])
					{
					fprintf(wave, "^%d %s\n", t->f_filter, GLOBALS->filesel_filter[t->f_filter]);
					}
					else
					{
					fprintf(wave, "^%d %s\n", 0, "disabled");
					}
				}
			else
			if(t->flags & TR_PTRANSLATED)
				{
				if(t->p_filter && GLOBALS->procsel_filter[t->p_filter])
					{
					fprintf(wave, "^>%d %s\n", t->p_filter, GLOBALS->procsel_filter[t->p_filter]);
					}
					else
					{
					fprintf(wave, "^>%d %s\n", 0, "disabled");
					}
				}

			/* NOT an else! */
			if(t->flags & TR_TTRANSLATED)
				{
				if(t->t_filter && GLOBALS->ttranssel_filter[t->t_filter])
					{
					fprintf(wave, "^<%d %s\n", t->t_filter, GLOBALS->ttranssel_filter[t->t_filter]);
					}
					else
					{
					fprintf(wave, "^<%d %s\n", 0, "disabled");
					}
				}

			if(t->vector)
				{
				int ix;
				nptr *nodes;
				bptr bits;
				baptr ba;

				if (HasAlias(t)) { fprintf(wave,"+{%s} ", t->name_full); }
				bits = t->n.vec->bits;
				ba = bits ? bits->attribs : NULL;

				fprintf(wave,"%c{%s}", ba ? ':' : '#', 
						t->n.vec->transaction_cache ? t->n.vec->transaction_cache->bvname : t->n.vec->bvname);

				nodes=t->n.vec->bits->nodes;
				for(ix=0;ix<t->n.vec->bits->nnbits;ix++)
					{
					if(nodes[ix]->expansion)
						{
						fprintf(wave," (%d)%s",nodes[ix]->expansion->parentbit, append_array_row(nodes[ix]->expansion->parent));
						}
						else
						{
						fprintf(wave," %s",append_array_row(nodes[ix]));
						}
					if(ba)
						{
						fprintf(wave, " "TTFormat" %x", ba[ix].shift, ba[ix].flags);
						}
					}
				fprintf(wave,"\n");
				}
				else
				{
				if(HasAlias(t))
					{
					if(t->n.nd->expansion)
						{
						fprintf(wave,"+{%s} (%d)%s\n",t->name_full,t->n.nd->expansion->parentbit, append_array_row(t->n.nd->expansion->parent));
						}
						else
						{
						fprintf(wave,"+{%s} %s\n",t->name_full,append_array_row(t->n.nd));
						}
					}
					else
					{
					if(t->n.nd->expansion)
						{
						fprintf(wave,"(%d)%s\n",t->n.nd->expansion->parentbit, append_array_row(t->n.nd->expansion->parent));
						}
						else
						{
						fprintf(wave,"%s\n",append_array_row(t->n.nd));
						}
					}
				}
			}
			else
			{
			if(!t->name) fprintf(wave,"-\n");
			else fprintf(wave,"-%s\n",t->name);
			}
		t=t->t_next;
		}

	WAVE_STRACE_ITERATOR(s_ctx_iter)
	{
	GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];
	fprintf(wave, "[pattern_trace] %d\n", s_ctx_iter);

	if(GLOBALS->strace_ctx->timearray)
		{
		if(GLOBALS->strace_ctx->shadow_straces)
			{
			swap_strace_contexts();

			st=GLOBALS->strace_ctx->straces;
			if(GLOBALS->strace_ctx->straces)
				{
				fprintf(wave, "!%d%d%d%d%d%d%c%c\n", GLOBALS->strace_ctx->logical_mutex[0], GLOBALS->strace_ctx->logical_mutex[1], GLOBALS->strace_ctx->logical_mutex[2], GLOBALS->strace_ctx->logical_mutex[3], GLOBALS->strace_ctx->logical_mutex[4], GLOBALS->strace_ctx->logical_mutex[5], '@'+GLOBALS->strace_ctx->mark_idx_start, '@'+GLOBALS->strace_ctx->mark_idx_end);
				}

			while(st)
				{
				if(st->value==ST_STRING)
					{
					fprintf(wave, "?\"%s\n", st->string ? st->string : ""); /* search type for this trace is string.. */
					}
					else
					{
					fprintf(wave, "?%02x\n", (unsigned char)st->value);	/* else search type for this trace.. */
					}
			
				t=st->trace;

				if(t->flags!=def)
					{
					if((t->flags & TR_FTRANSLATED) && (!t->f_filter)) t->flags &= (~TR_FTRANSLATED);
					if((t->flags & TR_PTRANSLATED) && (!t->p_filter)) t->flags &= (~TR_PTRANSLATED);
					if((t->flags & TR_TTRANSLATED) && (!t->t_filter)) t->flags &= (~TR_TTRANSLATED);
					fprintf(wave,"@%x\n",def=t->flags);
					}

				if((t->shift)||((prevshift)&&(!t->shift)))
					{
					fprintf(wave,">"TTFormat"\n", t->shift);
					}
				prevshift=t->shift;

				if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))	
					{
					if(t->flags & TR_FTRANSLATED)
						{
						if(t->f_filter && GLOBALS->filesel_filter[t->f_filter])
							{
							fprintf(wave, "^%d %s\n", t->f_filter, GLOBALS->filesel_filter[t->f_filter]);
							}
							else
							{
							fprintf(wave, "^%d %s\n", 0, "disabled");
							}
						}
					else
					if(t->flags & TR_PTRANSLATED)
						{
						if(t->p_filter && GLOBALS->procsel_filter[t->p_filter])
							{
							fprintf(wave, "^>%d %s\n", t->p_filter, GLOBALS->procsel_filter[t->p_filter]);
							}
							else
							{
							fprintf(wave, "^>%d %s\n", 0, "disabled");
							}
						}

					/* NOT an else! */
					if(t->flags & TR_TTRANSLATED)
						{
						if(t->t_filter && GLOBALS->ttranssel_filter[t->t_filter])
							{
							fprintf(wave, "^<%d %s\n", t->t_filter, GLOBALS->ttranssel_filter[t->t_filter]);
							}
							else
							{
							fprintf(wave, "^<%d %s\n", 0, "disabled");
							}
						}


					if(t->vector)
						{
						int ix;
                                                nptr *nodes;
						bptr bits;
						baptr ba;

						if (HasAlias(t)) { fprintf(wave,"+{%s} ", t->name_full); }

						bits = t->n.vec->bits;
						ba = bits ? bits->attribs : NULL;

						fprintf(wave,"%c{%s}", ba ? ':' : '#', 
							t->n.vec->transaction_cache ? t->n.vec->transaction_cache->bvname : t->n.vec->bvname);

						nodes=t->n.vec->bits->nodes;
						for(ix=0;ix<t->n.vec->bits->nnbits;ix++)
							{
							if(nodes[ix]->expansion)
								{
								fprintf(wave," (%d)%s",nodes[ix]->expansion->parentbit, append_array_row(nodes[ix]->expansion->parent));
								}
								else
								{
								fprintf(wave," %s",append_array_row(nodes[ix]));
								}
							if(ba)
								{
								fprintf(wave, " "TTFormat" %x", ba[ix].shift, ba[ix].flags);
								}
							}
						fprintf(wave,"\n");
						}
						else
						{
						if(HasAlias(t))
							{
							if(t->n.nd->expansion)
								{
								fprintf(wave,"+{%s} (%d)%s\n",t->name_full,t->n.nd->expansion->parentbit, append_array_row(t->n.nd->expansion->parent));
								}
								else
								{
								fprintf(wave,"+{%s} %s\n",t->name_full,append_array_row(t->n.nd));
								}
							}
							else
							{
							if(t->n.nd->expansion)
								{
								fprintf(wave,"(%d)%s\n",t->n.nd->expansion->parentbit, append_array_row(t->n.nd->expansion->parent));
								}
								else
								{
								fprintf(wave,"%s\n",append_array_row(t->n.nd));
								}
							}
						}
					}

				st=st->next;
				} /* while(st)... */

			if(GLOBALS->strace_ctx->straces)
				{
				fprintf(wave, "!!\n");	/* mark end of strace region */
				}
		
				swap_strace_contexts();
			}
			else
			{
			struct mprintf_buff_t *mt = GLOBALS->strace_ctx->mprintf_buff_head;

			while(mt)	
				{
				fprintf(wave, "%s", mt->str);
				mt=mt->next;
				}
			}

		} /* if(timearray)... */
	}
}


void
menu_write_save_cleanup(GtkWidget *widget, gpointer data)
{
FILE *wave;

if(!GLOBALS->filesel_ok)
	{
	return;
	}

if(!(wave=fopen(*GLOBALS->fileselbox_text,"wb")))
        {
        fprintf(stderr, "Error opening save file '%s' for writing.\n",*GLOBALS->fileselbox_text);
	perror("Why");
	errno=0;
        }
	else
	{
          write_save_helper(wave);
	  GLOBALS->save_success_menu_c_1 = 1;
	  fclose(wave);
	}

}

void
menu_write_save_file_as(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
	{
	help_text_bold("\n\nWrite Save File As");
	help_text(
		" will open a file requester that will ask for the name"
		" of a GTKWave save file.  The contents of the save file"
		" generated will be the traces as well as their"
 		" format (binary, decimal, hex, reverse, etc.) which"
		" are currently a part of the display.  Marker positional"
		" data and the zoom factor are also a part of the save file."
	);
	return;
	}

fileselbox("Write Save File",&GLOBALS->filesel_writesave,GTK_SIGNAL_FUNC(menu_write_save_cleanup), GTK_SIGNAL_FUNC(NULL), "*.sav", 1);
}

void
menu_write_save_file(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
	{
	help_text_bold("\n\nWrite Save File");
	help_text(
		" will invoke Write Save File As if no save file name has been specified previously."
		" Otherwise it will write the save file data without prompting."
	);
	return;
	}

if(!GLOBALS->filesel_writesave)
	{
	fileselbox("Write Save File",&GLOBALS->filesel_writesave,GTK_SIGNAL_FUNC(menu_write_save_cleanup), GTK_SIGNAL_FUNC(NULL), "*.sav", 1);
	}
	else
	{
	GLOBALS->filesel_ok = 1;
	GLOBALS->save_success_menu_c_1 = 0;
	GLOBALS->fileselbox_text = &GLOBALS->filesel_writesave;
	menu_write_save_cleanup(NULL, NULL);
	if(GLOBALS->save_success_menu_c_1)
		{
		status_text("Wrote save file OK.\n");
		}
		else
		{
		status_text("Problem writing save file.\n");
		}
	}
}
/**/


int read_save_helper(char *wname) { 
        FILE *wave;
        char *str = NULL;
        int wave_is_compressed;
	char traces_already_exist = (GLOBALS->traces.first != NULL);
	int rc = -1;

        if(((strlen(wname)>2)&&(!strcmp(wname+strlen(wname)-3,".gz")))||
          ((strlen(wname)>3)&&(!strcmp(wname+strlen(wname)-4,".zip"))))
                {
                str=wave_alloca(strlen(wname)+5+1);
                strcpy(str,"zcat ");
                strcpy(str+5,wname);
                wave=popen(str,"r");
                wave_is_compressed=~0;
                }
                else
                {   
                wave=fopen(wname,"rb");
                wave_is_compressed=0;
                }


        if(!wave)  
                {  
                fprintf(stderr, "Error opening save file '%s' for reading.\n",*GLOBALS->fileselbox_text);
		perror("Why");
		errno=0;
                }
                else
                {
                char *iline;      
		int s_ctx_iter;

                WAVE_STRACE_ITERATOR(s_ctx_iter)
                        {
                        GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];
                        GLOBALS->strace_ctx->shadow_encountered_parsewavline = 0;
                        }

		if(GLOBALS->traces.total)
			{
			  GLOBALS->group_depth=0;
			  /*		 AddBlankTrace(NULL); in order to terminate any possible collapsed groups */
			}

		if(GLOBALS->is_lx2)
			{
	                while((iline=fgetmalloc(wave)))
	                        {
	                        parsewavline_lx2(iline, NULL, 0);
	                        free_2(iline);
	                        }

			lx2_import_masked();

			if(wave_is_compressed)
		                {
				pclose(wave);
		                wave=popen(str,"r");
		                }
		                else
		                {   
				fclose(wave);
		                wave=fopen(wname,"rb");
		                }

		        if(!wave)  
		                {  
		                fprintf(stderr, "Error opening save file '%s' for reading.\n",*GLOBALS->fileselbox_text);
				perror("Why");
				errno=0;
				return(rc);
		                }
			}

                GLOBALS->default_flags=TR_RJUSTIFY;
		GLOBALS->shift_timebase_default_for_add=LLDescriptor(0);
		GLOBALS->strace_current_window = 0; /* in case there are shadow traces */

		rc = 0;
                while((iline=fgetmalloc(wave)))
                        {
                        parsewavline(iline, NULL, 0);
			GLOBALS->strace_ctx->shadow_encountered_parsewavline |= GLOBALS->strace_ctx->shadow_active;
                        free_2(iline);
			rc++;
                        }

		WAVE_STRACE_ITERATOR(s_ctx_iter)
			{
			GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];

			if(GLOBALS->strace_ctx->shadow_encountered_parsewavline)
				{
				GLOBALS->strace_ctx->shadow_encountered_parsewavline = 0;

				if(GLOBALS->strace_ctx->shadow_straces)
					{
					GLOBALS->strace_ctx->shadow_active = 1;
	
					swap_strace_contexts();
					strace_maketimetrace(1);
					swap_strace_contexts();
	
					GLOBALS->strace_ctx->shadow_active = 0;
					}
				}
			}

                GLOBALS->default_flags=TR_RJUSTIFY;
		GLOBALS->shift_timebase_default_for_add=LLDescriptor(0);
		update_markertime(GLOBALS->tims.marker);
                if(wave_is_compressed) pclose(wave); else fclose(wave);

		if(traces_already_exist) GLOBALS->timestart_from_savefile_valid = 0;

		EnsureGroupsMatch();

		GLOBALS->signalwindow_width_dirty=1;
		MaxSignalLength();
		signalarea_configure_event(GLOBALS->signalarea, NULL);
		wavearea_configure_event(GLOBALS->wavearea, NULL);

		if(!GLOBALS->block_xy_update)
			{
			int x, y;

			get_window_size(&x, &y);
			set_window_size(x, y);
			}
                }

	GLOBALS->current_translate_file = 0;

return(rc);
}


void
menu_read_save_cleanup(GtkWidget *widget, gpointer data)
{
if(GLOBALS->filesel_ok)
	{
	char *wname;

	DEBUG(printf("Read Save Fini: %s\n", *fileselbox_text));

        wname=*GLOBALS->fileselbox_text;
        read_save_helper(wname);
  }
}




void
menu_read_save_file(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
	{
	help_text_bold("\n\nRead Save File");
	help_text(
		" will open a file requester that will ask for the name"
		" of a GTKWave save file.  The contents of the save file"
		" will determine which traces and vectors as well as their"
 		" format (binary, decimal, hex, reverse, etc.) are to be"
		" appended to the display.  Note that the marker positional"
		" data and zoom factor present in the save file will"
		" replace any current settings."
	);
	return;
	}

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

fileselbox("Read Save File",&GLOBALS->filesel_writesave,GTK_SIGNAL_FUNC(menu_read_save_cleanup), GTK_SIGNAL_FUNC(NULL), "*.sav", 0);
}

#if !defined _MSC_VER
/**/
void
menu_read_stems_cleanup(GtkWidget *widget, gpointer data)
{
char *fname ;

if(GLOBALS->filesel_ok)
	{
	DEBUG(printf("Read Stems Fini: %s\n", *fileselbox_text));
        
        fname=*GLOBALS->fileselbox_text;
	if((fname)&&strlen(fname))
		{
	        activate_stems_reader(fname);
		}
	}
}
/**/
void
menu_read_stems_file(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
	{
	help_text_bold("\n\nRead Verilog Stemsfile");
	help_text(
		" will open a file requester that will ask for the name"
		" of a Verilog stemsfile.  This will then launch an RTL browser and allow sourcecode annotation based on"
		" the primary marker position."
		" Stems files are generated by Vermin.  Please see its manpage"
		" for syntax and more information on stems file generation."
	);
	return;
	}

if(!stems_are_active())
	{
	if(GLOBALS->stems_type != WAVE_ANNO_NONE)
		{
		fileselbox("Read Verilog Stemsfile",&GLOBALS->stems_name, GTK_SIGNAL_FUNC(menu_read_stems_cleanup), GTK_SIGNAL_FUNC(NULL), NULL, 0);
		}
		else
		{
		status_text("Unsupported dumpfile type for rtlbrowse.\n");
		}
	}
}
#endif

/**/
void
menu_read_log_cleanup(GtkWidget *widget, gpointer data)
{
char *fname ;

if(GLOBALS->filesel_ok)
	{
	DEBUG(printf("Read Log Fini: %s\n", *fileselbox_text));
        
        fname=*GLOBALS->fileselbox_text;
	if((fname)&&strlen(fname))
		{
	        logbox("Logfile viewer", 480, fname);
		}
	}
}
/**/
void
menu_read_log_file(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
	{
	help_text_bold("\n\nRead Logfile");
	help_text(
		" will open a file requester that will ask for the name"
		" of a plaintext simulation log.  By clicking on the numbers in the logfile,"
		" the marker will jump to the appropriate time value in the wave window."
	);
	return;
	}

fileselbox("Read Logfile",&GLOBALS->filesel_logfile_menu_c_1,GTK_SIGNAL_FUNC(menu_read_log_cleanup), GTK_SIGNAL_FUNC(NULL), NULL, 0);
}

/**/
void
menu_read_script_cleanup(GtkWidget *widget, gpointer data)
{
char *fname ;

if(GLOBALS->filesel_ok)
	{
	DEBUG(printf("Read Script Fini: %s\n", *fileselbox_text));
        
        fname=*GLOBALS->fileselbox_text;
	if((fname)&&strlen(fname))
		{
		execute_script(fname, 0);
		}
	}
}
/**/
void
menu_read_script_file(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
	{
	help_text_bold("\n\nRead Script File");
	help_text(
		" will open a file requester that will ask for the name"
		" of a TCL script to run.  This menu option itself is not callable"
		" by TCL scripts."
	);
	return;
	}

fileselbox("Read Script File",&GLOBALS->filesel_scriptfile_menu,GTK_SIGNAL_FUNC(menu_read_script_cleanup), GTK_SIGNAL_FUNC(NULL), "*.tcl", 0);
}

/**/
void
menu_insert_blank_traces(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {  
        help_text_bold("\n\nInsert Blank");
        help_text(
                " inserts a blank trace after the last highlighted trace."
                " If no traces are highlighted, the blank is inserted after"
		" the last trace."
        );
        return;
        }

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

DEBUG(printf("Insert Blank Trace\n"));

InsertBlankTrace(NULL, 0);
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}

void
menu_insert_analog_height_extension(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {  
        help_text_bold("\n\nInsert Analog Height Extension");
        help_text(
                " inserts a blank analog extension trace after the last highlighted trace."
                " If no traces are highlighted, the blank is inserted after"
		" the last trace.  This type of trace is used to increase the height of analog traces."
        );
        return;
        }

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

DEBUG(printf("Insert Analog Blank Trace\n"));

InsertBlankTrace(NULL, TR_ANALOG_BLANK_STRETCH);
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}
/**/
static void
comment_trace_cleanup(GtkWidget *widget, gpointer data)
{
InsertBlankTrace(GLOBALS->entrybox_text, 0);
if(GLOBALS->entrybox_text) { free_2(GLOBALS->entrybox_text); GLOBALS->entrybox_text=NULL; }
GLOBALS->signalwindow_width_dirty=1;
MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}

void
menu_insert_comment_traces(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {  
        help_text_bold("\n\nInsert Comment");
        help_text(
                " inserts a comment trace after the last highlighted trace."
                " If no traces are highlighted, the comment is inserted after"
		" the last trace."
        );
        return;
        }

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

DEBUG(printf("Insert Comment Trace\n"));

entrybox("Insert Comment Trace",300,"",NULL,128,GTK_SIGNAL_FUNC(comment_trace_cleanup));
}
/**/
static void strace_repcnt_cleanup(GtkWidget *widget, gpointer data)
{
if(GLOBALS->entrybox_text)
	{
	GLOBALS->strace_repeat_count = atoi_64(GLOBALS->entrybox_text);

	free_2(GLOBALS->entrybox_text);
	GLOBALS->entrybox_text=NULL;
	}
}

void menu_strace_repcnt(GtkWidget *widget, gpointer data)
{
char gt[32];

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nSet Pattern Search Repeat Count");
        help_text(
                " sets the number of times that both edge and pattern searches iterate forward or backward when marker forward/backward is selected."
		" Default value is one.  This can be used, for example, to skip forward 10 clock edges at a time rather than a single edge."
        );
        return;
        }

sprintf(gt, "%d", GLOBALS->strace_repeat_count);
entrybox("Repeat Count",300,gt,NULL,20,GTK_SIGNAL_FUNC(strace_repcnt_cleanup));
}
/**/
static void movetotime_cleanup(GtkWidget *widget, gpointer data)
{
if(GLOBALS->entrybox_text)
	{
	TimeType gt;
	char update_string[128];
	char timval[40];
	GtkAdjustment *hadj;
	TimeType pageinc;

        if((GLOBALS->entrybox_text[0] >= 'A' && GLOBALS->entrybox_text[0] <= 'Z')||(GLOBALS->entrybox_text[0] >= 'a' && GLOBALS->entrybox_text[0] <= 'z'))
                {
                int uch = toupper((int)(unsigned char)GLOBALS->entrybox_text[0]);
                gt=GLOBALS->named_markers[uch - 'A'];
                }
                else
                {
                gt=unformat_time(GLOBALS->entrybox_text, GLOBALS->time_dimension);
                }
	free_2(GLOBALS->entrybox_text);
	GLOBALS->entrybox_text=NULL;

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
	sprintf(update_string, "Moved to time: %s\n", timval);
	status_text(update_string);

	time_update();
	}
}

void menu_movetotime(GtkWidget *widget, gpointer data)
{
char gt[32];

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nMove To Time");
        help_text(
                " scrolls the waveform display such that the left border"
                " is the time entered in the requester."
                " Use one of the letters A-Z to move to a named marker."
        );
        return;
        }

reformat_time(gt, GLOBALS->tims.start, GLOBALS->time_dimension);

entrybox("Move To Time",200,gt,NULL,20,GTK_SIGNAL_FUNC(movetotime_cleanup));
}
/**/
static void fetchsize_cleanup(GtkWidget *widget, gpointer data)
{
if(GLOBALS->entrybox_text)
	{
	TimeType fw;
	char update_string[128];
	fw=unformat_time(GLOBALS->entrybox_text, GLOBALS->time_dimension);
	if(fw<1)
		{
		fw=GLOBALS->fetchwindow; /* in case they try to pull 0 or <0 */
		}
		else
		{
		GLOBALS->fetchwindow=fw;
		}
	free_2(GLOBALS->entrybox_text);
	GLOBALS->entrybox_text=NULL;
	sprintf(update_string, "Fetch Size is now: "TTFormat"\n", fw);
	status_text(update_string);
	}
}

void menu_fetchsize(GtkWidget *widget, gpointer data)
{
char fw[32];

if(GLOBALS->helpbox_is_active)   
        {
        help_text_bold("\n\nFetch Size");
        help_text(
                " brings up a requester which allows input of the"
                " number of ticks used for fetch/discard operations."
		"  Default is 100."
        );
        return;
        }

reformat_time(fw, GLOBALS->fetchwindow, GLOBALS->time_dimension);

entrybox("New Fetch Size",200,fw,NULL,20,GTK_SIGNAL_FUNC(fetchsize_cleanup));
}
/**/
static void zoomsize_cleanup(GtkWidget *widget, gpointer data)
{
if(GLOBALS->entrybox_text)
	{
	float f;
	char update_string[128];

	sscanf(GLOBALS->entrybox_text, "%f", &f);
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

	free_2(GLOBALS->entrybox_text);
	GLOBALS->entrybox_text=NULL;
	sprintf(update_string, "Zoom Amount is now: %g\n", f);
	status_text(update_string);
	}
}

void menu_zoomsize(GtkWidget *widget, gpointer data)
{
char za[32];

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nZoom Amount");
        help_text(
                " allows entry of zero or a negative value for the display"
		" zoom.  Zero is no magnification."
        );
        return;
        }


sprintf(za,"%g",(float)(GLOBALS->tims.zoom));

entrybox("New Zoom Amount",200,za,NULL,20,GTK_SIGNAL_FUNC(zoomsize_cleanup));
}
/**/
static void zoombase_cleanup(GtkWidget *widget, gpointer data)
{
if(GLOBALS->entrybox_text)
	{
	float za;
	char update_string[128];
	sscanf(GLOBALS->entrybox_text, "%f", &za);
	if(za>10.0)
		{
		za=10.0;
		}
	else
	if(za<1.5)
		{
		za=1.5;
		}

	GLOBALS->zoombase=(gdouble)za;
	calczoom(GLOBALS->tims.zoom);
	fix_wavehadj();

	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed");
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed");

	free_2(GLOBALS->entrybox_text);
	GLOBALS->entrybox_text=NULL;
	sprintf(update_string, "Zoom Base is now: %g\n", za);
	status_text(update_string);
	}
}

void menu_zoombase(GtkWidget *widget, gpointer data)
{
char za[32];

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nZoom Base");
        help_text(
                " allows entry of a zoom base for the zoom (magnification per integer step)"
		" Allowable values are 1.5 to 10.0.  Default is 2.0."
        );
        return;
        }


sprintf(za,"%g",GLOBALS->zoombase);

entrybox("New Zoom Base Amount",200,za,NULL,20,GTK_SIGNAL_FUNC(zoombase_cleanup));
}
/**/
static void dataformat(int mask, int patch)
{
  Trptr t;
  int fix=0;

  if((t=GLOBALS->traces.first))
    {
      while(t)
	{
	  if(IsSelected(t)&&!IsShadowed(t))
	    {
	      t->flags=((t->flags)&mask)|patch;
	      fix=1;
	    }
	  t=t->t_next;
	}
      if(fix)
	{
	  GLOBALS->signalwindow_width_dirty=1;
	  MaxSignalLength();
	  signalarea_configure_event(GLOBALS->signalarea, NULL);
	  wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
    }
}

void
menu_dataformat_ascii(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-ASCII");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed with ASCII"
		" values."
        );
        return;
        }

dataformat( ~(TR_NUMMASK), TR_ASCII );
}

void
menu_dataformat_real(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-BitsToReal");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed with Real"
		" values.  Note that this only works for 64-bit quantities"
		" and that ones of other sizes will display as binary."
        );
        return;
        }

dataformat( ~(TR_NUMMASK), TR_REAL );
}

void
menu_dataformat_real2bon(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-RealToBits On");
        help_text(
		" will step through all highlighted traces and ensure that"
		" Real vectors with this qualifier will be displayed as Hex"
		" values.  Note that this only works for Real quantities"
		" and other ones will remain to display as binary.  This is a pre-filter"
		" so it is possible to invert, reverse, apply Decimal, etc.  It will not be"
		" possible however to expand those values into their constituent bits."
        );
        return;
        }

dataformat( ~(TR_REAL2BITS|TR_NUMMASK), TR_REAL2BITS|TR_HEX );
}

void
menu_dataformat_real2boff(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-RealToBits Off");
        help_text(
		" will step through all highlighted traces and ensure that"
		" the Real To Bits qualifier is removed from those traces."
        );
        return;
        }

dataformat( ~(TR_REAL2BITS), 0 );
}

void
menu_dataformat_hex(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Hex");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed with hexadecimal"
		" values."
        );
        return;
        }

dataformat( ~(TR_NUMMASK), TR_HEX );
}

void
menu_dataformat_dec(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Decimal");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed with decimal"
		" values."
        );
        return;
        }

dataformat( ~(TR_NUMMASK), TR_DEC );
}

void
menu_dataformat_signed(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Signed");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed as sign extended decimal"
		" values."
        );
        return;
        }

dataformat( ~(TR_NUMMASK), TR_SIGNED );
}

void
menu_dataformat_bin(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Binary");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed with binary"
		" values."
        );
        return;
        }

dataformat( ~(TR_NUMMASK), TR_BIN );
}

void
menu_dataformat_oct(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Octal");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed with octal"
		" values."
        );
        return;
        }

dataformat( ~(TR_NUMMASK), TR_OCT );
}

void
menu_dataformat_rjustify_on(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Right Justify-On");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed right"
		" justified."
        );
        return;
        }

dataformat( ~(TR_RJUSTIFY), TR_RJUSTIFY );
}

void
menu_dataformat_rjustify_off(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Right Justify-Off");
        help_text(
                " will step through all highlighted traces and ensure that"
                " vectors with this qualifier will not be displayed right"       
                " justified."
        );
        return;
        }  

dataformat( ~(TR_RJUSTIFY), 0 );
}

void
menu_dataformat_bingray_on(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Gray Filters-To Gray");
        help_text(
                " will step through all highlighted traces and ensure that"
                " bits and vectors with this qualifier will be displayed after"       
                " going through normal to gray conversion. This is a filter"
		" which sits before other Data Format options such as hex, etc."
        );
        return;
        }  

dataformat( ~(TR_GRAYMASK), TR_BINGRAY );
}

void
menu_dataformat_graybin_on(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Gray Filters-From Gray");
        help_text(
                " will step through all highlighted traces and ensure that"
                " bits and vectors with this qualifier will be displayed after"       
                " going through gray to normal conversion. This is a filter"
                " which sits before other Data Format options such as hex, etc."
        );
        return;
        }  

dataformat( ~(TR_GRAYMASK), TR_GRAYBIN );
}


void
menu_dataformat_nogray(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Gray Filters-None");
        help_text(
                " will step through all highlighted traces and ensure that"
                " bits and vectors with this qualifier will be displayed with"       
                " normal encoding."
        );
        return;
        }  

dataformat( ~(TR_GRAYMASK), 0 );
}

void
menu_dataformat_invert_on(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Invert-On");
        help_text(
                " will step through all highlighted traces and ensure that"
                " bits and vectors with this qualifier will be displayed with"       
                " 1's and 0's inverted."
        );
        return;
        }  

dataformat( ~(TR_INVERT), TR_INVERT );
}

void
menu_dataformat_invert_off(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Invert-Off");
        help_text(
                " will step through all highlighted traces and ensure that"
                " bits and vectors with this qualifier will not be displayed with" 
                " 1's and 0's inverted."                       
        );
        return;
        }  

dataformat( ~(TR_INVERT), 0 );
}

void
menu_dataformat_reverse_on(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Reverse Bits-On");
        help_text(
                " will step through all highlighted traces and ensure that"
                " vectors with this qualifier will be displayed in" 
                " reversed bit order."                       
        );
        return;
        }  

dataformat( ~(TR_REVERSE), TR_REVERSE );
}

void
menu_dataformat_reverse_off(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Reverse Bits-Off");
        help_text(
                " will step through all highlighted traces and ensure that"
                " vectors with this qualifier will not be displayed in"
                " reversed bit order."
        );
        return;   
        }

dataformat( ~(TR_REVERSE), 0 );
}

void
menu_dataformat_exclude_on(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nExclude");
        help_text(
		" causes the waveform data for all currently highlighted traces"
		" to be blanked out."
        );
        return;
        }

dataformat( ~(TR_EXCLUDE), TR_EXCLUDE );
}

void
menu_dataformat_exclude_off(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nShow");
        help_text(
                " causes the waveform data for all currently highlighted traces"
                " to be displayed as normal if the exclude attribute is currently"
		" set on the highlighted traces."            
        );
        return;
        }

dataformat( ~(TR_EXCLUDE), 0 );
}
/**/
void
menu_dataformat_rangefill_zero(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Range Fill With 0s");
        help_text(
                " will step through all highlighted traces and ensure that"
                " vectors with this qualifier will be displayed as if" 
                " the bitrange of the MSB or LSB as appropriate goes to zero."                       
		" Zero bits will be filled in for the missing bits."
        );
        return;
        }  

dataformat( ~(TR_ZEROFILL|TR_ONEFILL), TR_ZEROFILL );
}

void
menu_dataformat_rangefill_one(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Range Fill With 1s");
        help_text(
                " will step through all highlighted traces and ensure that"
                " vectors with this qualifier will be displayed as if" 
                " the bitrange of the MSB or LSB as appropriate goes to zero."                       
		" One bits will be filled in for the missing bits; this is mostly intended"
		" to be used when viewing values which are inverted in the logic and need"
		" to be inverted in the viewer."
        );
        return;
        }  

dataformat( ~(TR_ZEROFILL|TR_ONEFILL), TR_ONEFILL );
}

void
menu_dataformat_rangefill_off(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Zero Range Fill Off");
        help_text(
                " will step through all highlighted traces and ensure that"
		" normal bitrange displays are used."
        );
        return;
        }  

dataformat( ~(TR_ZEROFILL|TR_ONEFILL), 0 );
}
/**/
void
menu_dataformat_analog_off(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nAnalog Off");
        help_text(
                " causes the waveform data for all currently highlighted traces"
                " to be displayed as normal."            
        );
        return;
        }

dataformat( ~(TR_ANALOGMASK), 0 );
}

void
menu_dataformat_analog_step(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nAnalog Step");
        help_text(
                " causes the waveform data for all currently highlighted traces"
                " to be displayed as stepwise analog waveform."            
        );
        return;
        }

dataformat( ~(TR_ANALOGMASK), TR_ANALOG_STEP );
}

void
menu_dataformat_analog_interpol(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nAnalog Interpolate");
        help_text(
                " causes the waveform data for all currently highlighted traces"
                " to be displayed as interpolated analog waveform."            
        );
        return;
        }

dataformat( ~(TR_ANALOGMASK), TR_ANALOG_INTERPOLATED );
}

void
menu_dataformat_analog_interpol_step(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nAnalog Interpolate Annotated");
        help_text(
                " causes the waveform data for all currently highlighted traces"
                " to be displayed as an interpolated analog waveform annotated"
		" with the non-interpolated data sampling points that the cursor snaps to."            
        );
        return;
        }

dataformat( ~(TR_ANALOGMASK), (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP) );
}

void
menu_dataformat_analog_resize_screen(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nAnalog Resizing Screen Data");
        help_text(
                " causes the waveform data for all currently highlighted traces"
                " to be displayed such that the y-value scaling maximizes the on-screen trace"
		" data so if fills the whole trace width at all times."            
        );
        return;
        }

dataformat( ~(TR_ANALOG_FULLSCALE), 0 );
}

void
menu_dataformat_analog_resize_all(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nAnalog Resizing All Data"); 
        help_text(
                " causes the waveform data for all currently highlighted traces"
                " to be displayed such that the y-value scaling maximizes the on-screen trace"
		" data so if fills the whole trace width only when fully zoomed out."            
		" (i.e., the scale used goes across all trace data)"
        );
        return;
        }

dataformat( ~(TR_ANALOG_FULLSCALE), (TR_ANALOG_FULLSCALE) );
}
/**/
void menu_dataformat_highlight_all(GtkWidget *widget, gpointer data)
{
Trptr t;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nHighlight All");
        help_text(
		" simply highlights all displayed traces."
        );
        return;
        }

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

if((t=GLOBALS->traces.first))
	{
	while(t)
		{
		t->flags|=TR_HIGHLIGHT;
		t=t->t_next;
		}
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}

void menu_dataformat_unhighlight_all(GtkWidget *widget, gpointer data)
{
Trptr t;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nUnHighlight All");
        help_text(
                " simply unhighlights all displayed traces."
        );
        return;
        }

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

if((t=GLOBALS->traces.first))
	{
	while(t)
		{
		t->flags&=(~TR_HIGHLIGHT);
		t=t->t_next;
		}
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}

void menu_lexize(GtkWidget *widget, gpointer data)
{
Trptr t;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nSigsort All");
        help_text(
                " sorts all displayed traces with the numeric parts being taken into account.  Blank traces are sorted to the bottom."
        );
        return;
        }

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

if((t=GLOBALS->traces.first))
	{
	if(TracesReorder(TR_SORT_LEX))
		{
		signalarea_configure_event(GLOBALS->signalarea, NULL);
		wavearea_configure_event(GLOBALS->wavearea, NULL);
		}
	}
}
/**/
void menu_alphabetize(GtkWidget *widget, gpointer data)
{
Trptr t;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nAlphabetize All");
        help_text(
                " alphabetizes all displayed traces.  Blank traces are sorted to the bottom."
        );
        return;
        }

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

if((t=GLOBALS->traces.first))
	{
	if(TracesReorder(TR_SORT_NORM))
		{
		signalarea_configure_event(GLOBALS->signalarea, NULL);
		wavearea_configure_event(GLOBALS->wavearea, NULL);
		}
	}
}
/**/
void menu_alphabetize2(GtkWidget *widget, gpointer data)
{
Trptr t;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nAlphabetize All (CaseIns)");
        help_text(
                " alphabetizes all displayed traces without regard to case.  Blank traces are sorted to the bottom."
        );
        return;
        }

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

if((t=GLOBALS->traces.first))
	{
	if(TracesReorder(TR_SORT_INS))
		{
		signalarea_configure_event(GLOBALS->signalarea, NULL);
		wavearea_configure_event(GLOBALS->wavearea, NULL);
		}
	}
}
/**/
void menu_reverse(GtkWidget *widget, gpointer data)
{
Trptr t;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nReverse All");
        help_text(
                " reverses all displayed traces unconditionally."
        );
        return;
        }

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

if((t=GLOBALS->traces.first))
	{
	if(TracesReorder(TR_SORT_RVS))
		{
		signalarea_configure_event(GLOBALS->signalarea, NULL);
		wavearea_configure_event(GLOBALS->wavearea, NULL);
		}
	}
}
/**/
void
menu_cut_traces(GtkWidget *widget, gpointer data)
{
Trptr cutbuffer = NULL;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nCut");
        help_text(
                " removes highlighted signals from the display and places them" 
		" in an offscreen cut/copy buffer for later Paste operations. "
		" Cut implicitly destroys the previous contents of the cut/copy buffer."
        );
        return;
        }                

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

DEBUG(printf("Cut Traces\n"));

cutbuffer = CutBuffer();
if(cutbuffer)
	{
	if(GLOBALS->cutcopylist)
		{
		free_2(GLOBALS->cutcopylist);
		}
	GLOBALS->cutcopylist = emit_gtkwave_savefile_formatted_entries_in_tcl_list(cutbuffer, FALSE);
	/* printf("Cutlist: '%s'\n", GLOBALS->cutcopylist); */

	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
	else
	{
	must_sel();
	}
}

void
menu_copy_traces(GtkWidget *widget, gpointer data)
{
Trptr t = GLOBALS->traces.first;
gboolean highlighted = FALSE;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nCopy");
        help_text(
                " copies highlighted signals from the display and places them" 
		" in an offscreen cut/copy buffer for later Paste operations. "
		" Copy implicitly destroys the previous contents of the cut/copy buffer."
        );
        return;
        }                

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

DEBUG(printf("Copy Traces\n"));

while(t)
	{
	if(t->flags & TR_HIGHLIGHT)
		{
		highlighted = TRUE;
		break;
		}
	t = t->t_next;
	}

if(!highlighted)
	{
	must_sel();
	}
	else
	{
	if(GLOBALS->cutcopylist)
		{
		free_2(GLOBALS->cutcopylist);
		}
	GLOBALS->cutcopylist = emit_gtkwave_savefile_formatted_entries_in_tcl_list(GLOBALS->traces.first, TRUE);
	/* printf("Copylist: '%s'\n", GLOBALS->cutcopylist); */

	FreeCutBuffer();
	}
}

void
menu_paste_traces(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nPaste");
        help_text(
                " pastes signals from"       
                " an offscreen cut/copy buffer and places them in a group after"
		" the last highlighted signal, or at the end of the display"
		" if no signal is highlighted."
        );
        return;
        }

if(GLOBALS->dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

DEBUG(printf("Paste Traces\n"));

if(PasteBuffer())
	{
	GLOBALS->signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
	else
	{
	if(GLOBALS->cutcopylist)
		{
		/*int num_traces =*/ process_tcl_list(GLOBALS->cutcopylist, FALSE);
		/* printf("Pastelist: %d '%s'\n", num_traces, GLOBALS->cutcopylist); */

	        GLOBALS->signalwindow_width_dirty=1;
	        MaxSignalLength();
	        signalarea_configure_event(GLOBALS->signalarea, NULL);
	        wavearea_configure_event(GLOBALS->wavearea, NULL);
		}
	}

}
/**/
void menu_center_zooms(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nCenter Zooms");
        help_text(
		" when enabled"
		" configures zoom in/out operations such that all zooms use the center of the"
		" display as the fixed zoom origin if the primary (unnamed) marker is"
		" not present, otherwise, the primary marker is used as the center origin."
		" When disabled, it"
		" configures zoom in/out operations such that all zooms use the"
		" left margin of the display as the fixed zoom origin."
        );
        }
	else
	{
	GLOBALS->do_zoom_center=(GLOBALS->do_zoom_center)?0:1;
	DEBUG(printf("Center Zooms\n"));
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VCZ].path))->active=(GLOBALS->do_zoom_center)?TRUE:FALSE;
}


void menu_show_base(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nShow Base Symbols");
        help_text(
		" enables the display of leading base symbols ('$' for hex,"
		" '%' for binary, '#' for octal if they are turned off and"
		" disables the drawing of leading base symbols if"
		" they are turned on."
		" Base symbols are displayed by default."
        );
        }
	else
	{
	GLOBALS->show_base=(GLOBALS->show_base)?0:~0;
	GLOBALS->signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	DEBUG(printf("Show Base Symbols\n"));
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VSBS].path))->active=(GLOBALS->show_base)?TRUE:FALSE;
}

/**/
void menu_show_grid(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nShow Grid");
        help_text(
		" toggles the drawing of gridlines in the waveform display."
        );
        }
	else
	{
	GLOBALS->display_grid=(GLOBALS->display_grid)?0:~0;
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)),"changed");
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)),"value_changed");
	DEBUG(printf("Show Grid\n"));
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VSG].path))->active=(GLOBALS->display_grid)?TRUE:FALSE;
}

/**/
void menu_show_mouseover(GtkWidget *widget, gpointer data)
{
if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nShow Mouseover");
        help_text(
		" toggles the dynamic tooltip for signal names and values which follow the marker on mouse button presses in the waveform display."
		" This is useful for examining the values of closely packed value changes without having to zoom outward and without having to"
		" refer to the signal name pane to the left.  Note that an encoded string will be displayed next to the signal name that"
		" indicates what data format flags are currently active for that signal.  Flags are as follows:\n"
                " + = Signed Decimal\n"
                " X = Hexadecimal\n"
                " A = ASCII\n"
                " D = Decimal\n"
                " B = Binary\n"
                " O = Octal\n"
                " J = Right Justify\n"
                " ~ = Invert\n"
                " V = Reverse\n"
                " * = Analog Step+Interpolated\n"   
                " S = Analog Step\n"
                " I = Analog Interpolated\n"
                " R = Real\n"
		" r = Real To Bits\n"
                " 0 = Range Fill with 0s\n"
                " 1 = Range Fill with 1s\n"
		" G = Binary to Gray\n"
		" g = Gray to Binary\n"
        );
        }
	else
	{
	GLOBALS->disable_mouseover=(GLOBALS->disable_mouseover)?0:~0;
	DEBUG(printf("Show Mouseover\n"));
	}

#if !defined __MINGW32__ && !defined _MSC_VER
GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VSMO].path))->active=(GLOBALS->disable_mouseover)?FALSE:TRUE;
#endif
}

/**/


/* this is the GtkMenuEntry structure used to create new menus.  The
 * first member is the menu definition string.  The second, the
 * default accelerator key used to access this menu function with
 * the keyboard.  The third is the callback function to call when
 * this menu item is selected (by the accelerator key, or with the
 * mouse.) The last member is the data to pass to your callback function.
 *
 * ...This has all been changed to use itemfactory stuff which is more
 * powerful.  The only real difference is the final item which tells 
 * the itemfactory just what the item "is".
 */
#ifdef WAVE_USE_MENU_BLACKOUTS
static const char *menu_blackouts[] = { "/Edit", "/Search", "/Time", "/Markers", "/View" };
#endif

static GtkItemFactoryEntry menu_items[] =
{
    WAVE_GTKIFE("/File/Open New Window", "<Control>N", menu_new_viewer, WV_MENU_FONV, "<Item>"),
    WAVE_GTKIFE("/File/Open New Tab", "<Control>T", menu_new_viewer_tab, WV_MENU_FONVT, "<Item>"),
    WAVE_GTKIFE("/File/Reload Waveform", "<Shift><Control>R", menu_reload_waveform, WV_MENU_FRW, "<Item>"),    
    WAVE_GTKIFE("/File/Export/Write VCD File As", NULL, menu_write_vcd_file, WV_MENU_WRVCD, "<Item>"),
    WAVE_GTKIFE("/File/Export/Write LXT File As", NULL, menu_write_lxt_file, WV_MENU_WRLXT, "<Item>"),
    WAVE_GTKIFE("/File/Export/Write TIM File As", NULL, menu_write_tim_file, WV_MENU_WRTIM, "<Item>"),
    WAVE_GTKIFE("/File/Close", "<Control>W", menu_quit_close, WV_MENU_WCLOSE, "<Item>"),
    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_SEP2VCD, "<Separator>"),
    WAVE_GTKIFE("/File/Print To File", "<Control>P", menu_print, WV_MENU_FPTF, "<Item>"),
    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_SEP1, "<Separator>"),
    WAVE_GTKIFE("/File/Read Save File", "<Control>O", menu_read_save_file, WV_MENU_FRSF, "<Item>"),
    WAVE_GTKIFE("/File/Write Save File", "<Control>S", menu_write_save_file, WV_MENU_FWSF, "<Item>"),
    WAVE_GTKIFE("/File/Write Save File As", "<Shift><Control>S", menu_write_save_file_as, WV_MENU_FWSFAS, "<Item>"),
    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_SEP2, "<Separator>"),
    WAVE_GTKIFE("/File/Read Sim Logfile", "L", menu_read_log_file, WV_MENU_FRLF, "<Item>"),
      /* 10 */
    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_SEP2LF, "<Separator>"),
#if !defined _MSC_VER
    WAVE_GTKIFE("/File/Read Verilog Stemsfile", NULL, menu_read_stems_file, WV_MENU_FRSTMF, "<Item>"),
    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_SEP2STMF, "<Separator>"),
#endif
#if defined(HAVE_LIBTCL)
    WAVE_GTKIFE("/File/Read Tcl Script File", NULL, menu_read_script_file, WV_MENU_TCLSCR, "<Item>"),
    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_TCLSEP, "<Separator>"),
#endif
    WAVE_GTKIFE("/File/Quit", "<Control>Q", menu_quit, WV_MENU_FQY, "<Item>"),

    WAVE_GTKIFE("/Edit/Set Trace Max Hier", NULL, menu_set_max_hier, WV_MENU_ESTMH, "<Item>"),
    WAVE_GTKIFE("/Edit/Toggle Trace Hier", "H", menu_toggle_hier, WV_MENU_ETH, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP3, "<Separator>"),
    WAVE_GTKIFE("/Edit/Insert Blank", "<Control>B", menu_insert_blank_traces, WV_MENU_EIB, "<Item>"),
    WAVE_GTKIFE("/Edit/Insert Comment", NULL, menu_insert_comment_traces, WV_MENU_EIC, "<Item>"),
    WAVE_GTKIFE("/Edit/Insert Analog Height Extension", NULL, menu_insert_analog_height_extension, WV_MENU_EIA, "<Item>"),


    WAVE_GTKIFE("/Edit/Cut", "<Control>X", menu_cut_traces, WV_MENU_EC, "<Item>"),
    WAVE_GTKIFE("/Edit/Copy", "<Control>C", menu_copy_traces, WV_MENU_ECY, "<Item>"),
    WAVE_GTKIFE("/Edit/Paste", "<Control>V", menu_paste_traces, WV_MENU_EP, "<Item>"),

    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP3A, "<Separator>"),

    WAVE_GTKIFE("/Edit/Alias Highlighted Trace", "<Alt>A", menu_alias, WV_MENU_EAHT, "<Item>"),
    WAVE_GTKIFE("/Edit/Remove Highlighted Aliases", "<Shift><Alt>A", menu_remove_aliases, WV_MENU_ERHA, "<Item>"),
      /* 20 */
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP4, "<Separator>"),
    WAVE_GTKIFE("/Edit/Expand", "F3", menu_expand, WV_MENU_EE, "<Item>"),
    WAVE_GTKIFE("/Edit/Combine Down", "F4", menu_combine_down, WV_MENU_ECD, "<Item>"),
    WAVE_GTKIFE("/Edit/Combine Up", "F5", menu_combine_up, WV_MENU_ECU, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP5, "<Separator>"),
    WAVE_GTKIFE("/Edit/Data Format/Hex", "<Alt>X", menu_dataformat_hex, WV_MENU_EDFH, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Decimal", "<Alt>D", menu_dataformat_dec, WV_MENU_EDFD, "<Item>"),
      /* 30 */
    WAVE_GTKIFE("/Edit/Data Format/Signed Decimal", NULL, menu_dataformat_signed, WV_MENU_EDFSD, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Binary", "<Alt>B", menu_dataformat_bin, WV_MENU_EDFB, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Octal", "<Alt>O", menu_dataformat_oct, WV_MENU_EDFO, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/ASCII", NULL, menu_dataformat_ascii, WV_MENU_EDFA, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/BitsToReal", NULL, menu_dataformat_real, WV_MENU_EDRL, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/RealToBits/On", NULL, menu_dataformat_real2bon, WV_MENU_EDR2BON, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/RealToBits/Off", NULL, menu_dataformat_real2boff, WV_MENU_EDR2BOFF, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Right Justify/On", "<Alt>J", menu_dataformat_rjustify_on, WV_MENU_EDFRJON, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Right Justify/Off", "<Shift><Alt>J", menu_dataformat_rjustify_off, WV_MENU_EDFRJOFF, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Invert/On", "<Alt>I", menu_dataformat_invert_on, WV_MENU_EDFION, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Invert/Off", "<Shift><Alt>I", menu_dataformat_invert_off, WV_MENU_EDFIOFF, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Reverse Bits/On", "<Alt>V", menu_dataformat_reverse_on, WV_MENU_EDFRON, "<Item>"),
      /* 40 */
    WAVE_GTKIFE("/Edit/Data Format/Reverse Bits/Off", "<Shift><Alt>V", menu_dataformat_reverse_off, WV_MENU_EDFROFF, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Translate Filter File/Disable", NULL, menu_dataformat_xlate_file_0, WV_MENU_XLF_0, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Translate Filter File/Enable and Select", NULL, menu_dataformat_xlate_file_1, WV_MENU_XLF_1, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Translate Filter Process/Disable", NULL, menu_dataformat_xlate_proc_0, WV_MENU_XLP_0, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Translate Filter Process/Enable and Select", NULL, menu_dataformat_xlate_proc_1, WV_MENU_XLP_1, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Transaction Filter Process/Disable", NULL, menu_dataformat_xlate_ttrans_0, WV_MENU_TTXLP_0, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Transaction Filter Process/Enable and Select", NULL, menu_dataformat_xlate_ttrans_1, WV_MENU_TTXLP_1, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Analog/Off", NULL, menu_dataformat_analog_off, WV_MENU_EDFAOFF, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Analog/Step", NULL, menu_dataformat_analog_step, WV_MENU_EDFASTEP, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Analog/Interpolated", NULL, menu_dataformat_analog_interpol, WV_MENU_EDFAINTERPOL, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Analog/Interpolated Annotated", NULL, menu_dataformat_analog_interpol_step, WV_MENU_EDFAINTERPOL2, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Analog/Resizing/Screen Data", NULL, menu_dataformat_analog_resize_screen, WV_MENU_EDFARSD, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Analog/Resizing/All Data", NULL, menu_dataformat_analog_resize_all, WV_MENU_EDFARAD, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Range Fill/With 0s", NULL, menu_dataformat_rangefill_zero, WV_MENU_RFILL0, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Range Fill/With 1s", NULL, menu_dataformat_rangefill_one, WV_MENU_RFILL1, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Range Fill/Off", NULL, menu_dataformat_rangefill_off, WV_MENU_RFILLOFF, "<Item>"),

    WAVE_GTKIFE("/Edit/Data Format/Gray Filters/To Gray", NULL, menu_dataformat_bingray_on, WV_MENU_B2G, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Gray Filters/From Gray", NULL, menu_dataformat_graybin_on, WV_MENU_G2B, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Gray Filters/None", NULL, menu_dataformat_nogray,    WV_MENU_GBNONE, "<Item>"),

    WAVE_GTKIFE("/Edit/Show-Change All Highlighted", NULL, menu_showchangeall, WV_MENU_ESCAH, "<Item>"),
    WAVE_GTKIFE("/Edit/Show-Change First Highlighted", "<Control>F", menu_showchange, WV_MENU_ESCFH, "<Item>"),
      /* 50 */
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP6, "<Separator>"),
    WAVE_GTKIFE("/Edit/Time Warp/Warp Marked", NULL, menu_warp_traces, WV_MENU_WARP, "<Item>"),
    WAVE_GTKIFE("/Edit/Time Warp/Unwarp Marked", NULL, menu_unwarp_traces, WV_MENU_UNWARP, "<Item>"),
    WAVE_GTKIFE("/Edit/Time Warp/Unwarp All", NULL, menu_unwarp_traces_all, WV_MENU_UNWARPA, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP7A, "<Separator>"),
    WAVE_GTKIFE("/Edit/Exclude", "<Shift><Alt>E", menu_dataformat_exclude_on, WV_MENU_EEX, "<Item>"),
    WAVE_GTKIFE("/Edit/Show", "<Shift><Alt>S", menu_dataformat_exclude_off, WV_MENU_ESH, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP6A, "<Separator>"),
    /* WAVE_GTKIFE("/Edit/Expand All Groups", "F12", menu_expand_all, WV_MENU_EXA, "<Item>"), */
    /* WAVE_GTKIFE("/Edit/Collapse All Groups", "<Shift>F12", menu_collapse_all, WV_MENU_CPA, "<Item>"), */
      /* 60 */
    WAVE_GTKIFE("/Edit/Toggle Group Open|Close", "T", menu_toggle_group, WV_MENU_TG, "<Item>"),
    WAVE_GTKIFE("/Edit/Create Group", "G", menu_create_group, WV_MENU_AG, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP6A1, "<Separator>"),
    WAVE_GTKIFE("/Edit/Highlight Regexp", "<Alt>R", menu_regexp_highlight, WV_MENU_EHR, "<Item>"),
    WAVE_GTKIFE("/Edit/UnHighlight Regexp", "<Shift><Alt>R", menu_regexp_unhighlight, WV_MENU_EUHR, "<Item>"),
    WAVE_GTKIFE("/Edit/Highlight All", "<Control>A", menu_dataformat_highlight_all, WV_MENU_EHA, "<Item>"),
    WAVE_GTKIFE("/Edit/UnHighlight All", "<Shift><Control>A", menu_dataformat_unhighlight_all, WV_MENU_EUHA, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP6B, "<Separator>"),
    WAVE_GTKIFE("/Edit/Sort/Alphabetize All", NULL, menu_alphabetize, WV_MENU_ALPHA, "<Item>"),
    WAVE_GTKIFE("/Edit/Sort/Alphabetize All (CaseIns)", NULL, menu_alphabetize2, WV_MENU_ALPHA2, "<Item>"),
    WAVE_GTKIFE("/Edit/Sort/Sigsort All", NULL, menu_lexize, WV_MENU_LEX, "<Item>"),
    WAVE_GTKIFE("/Edit/Sort/Reverse All", NULL, menu_reverse, WV_MENU_RVS, "<Item>"),
      /* 70 */
    WAVE_GTKIFE("/Search/Pattern Search 1", NULL, menu_tracesearchbox, WV_MENU_SPS, "<Item>"),
#ifdef WAVE_USE_GTK2
    WAVE_GTKIFE("/Search/Pattern Search 2", NULL, menu_tracesearchbox, WV_MENU_SPS2, "<Item>"),
#endif
    WAVE_GTKIFE("/Search/<separator>", NULL, NULL, WV_MENU_SEP7B, "<Separator>"),
    WAVE_GTKIFE("/Search/Signal Search Regexp", "<Alt>S", menu_signalsearch, WV_MENU_SSR, "<Item>"),
    WAVE_GTKIFE("/Search/Signal Search Hierarchy", "<Alt>T", menu_hiersearch, WV_MENU_SSH, "<Item>"),
    WAVE_GTKIFE("/Search/Signal Search Tree", "<Shift><Alt>T", menu_treesearch, WV_MENU_SST, "<Item>"),
    WAVE_GTKIFE("/Search/<separator>", NULL, NULL, WV_MENU_SEP7, "<Separator>"),
    WAVE_GTKIFE("/Search/Autocoalesce", NULL, menu_autocoalesce, WV_MENU_ACOL, "<ToggleItem>"),
    WAVE_GTKIFE("/Search/Autocoalesce Reversal", NULL, menu_autocoalesce_reversal, WV_MENU_ACOLR, "<ToggleItem>"),
    WAVE_GTKIFE("/Search/Autoname Bundles", NULL, menu_autoname_bundles_on, WV_MENU_ABON, "<ToggleItem>"),
    WAVE_GTKIFE("/Search/Search Hierarchy Grouping", NULL, menu_hgrouping, WV_MENU_HTGP, "<ToggleItem>"),
      /* 80 */
    WAVE_GTKIFE("/Search/<separator>", NULL, NULL, WV_MENU_SEP7C, "<Separator>"),
    WAVE_GTKIFE("/Search/Set Pattern Search Repeat Count", NULL, menu_strace_repcnt, WV_MENU_STRSE, "<Item>"),

    WAVE_GTKIFE("/Time/Move To Time", "F1", menu_movetotime, WV_MENU_TMTT, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom Amount", "F2", menu_zoomsize, WV_MENU_TZZA, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom Base", "<Shift>F2", menu_zoombase, WV_MENU_TZZB, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom In", "<Alt>Z", service_zoom_in, WV_MENU_TZZI, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom Out", "<Shift><Alt>Z", service_zoom_out, WV_MENU_TZZO, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom Full", "<Alt>F", service_zoom_full, WV_MENU_TZZBFL, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom Best Fit", "<Shift><Alt>F", service_zoom_fit, WV_MENU_TZZBF, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom To Start", "Home", service_zoom_left, WV_MENU_TZZTS, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom To End", "End", service_zoom_right, WV_MENU_TZZTE, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Undo Zoom", "<Alt>U", service_zoom_undo, WV_MENU_TZUZ, "<Item>"),
      /* 90 */
    WAVE_GTKIFE("/Time/Fetch/Fetch Size", "F7", menu_fetchsize, WV_MENU_TFFS, "<Item>"),
    WAVE_GTKIFE("/Time/Fetch/Fetch ->", "<Alt>2", fetch_right, WV_MENU_TFFR, "<Item>"),
    WAVE_GTKIFE("/Time/Fetch/Fetch <-", "<Alt>1", fetch_left, WV_MENU_TFFL, "<Item>"),
    WAVE_GTKIFE("/Time/Discard/Discard ->", "<Alt>4", discard_right, WV_MENU_TDDR, "<Item>"),
    WAVE_GTKIFE("/Time/Discard/Discard <-", "<Alt>3", discard_left, WV_MENU_TDDL, "<Item>"),
    WAVE_GTKIFE("/Time/Shift/Shift ->", "<Alt>6", service_right_shift, WV_MENU_TSSR, "<Item>"),
    WAVE_GTKIFE("/Time/Shift/Shift <-", "<Alt>5", service_left_shift, WV_MENU_TSSL, "<Item>"),
    WAVE_GTKIFE("/Time/Page/Page ->", "<Alt>8", service_right_page, WV_MENU_TPPR, "<Item>"),
    WAVE_GTKIFE("/Time/Page/Page <-", "<Alt>7", service_left_page, WV_MENU_TPPL, "<Item>"),
    WAVE_GTKIFE("/Markers/Show-Change Marker Data", "<Alt>M", menu_markerbox, WV_MENU_MSCMD, "<Item>"),
      /* 100 */
    WAVE_GTKIFE("/Markers/Drop Named Marker", "<Alt>N", drop_named_marker, WV_MENU_MDNM, "<Item>"),
    WAVE_GTKIFE("/Markers/Collect Named Marker", "<Shift><Alt>N", collect_named_marker, WV_MENU_MCNM, "<Item>"),
    WAVE_GTKIFE("/Markers/Collect All Named Markers", "<Shift><Control><Alt>N", collect_all_named_markers, WV_MENU_MCANM, "<Item>"),
    WAVE_GTKIFE("/Markers/Delete Primary Marker", "<Shift><Alt>M", delete_unnamed_marker, WV_MENU_MDPM, "<Item>"),
    WAVE_GTKIFE("/Markers/<separator>", NULL, NULL, WV_MENU_SEP8, "<Separator>"),
    WAVE_GTKIFE("/Markers/Wave Scrolling", "F9", wave_scrolling_on, WV_MENU_MWSON, "<ToggleItem>"),

    WAVE_GTKIFE("/Markers/Locking/Lock to Lesser Named Marker", "Q", lock_marker_left, WV_MENU_MLKLT, "<Item>"),
    WAVE_GTKIFE("/Markers/Locking/Lock to Greater Named Marker", "W", lock_marker_right, WV_MENU_MLKRT, "<Item>"),
    WAVE_GTKIFE("/Markers/Locking/Unlock from Named Marker", "O", unlock_marker, WV_MENU_MLKOFF, "<Item>"),

    WAVE_GTKIFE("/View/Show Grid", "<Alt>G", menu_show_grid, WV_MENU_VSG, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP9, "<Separator>"),
#if !defined _MSC_VER && !defined __MINGW32__
    WAVE_GTKIFE("/View/Show Mouseover", NULL, menu_show_mouseover, WV_MENU_VSMO, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP9A, "<Separator>"),
#endif
    WAVE_GTKIFE("/View/Show Base Symbols", "<Alt>F1", menu_show_base, WV_MENU_VSBS, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP10, "<Separator>"),
      /* 110 */
    WAVE_GTKIFE("/View/Dynamic Resize", "<Alt>9", menu_enable_dynamic_resize, WV_MENU_VDR, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP11, "<Separator>"),
    WAVE_GTKIFE("/View/Center Zooms", "F8", menu_center_zooms, WV_MENU_VCZ, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP12, "<Separator>"),
    WAVE_GTKIFE("/View/Toggle Delta-Frequency", NULL, menu_toggle_delta_or_frequency, WV_MENU_VTDF, "<Item>"),
    WAVE_GTKIFE("/View/Toggle Max-Marker", "F10", menu_toggle_max_or_marker, WV_MENU_VTMM, "<Item>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP13, "<Separator>"),
    WAVE_GTKIFE("/View/Constant Marker Update", "F11", menu_enable_constant_marker_update, WV_MENU_VCMU, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP14, "<Separator>"),
    WAVE_GTKIFE("/View/Draw Roundcapped Vectors", "<Alt>F2", menu_use_roundcaps, WV_MENU_VDRV, "<ToggleItem>"),
      /* 120 */
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP15, "<Separator>"),
    WAVE_GTKIFE("/View/Left Justified Signals", "<Shift>Home", menu_left_justify, WV_MENU_VLJS, "<Item>"),
    WAVE_GTKIFE("/View/Right Justified Signals", "<Shift>End", menu_right_justify, WV_MENU_VRJS, "<Item>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP16, "<Separator>"),
    WAVE_GTKIFE("/View/Zoom Pow10 Snap", "<Shift>Pause", menu_zoom10_snap, WV_MENU_VZPS, "<ToggleItem>"),
    WAVE_GTKIFE("/View/Partial VCD Dynamic Zoom Full", NULL, menu_zoom_dynf, WV_MENU_VZDYN, "<ToggleItem>"),
    WAVE_GTKIFE("/View/Partial VCD Dynamic Zoom To End", NULL, menu_zoom_dyne, WV_MENU_VZDYNE, "<ToggleItem>"),
    WAVE_GTKIFE("/View/Full Precision", "<Alt>Pause", menu_use_full_precision, WV_MENU_VFTP, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP17, "<Separator>"),
    WAVE_GTKIFE("/View/Remove Pattern Marks", NULL, menu_remove_marked, WV_MENU_RMRKS, "<Item>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP17A, "<Separator>"),
    WAVE_GTKIFE("/View/Use Color", NULL, menu_use_color, WV_MENU_USECOLOR, "<Item>"),
    WAVE_GTKIFE("/View/Use Black and White", NULL, menu_use_bw, WV_MENU_USEBW, "<Item>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP18, "<Separator>"),
    WAVE_GTKIFE("/View/LXT Clock Compress to Z", NULL, menu_lxt_clk_compress, WV_MENU_LXTCC2Z, "<ToggleItem>"),

    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP19, "<Separator>"),
    WAVE_GTKIFE("/View/Scale To Time Dimension/None", NULL, menu_scale_to_td_x, WV_MENU_TDSCALEX, "<ToggleItem>"),
    WAVE_GTKIFE("/View/Scale To Time Dimension/sec",  NULL, menu_scale_to_td_s, WV_MENU_TDSCALES, "<ToggleItem>"),
    WAVE_GTKIFE("/View/Scale To Time Dimension/ms",   NULL, menu_scale_to_td_m, WV_MENU_TDSCALEM, "<ToggleItem>"),
    WAVE_GTKIFE("/View/Scale To Time Dimension/us",   NULL, menu_scale_to_td_u, WV_MENU_TDSCALEU, "<ToggleItem>"),
    WAVE_GTKIFE("/View/Scale To Time Dimension/ns",   NULL, menu_scale_to_td_n, WV_MENU_TDSCALEN, "<ToggleItem>"),
    WAVE_GTKIFE("/View/Scale To Time Dimension/ps",   NULL, menu_scale_to_td_p, WV_MENU_TDSCALEP, "<ToggleItem>"),
    WAVE_GTKIFE("/View/Scale To Time Dimension/fs",   NULL, menu_scale_to_td_f, WV_MENU_TDSCALEF, "<ToggleItem>"),

      /* 130 */
    WAVE_GTKIFE("/Help/WAVE Help", "<Control>H", menu_help, WV_MENU_HWH, "<Item>"),
    WAVE_GTKIFE("/Help/Wave Version", NULL, menu_version, WV_MENU_HWV, "<Item>"),
};


void set_scale_to_time_dimension_toggles(void)
{
int i;

for(i = WV_MENU_TDSCALEX; i<= WV_MENU_TDSCALEF; i++)
	{
	GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[i].path))->active=FALSE;
	}

switch(GLOBALS->scale_to_time_dimension)
	{
	case 's':	i = WV_MENU_TDSCALES; break;
	case 'm':	i = WV_MENU_TDSCALEM; break;
	case 'u':	i = WV_MENU_TDSCALEU; break;
	case 'n':	i = WV_MENU_TDSCALEN; break;
	case 'p':	i = WV_MENU_TDSCALEP; break;
	case 'f':	i = WV_MENU_TDSCALEF; break;
	default: 	i = WV_MENU_TDSCALEX; break;
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[i].path))->active=TRUE;
}


/*
 * set toggleitems to their initial states
 */
static void set_menu_toggles(void)
{
GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VZPS].path))->active=(GLOBALS->zoom_pow10_snap)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VSG].path))->active=(GLOBALS->display_grid)?TRUE:FALSE;

#if !defined __MINGW32__ && !defined _MSC_VER
GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1,menu_items[WV_MENU_VSMO].path))->active=(GLOBALS->disable_mouseover)?FALSE:TRUE;
#endif

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VSBS].path))->active=(GLOBALS->show_base)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VDR].path))->active=(GLOBALS->do_resize_signals)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VCMU].path))->active=(GLOBALS->constant_marker_update)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VCZ].path))->active=(GLOBALS->do_zoom_center)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VDRV].path))->active=(GLOBALS->use_roundcaps)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_MWSON].path))->active=(GLOBALS->wave_scrolling)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_ABON].path))->active=(GLOBALS->autoname_bundles)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_HTGP].path))->active=(GLOBALS->hier_grouping)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VFTP].path))->active=(GLOBALS->use_full_precision)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_ACOL].path))->active=(GLOBALS->autocoalesce)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_ACOLR].path))->active=(GLOBALS->autocoalesce_reversal)?TRUE:FALSE;

if(GLOBALS->partial_vcd)
	{
	GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VZDYN].path))->active=(GLOBALS->zoom_dyn)?TRUE:FALSE;
	GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VZDYNE].path))->active=(GLOBALS->zoom_dyne)?TRUE:FALSE;
	}

if(GLOBALS->loaded_file_type == LXT_FILE)
	{
	GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_LXTCC2Z].path))->active=(GLOBALS->lxt_clock_compress_to_z)?TRUE:FALSE;
	}

set_scale_to_time_dimension_toggles();
}


/*
 * create the menu through an itemfactory instance
 */
void get_main_menu(GtkWidget *window, GtkWidget ** menubar)
{
    int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
    GtkAccelGroup *global_accel;
    int i;
    GtkWidget *mw;

    GLOBALS->regexp_string_menu_c_1 = calloc_2(1, 129);

    global_accel = gtk_accel_group_new();
    GLOBALS->item_factory_menu_c_1 = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", global_accel);
    gtk_item_factory_create_items(GLOBALS->item_factory_menu_c_1, nmenu_items, menu_items, NULL);

    if(GLOBALS->loaded_file_type == MISSING_FILE)
	{
	    for(i=0;i<nmenu_items;i++)
		{
		switch(i)
			{
			case WV_MENU_FONVT:
			case WV_MENU_WCLOSE:
#if defined(HAVE_LIBTCL)
	    		case WV_MENU_TCLSCR:
#endif
			case WV_MENU_FQY:
			case WV_MENU_HWH:
			case WV_MENU_HWV:
				break;
	
			default: 
				mw = gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[i].path);
				if(mw) gtk_widget_set_sensitive(mw, FALSE);
				break;
			}
		}
	
#ifdef WAVE_USE_MENU_BLACKOUTS
		for(i=0;i<(sizeof(menu_blackouts)/sizeof(char *));i++)
			{
			mw = gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_blackouts[i]);
			if(mw) gtk_widget_set_sensitive(mw, FALSE);
			}
#endif
	}

    if(
#ifdef WAVE_USE_GTK2
	(GLOBALS->socket_xid)||
#endif
	(GLOBALS->partial_vcd))
	{
	gtk_item_factory_delete_item(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_FONVT].path);
	}

    if(!GLOBALS->partial_vcd)
	{
	gtk_item_factory_delete_item(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VZDYN].path);
	gtk_item_factory_delete_item(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_VZDYNE].path);
	}

    if(GLOBALS->loaded_file_type == DUMPLESS_FILE)
	{
    	gtk_item_factory_delete_item(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_FRW].path);
	}

    if(GLOBALS->loaded_file_type != LXT_FILE)
	{
	gtk_item_factory_delete_item(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_SEP18].path);
    	gtk_item_factory_delete_item(GLOBALS->item_factory_menu_c_1, menu_items[WV_MENU_LXTCC2Z].path);
	}

    gtk_window_add_accel_group(GTK_WINDOW(window), global_accel);
    if(menubar)
	{
	*menubar = gtk_item_factory_get_widget (GLOBALS->item_factory_menu_c_1, "<main>");
        set_menu_toggles();
	}
}


void menu_set_sensitive(void)
{
    int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
    int i;
    GtkWidget *mw;

#ifdef WAVE_USE_MENU_BLACKOUTS
    for(i=0;i<(sizeof(menu_blackouts)/sizeof(char *));i++)
	{
	mw = gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_blackouts[i]);
	if(mw) gtk_widget_set_sensitive(mw, TRUE);
	}
#endif

    for(i=0;i<nmenu_items;i++)
        {
        switch(i)
                {
                case WV_MENU_FONVT:
                case WV_MENU_WCLOSE:
#if defined(HAVE_LIBTCL)
    		case WV_MENU_TCLSCR:
#endif
                case WV_MENU_FQY:
                case WV_MENU_HWH:
                case WV_MENU_HWV:  
                        break;

                default:
                        mw = gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_items[i].path);
                        if(mw) gtk_widget_set_sensitive(mw, TRUE);
                        break;
                }
        }
}

/*
 * bail out
 */
int file_quit_cmd_callback (GtkWidget *widget, gpointer data)
{
if(!GLOBALS->enable_fast_exit)
	{
	simplereqbox("Quit Program",300,"Do you really want to quit?","Yes", "No", GTK_SIGNAL_FUNC(menu_quit_callback), 1);
	}
	else
	{
#ifdef __CYGWIN__
	kill_stems_browser();
#endif	
	g_print ("WM Destroy\n");
	gtk_exit(0);	
	}

return(TRUE); /* keeps "delete_event" from happening...we'll manually destory later if need be */
}


/*
 * RPC
 */
int execute_script(char *name, int dealloc_name)
{
int i;
int nlen = strlen(name);

if(GLOBALS->tcl_running)
	{
	fprintf(stderr, "Could not run script file '%s', as one is already running.\n", name);

	if(dealloc_name)
		{
		free_2(name);
		}

	return(0);	
	}

GLOBALS->tcl_running = 1;

if(1) /* all scripts are Tcl now */
	{
#if defined(HAVE_LIBTCL)
	int tclrc;
	char *tcl_cmd = wave_alloca(7 + nlen + 1); /* originally a malloc, but the script can change the context! */
	strcpy(tcl_cmd, "source ");
	strcpy(tcl_cmd+7, name);

	fprintf(stderr, "GTKWAVE | Executing Tcl script '%s'\n", name);

	if(dealloc_name)
		{
		free_2(name);
		}

#ifdef WIN32
	{
	char *slashfix = tcl_cmd;
	while(*slashfix)
		{
		if(*slashfix=='\\') *slashfix='/';
		slashfix++;
		}
	}
#endif

	tclrc = Tcl_Eval (GLOBALS->interp, tcl_cmd);
	if(tclrc != TCL_OK) { fprintf (stderr, "GTKWAVE | %s\n", Tcl_GetStringResult (GLOBALS->interp)); }
#else
	fprintf(stderr, "GTKWAVE | Tcl support not compiled into gtkwave, exiting.\n");
	gtk_exit(255);
#endif
	}

for(i=0;i<GLOBALS->num_notebook_pages;i++)
	{
        (*GLOBALS->contexts)[i]->wave_script_args = NULL; /* just in case there was a CTX swap */
	}

GLOBALS->tcl_running = 0;
return(0);
}


GtkItemFactoryEntry *retrieve_menu_items_array(int *num_items)
{
*num_items = sizeof(menu_items) / sizeof(menu_items[0]);

return(menu_items);
}


/*
 * support for menu accelerator modifications...
 */
int set_wave_menu_accelerator(char *str)
{
char *path, *pathend;
char *accel;
int i;

path = strchr(str, '\"');
if(!path) return(1);
path++;
if(!*path) return(1);

pathend = strchr(path+1, '\"');
if(!path) return(1);

*pathend = 0;

accel = pathend + 1;
while(*accel)
	{
	if(!isspace((int)(unsigned char)*accel)) break;
	accel++;
	}

if(!*accel) return(1);

if(strstr(path, "<separator>")) return(1);
if(!strcmp(accel, "(null)")) 
	{
	accel = NULL;
	}
	else
	{
	for(i=0;i<WV_MENU_NUMITEMS;i++)
		{
		if(menu_items[i].accelerator)
			{
			if(!strcmp(menu_items[i].accelerator, accel))
				{
				menu_items[i].accelerator = NULL;
				}
			}
		}
	}

for(i=0;i<WV_MENU_NUMITEMS;i++)
	{
	if(menu_items[i].path)
		{
		if(!strcmp(menu_items[i].path, path))
			{
			menu_items[i].accelerator = accel ? strdup_2(accel) : NULL;
			break;
			}
		}
	}

return(0);
}


/***********************/
/*** popup menu code ***/
/***********************/

static GtkItemFactoryEntry popmenu_items[] =
{
    WAVE_GTKIFE("/Data Format/Hex", NULL, menu_dataformat_hex, WV_MENU_EDFH, "<Item>"),
    WAVE_GTKIFE("/Data Format/Decimal", NULL, menu_dataformat_dec, WV_MENU_EDFD, "<Item>"),
    WAVE_GTKIFE("/Data Format/Signed Decimal", NULL, menu_dataformat_signed, WV_MENU_EDFSD, "<Item>"),
    WAVE_GTKIFE("/Data Format/Binary", NULL, menu_dataformat_bin, WV_MENU_EDFB, "<Item>"),
    WAVE_GTKIFE("/Data Format/Octal", NULL, menu_dataformat_oct, WV_MENU_EDFO, "<Item>"),
    WAVE_GTKIFE("/Data Format/ASCII", NULL, menu_dataformat_ascii, WV_MENU_EDFA, "<Item>"),
    WAVE_GTKIFE("/Data Format/BitsToReal", NULL, menu_dataformat_real, WV_MENU_EDRL, "<Item>"),
    WAVE_GTKIFE("/Data Format/RealToBits/On", NULL, menu_dataformat_real2bon, WV_MENU_EDR2BON, "<Item>"),
    WAVE_GTKIFE("/Data Format/RealToBits/Off", NULL, menu_dataformat_real2boff, WV_MENU_EDR2BOFF, "<Item>"),
    WAVE_GTKIFE("/Data Format/Right Justify/On", NULL, menu_dataformat_rjustify_on, WV_MENU_EDFRJON, "<Item>"),
    WAVE_GTKIFE("/Data Format/Right Justify/Off", NULL, menu_dataformat_rjustify_off, WV_MENU_EDFRJOFF, "<Item>"),
    WAVE_GTKIFE("/Data Format/Invert/On", NULL, menu_dataformat_invert_on, WV_MENU_EDFION, "<Item>"),
    WAVE_GTKIFE("/Data Format/Invert/Off", NULL, menu_dataformat_invert_off, WV_MENU_EDFIOFF, "<Item>"),
    WAVE_GTKIFE("/Data Format/Reverse Bits/On", NULL, menu_dataformat_reverse_on, WV_MENU_EDFRON, "<Item>"),
    WAVE_GTKIFE("/Data Format/Reverse Bits/Off", NULL, menu_dataformat_reverse_off, WV_MENU_EDFROFF, "<Item>"),
    WAVE_GTKIFE("/Data Format/Translate Filter File/Disable", NULL, menu_dataformat_xlate_file_0, WV_MENU_XLF_0, "<Item>"),
    WAVE_GTKIFE("/Data Format/Translate Filter File/Enable and Select", NULL, menu_dataformat_xlate_file_1, WV_MENU_XLF_1, "<Item>"),
    WAVE_GTKIFE("/Data Format/Translate Filter Process/Disable", NULL, menu_dataformat_xlate_proc_0, WV_MENU_XLP_0, "<Item>"),
    WAVE_GTKIFE("/Data Format/Translate Filter Process/Enable and Select", NULL, menu_dataformat_xlate_proc_1, WV_MENU_XLP_1, "<Item>"),
    WAVE_GTKIFE("/Data Format/Transaction Filter Process/Disable", NULL, menu_dataformat_xlate_ttrans_0, WV_MENU_TTXLP_0, "<Item>"),
    WAVE_GTKIFE("/Data Format/Transaction Filter Process/Enable and Select", NULL, menu_dataformat_xlate_ttrans_1, WV_MENU_TTXLP_1, "<Item>"),
    WAVE_GTKIFE("/Data Format/Analog/Off", NULL, menu_dataformat_analog_off, WV_MENU_EDFAOFF, "<Item>"),
    WAVE_GTKIFE("/Data Format/Analog/Step", NULL, menu_dataformat_analog_step, WV_MENU_EDFASTEP, "<Item>"),
    WAVE_GTKIFE("/Data Format/Analog/Interpolated", NULL, menu_dataformat_analog_interpol, WV_MENU_EDFAINTERPOL, "<Item>"),
    WAVE_GTKIFE("/Data Format/Analog/Interpolated Annotated", NULL, menu_dataformat_analog_interpol_step, WV_MENU_EDFAINTERPOL2, "<Item>"),
    WAVE_GTKIFE("/Data Format/Analog/Resizing/Screen Data", NULL, menu_dataformat_analog_resize_screen, WV_MENU_EDFARSD, "<Item>"),
    WAVE_GTKIFE("/Data Format/Analog/Resizing/All Data", NULL, menu_dataformat_analog_resize_all, WV_MENU_EDFARAD, "<Item>"),

    WAVE_GTKIFE("/Data Format/Range Fill/With 0s", NULL, menu_dataformat_rangefill_zero, WV_MENU_RFILL0, "<Item>"),
    WAVE_GTKIFE("/Data Format/Range Fill/With 1s", NULL, menu_dataformat_rangefill_one, WV_MENU_RFILL1, "<Item>"),
    WAVE_GTKIFE("/Data Format/Range Fill/Off", NULL, menu_dataformat_rangefill_off, WV_MENU_RFILLOFF, "<Item>"),

    WAVE_GTKIFE("/Data Format/Gray Filters/To Gray", NULL, menu_dataformat_bingray_on, WV_MENU_B2G, "<Item>"),
    WAVE_GTKIFE("/Data Format/Gray Filters/From Gray", NULL, menu_dataformat_graybin_on, WV_MENU_G2B, "<Item>"),
    WAVE_GTKIFE("/Data Format/Gray Filters/None", NULL, menu_dataformat_nogray,    WV_MENU_GBNONE, "<Item>"),

    WAVE_GTKIFE("/<separator>", NULL, NULL, WV_MENU_SEP1, "<Separator>"),
    WAVE_GTKIFE("/Insert Analog Height Extension", NULL, menu_insert_analog_height_extension, WV_MENU_EIA, "<Item>"),
    WAVE_GTKIFE("/<separator>", NULL, NULL, WV_MENU_SEP2, "<Separator>"),

    WAVE_GTKIFE("/Insert Blank", NULL, menu_insert_blank_traces, WV_MENU_EIB, "<Item>"),
    WAVE_GTKIFE("/Insert Comment", NULL, menu_insert_comment_traces, WV_MENU_EIC, "<Item>"),
    WAVE_GTKIFE("/Alias Highlighted Trace", NULL, menu_alias, WV_MENU_EAHT, "<Item>"),
    WAVE_GTKIFE("/Remove Highlighted Aliases", NULL, menu_remove_aliases, WV_MENU_ERHA, "<Item>"),
    WAVE_GTKIFE("/<separator>", NULL, NULL, WV_MENU_SEP3, "<Separator>"),
    WAVE_GTKIFE("/Cut", NULL, menu_cut_traces, WV_MENU_EC, "<Item>"),
    WAVE_GTKIFE("/Copy", NULL, menu_copy_traces, WV_MENU_ECY, "<Item>"),
    WAVE_GTKIFE("/Paste", NULL, menu_paste_traces, WV_MENU_EP, "<Item>")
};


void do_popup_menu (GtkWidget *my_widget, GdkEventButton *event)
{
  GtkWidget *menu;
  int button, event_time;

  if(!GLOBALS->signal_popup_menu)
    {
    int nmenu_items = sizeof(popmenu_items) / sizeof(popmenu_items[0]);
    GtkItemFactory *item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", NULL);
    gtk_item_factory_create_items (item_factory, nmenu_items, popmenu_items, NULL);
    GLOBALS->signal_popup_menu = menu = gtk_item_factory_get_widget (item_factory, "<main>");
    }
    else
    {
    menu = GLOBALS->signal_popup_menu;
    }

  if (event)
    {
      button = event->button;
      event_time = event->time;
    }
  else
    {
      button = 0;
#if WAVE_USE_GTK2
      event_time = gtk_get_current_event_time ();
#else
      return; /* disabled in GTK1.2 */
#endif
    }

  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 
                  button, event_time);
}

void SetTraceScrollbarRowValue(int row, unsigned location)
{
  if(row >= 0)
    {
      GtkAdjustment *wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
      int target = row;

      int num_traces_displayable=(GLOBALS->signalarea->allocation.height)/(GLOBALS->fontheight);
      num_traces_displayable--;   /* for the time trace that is always there */

      /* center */
      if (location == 1) {
	target = target - num_traces_displayable/2;
      }

      /* end */
      if (location == 2) {
	target = target - num_traces_displayable;
      }

      if(target > GLOBALS->traces.visible - num_traces_displayable) target = GLOBALS->traces.visible - num_traces_displayable;

      if(target < 0) target = 0;

      wadj->value = target;

      gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed"); /* force bar update */
      gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /* force text update */
      gtkwave_main_iteration();

    }
}

/*
 * $Id$
 * $Log$
 * Revision 1.116  2010/10/06 20:15:51  gtkwave
 * preliminary version of RPC mechanism
 *
 * Revision 1.115  2010/10/02 18:58:55  gtkwave
 * ctype.h compiler warning fixes (char vs int)
 *
 * Revision 1.114  2010/08/26 18:42:23  gtkwave
 * added support for transaction filters in mingw
 *
 * Revision 1.113  2010/08/26 01:32:01  gtkwave
 * added mingw support for Open New Window menu option
 *
 * Revision 1.112  2010/08/25 22:58:23  gtkwave
 * added process file support for mingw
 *
 * Revision 1.111  2010/07/27 22:16:11  gtkwave
 * added cbOpenTraceGroup and cbCloseTraceGroup
 *
 * Revision 1.110  2010/07/27 19:25:41  gtkwave
 * initial tcl callback function adds
 *
 * Revision 1.109  2010/07/12 20:51:42  gtkwave
 * free to non-malloc'd address fix
 *
 * Revision 1.108  2010/06/29 02:05:14  gtkwave
 * changed quit accelerator
 *
 * Revision 1.107  2010/06/22 19:57:12  gtkwave
 * remove don't quit option on fast exit (via rc file)
 *
 * Revision 1.106  2010/06/13 20:37:59  gtkwave
 * added strace repeat count
 *
 * Revision 1.105  2010/04/14 07:45:12  gtkwave
 * P -> O to avoid accelerator conflict with psec
 *
 * Revision 1.104  2010/04/14 03:28:39  gtkwave
 * change lock/unlock from 1/2/0 to Q/W/P
 *
 * Revision 1.103  2010/04/12 23:07:12  gtkwave
 * add ability to make single signals transactions
 *
 * Revision 1.102  2010/04/07 01:50:45  gtkwave
 * improved name handling for bvname, add $next transaction operation
 *
 * Revision 1.101  2010/04/04 19:09:57  gtkwave
 * rename name->bvname in struct BitVector for easier grep tracking
 *
 * Revision 1.100  2010/03/31 15:42:47  gtkwave
 * added preliminary transaction filter support
 *
 * Revision 1.99  2010/03/24 23:05:09  gtkwave
 * added RealToBits menu option
 *
 * Revision 1.98  2010/03/18 17:12:37  gtkwave
 * pedantic warning cleanups
 *
 * Revision 1.97  2010/03/14 07:09:49  gtkwave
 * removed ExtNode and merged with Node
 *
 * Revision 1.96  2010/03/10 18:14:13  gtkwave
 * Toggle Trace Hier fix (cache old value for more usability)
 *
 * Revision 1.95  2010/03/10 04:00:07  gtkwave
 * fixed compiler warnings for 64-bit
 *
 * Revision 1.94  2010/02/24 18:06:14  gtkwave
 * removed reduce single bit vectors
 *
 * Revision 1.93  2010/02/24 17:35:35  gtkwave
 * gtk1 compile fixes
 *
 * Revision 1.92  2010/02/18 23:06:04  gtkwave
 * change name of main iteration loop calls
 *
 * Revision 1.91  2010/01/22 20:24:43  gtkwave
 * sanity checking on pattern search ID value
 *
 * Revision 1.90  2010/01/22 16:51:21  gtkwave
 * fixes to ensure WAVE_NUM_STRACE_WINDOWS is more maintainable
 *
 * Revision 1.89  2010/01/22 02:10:49  gtkwave
 * added second pattern search capability
 *
 * Revision 1.88  2010/01/20 21:39:20  gtkwave
 * move to time support of named markers
 * 
 * Revision 1.87  2010/01/19 02:19:43  gtkwave
 * left lock starts at highest defined marker now
 *
 * Revision 1.86  2010/01/19 02:06:28  gtkwave
 * update stale marker time values on lock left/right ops
 *
 * Revision 1.85  2010/01/18 20:47:02  gtkwave
 * added named locker marking
 *
 * Revision 1.84  2009/12/24 20:55:27  gtkwave
 * warnings cleanups
 *
 * Revision 1.83  2009/12/24 19:11:26  gtkwave
 * warnings fixes
 *
 * Revision 1.82  2009/12/15 23:40:59  gtkwave
 * removed old style scripts; also removed tempfiles for Tcl args
 *
 * Revision 1.81  2009/12/11 19:48:59  gtkwave
 * mingw tcl fixes
 *
 * Revision 1.80  2009/12/02 05:16:38  gtkwave
 * revamped the gray help text to use normal instead of binary
 *
 * Revision 1.79  2009/12/01 21:26:55  gtkwave
 * renamed B2G and G2B options
 *
 * Revision 1.78  2009/11/25 09:49:28  gtkwave
 * added gray code support
 *
 * Revision 1.77  2009/11/06 04:15:17  gtkwave
 * gtk+-1.2 compile fixes
 *
 * Revision 1.76  2009/11/05 23:11:09  gtkwave
 * added EnsureGroupsMatch()
 *
 * Revision 1.75  2009/11/05 20:09:16  gtkwave
 * anonymous group name fix
 *
 * Revision 1.74  2009/11/03 07:08:21  gtkwave
 * enabled reverse when groups present
 *
 * Revision 1.73  2009/11/02 03:10:51  gtkwave
 * documentation updates
 *
 * Revision 1.72  2009/09/22 13:51:13  gtkwave
 * warnings fixes
 *
 * Revision 1.71  2009/09/14 03:00:08  gtkwave
 * bluespec code integration
 *
 * Revision 1.70  2009/07/16 15:42:13  gtkwave
 * only copy over translate filenames for now (all the others on a new tab
 * might be disruptive)
 *
 * Revision 1.69  2009/07/16 15:36:34  gtkwave
 * file name updates...derive unspecified names from loaded file name directory
 *
 * Revision 1.68  2009/07/01 21:58:32  gtkwave
 * more GHW module type adds for icons in hierarchy window
 *
 * Revision 1.67  2009/07/01 18:22:35  gtkwave
 * added VHDL (GHW) instance types as icons
 *
 * Revision 1.66  2009/07/01 07:39:12  gtkwave
 * decorating hierarchy tree with module type info
 *
 * Revision 1.65  2009/04/24 20:00:03  gtkwave
 * rtlbrowse exit kill mods
 *
 * Revision 1.64  2009/04/24 05:42:33  gtkwave
 * clear out t->minmax_valid in dataformat() for sign changes on analog
 * vectors that use full data range caching
 *
 * Revision 1.63  2009/04/24 04:24:22  gtkwave
 * reload and cygwin fixes for rtlbrowse
 *
 * Revision 1.62  2009/04/23 21:57:53  gtkwave
 * added mingw support for rtlbrowse
 *
 * Revision 1.61  2009/04/10 16:24:35  gtkwave
 * made blackouts ifdef'd out for now on menus
 *
 * Revision 1.60  2009/04/10 04:33:58  gtkwave
 * added menu blackouts on empty gui
 *
 * Revision 1.59  2009/03/27 04:38:05  gtkwave
 * working on ergonomics of drag and drop into an empty gui
 *
 * Revision 1.58  2009/03/26 20:57:41  gtkwave
 * added MISSING_FILE support for bringing up gtkwave without a dumpfile
 *
 * Revision 1.57  2009/02/16 20:55:10  gtkwave
 * added extload to dnd filetypes.  set window title on tab close
 *
 * Revision 1.56  2009/01/12 04:17:39  gtkwave
 * added dynamic zoom for end for partial vcd
 *
 * Revision 1.55  2009/01/06 20:04:58  gtkwave
 * added copy to right click menu
 *
 * Revision 1.54  2008/12/28 03:03:34  gtkwave
 * Added scale_to_time_dimension rc variable and menu options.
 *
 * Revision 1.53  2008/12/25 04:07:29  gtkwave
 * -Wshadow warning fixes
 *
 * Revision 1.52  2008/12/20 07:44:22  gtkwave
 * experimental support for Tcl repscripts
 *
 * Revision 1.51  2008/12/16 19:28:20  gtkwave
 * more warnings cleanups
 *
 * Revision 1.50  2008/12/11 21:08:15  gtkwave
 * allow support for marker names which are strings
 *
 * Revision 1.49  2008/12/06 19:55:53  gtkwave
 * more adds to the timinganalyzer output writer
 *
 * Revision 1.48  2008/12/05 19:44:08  gtkwave
 * prelim support for timinganalyzer file format export
 *
 * Revision 1.47  2008/12/04 19:21:00  gtkwave
 * added zoom_dynamic option for partial VCD loading
 *
 * Revision 1.46  2008/11/27 19:37:38  gtkwave
 * disable filter process in windows compiles
 *
 * Revision 1.45  2008/11/25 18:07:32  gtkwave
 * added cut copy paste functionality that survives reload and can do
 * multiple pastes on the same cut buffer
 *
 * Revision 1.44  2008/11/24 02:55:10  gtkwave
 * use TCL_INCLUDE_SPEC to fix ubuntu compiles
 *
 * Revision 1.43  2008/11/23 05:51:51  gtkwave
 * fixed missing carriage return in error message for Tcl
 *
 * Revision 1.42  2008/11/19 19:08:34  gtkwave
 * add tcl menu option, update file requester name reentrancy code
 *
 * Revision 1.41  2008/11/19 18:15:35  gtkwave
 * add HAVE_LIBTCL to ifdefs which have HAVE_TCL_H
 *
 * Revision 1.40  2008/10/14 00:53:46  gtkwave
 * enabled tcl scripts to call existing gtkwave style scripted menu functions
 *
 * Revision 1.39  2008/10/02 00:52:25  gtkwave
 * added dnd of external filetypes into viewer
 *
 * Revision 1.38  2008/09/27 19:08:39  gtkwave
 * compiler warning fixes
 *
 * Revision 1.37  2008/09/24 23:41:22  gtkwave
 * drag from signal window into external process
 *
 * Revision 1.36  2008/08/24 21:55:27  gtkwave
 * update for timestart line in savefiles
 *
 * Revision 1.35  2008/07/18 17:27:01  gtkwave
 * adding hierpack code
 *
 * Revision 1.34  2008/07/01 18:51:07  gtkwave
 * compiler warning fixes for amd64
 *
 * Revision 1.33  2008/06/11 08:01:51  gtkwave
 * gcc 4.3.x compiler warning fixes
 *
 * Revision 1.32  2008/04/03 16:56:10  gtkwave
 * more time_trunc() removals
 *
 * Revision 1.31  2008/03/25 19:21:25  gtkwave
 * updated mouseover help to indicate what the flags mean
 *
 * Revision 1.30  2008/03/25 03:22:10  gtkwave
 * expanded zero fill to include also a one fill (for pre-inverted nets)
 *
 * Revision 1.29  2008/03/24 19:34:00  gtkwave
 * added zero range fill feature
 *
 * Revision 1.28  2008/01/25 04:33:48  gtkwave
 * gtk1 fix
 *
 * Revision 1.27  2008/01/25 04:10:14  gtkwave
 * added new resizing options to menu
 *
 * Revision 1.26  2008/01/24 03:17:57  gtkwave
 * added cut and paste to popup menu
 *
 * Revision 1.25  2008/01/23 16:37:56  gtkwave
 * created separate smaller popup instead of using normal itemfactory list
 *
 * Revision 1.24  2008/01/23 11:07:19  gtkwave
 * integration of preliminary signalwindow popup menu code
 *
 * Revision 1.23  2008/01/23 04:49:31  gtkwave
 * more tweaking of interpolated+step mode (use snap dots)
 *
 * Revision 1.22  2008/01/23 02:05:43  gtkwave
 * added interpolated + step mode
 *
 * Revision 1.21  2008/01/20 08:56:35  gtkwave
 * added dirty_kick to MaxSignalLength
 *
 * Revision 1.20  2008/01/12 21:36:44  gtkwave
 * added black and white vs color rendering menu options
 *
 * Revision 1.19  2008/01/09 19:20:52  gtkwave
 * more updating to globals management (expose events cause wrong swap)
 *
 * Revision 1.18  2008/01/09 08:07:03  gtkwave
 * removal of explicit menu string names when unnecessary
 *
 * Revision 1.17  2008/01/08 04:01:12  gtkwave
 * more accelerator key ergonomic updates
 *
 * Revision 1.16  2008/01/04 04:03:14  gtkwave
 * disable dnd for 1.3.
 *
 * Revision 1.15  2007/09/23 18:33:54  gtkwave
 * warnings cleanups from sun compile
 *
 * Revision 1.14  2007/09/17 23:02:54  gtkwave
 * allow scripting with tabs
 *
 * Revision 1.13  2007/09/17 19:26:46  gtkwave
 * added dead context sweep code (deferred cleanup of multi-tab destroy)
 *
 * Revision 1.12  2007/09/12 17:32:04  gtkwave
 * cache globals on tab destroy (moved earlier)
 *
 * Revision 1.11  2007/09/11 11:43:01  gtkwave
 * freeze-out tabs on partial vcd due to context swapping conflicts
 *
 * Revision 1.10  2007/09/11 04:13:25  gtkwave
 * loader hardening for tabbed loads
 *
 * Revision 1.9  2007/09/11 02:12:50  gtkwave
 * context locking in busy spinloops (gtk_main_iteration() calls)
 *
 * Revision 1.8  2007/09/10 19:46:36  gtkwave
 * datatype warning fix
 *
 * Revision 1.7  2007/09/10 19:43:20  gtkwave
 * gtk1.2 compile fixes
 *
 * Revision 1.6  2007/09/09 20:10:30  gtkwave
 * preliminary support for tabbed viewing of multiple waveforms
 *
 * Revision 1.5  2007/08/29 23:38:47  gtkwave
 * 3.1.0 RC2 minor compatibility/bugfixes
 *
 * Revision 1.4  2007/08/26 21:35:43  gtkwave
 * integrated global context management from SystemOfCode2007 branch
 *
 * Revision 1.1.1.1.2.34  2007/08/25 19:43:45  gtkwave
 * header cleanups
 *
 * Revision 1.1.1.1.2.33  2007/08/23 23:51:50  gtkwave
 * moved reload function to globals.c
 *
 * Revision 1.1.1.1.2.32  2007/08/23 23:40:11  gtkwave
 * merged in twinwave support
 *
 * Revision 1.1.1.1.2.31  2007/08/23 23:28:48  gtkwave
 * reload fail handling and retries
 *
 * Revision 1.1.1.1.2.30  2007/08/23 03:16:03  gtkwave
 * NO_FILE now set on stdin sourced VCDs
 *
 * Revision 1.1.1.1.2.29  2007/08/23 03:04:45  gtkwave
 * merge status.c widgets across ctx
 *
 * Revision 1.1.1.1.2.28  2007/08/23 02:47:32  gtkwave
 * updating of reload debug messages
 *
 * Revision 1.1.1.1.2.27  2007/08/23 02:42:51  gtkwave
 * convert c++ style comments to c to aid with compiler compatibility
 *
 * Revision 1.1.1.1.2.26  2007/08/23 02:19:49  gtkwave
 * merge GLOBALS state from old hier_search widget into new one
 *
 * Revision 1.1.1.1.2.25  2007/08/22 22:11:37  gtkwave
 * made search re-entrant, additional state for lxt2/vzt/ae2 loaders
 *
 * Revision 1.1.1.1.2.24  2007/08/22 03:02:42  gtkwave
 * from..to entry widget state merge
 *
 * Revision 1.1.1.1.2.23  2007/08/22 02:17:13  gtkwave
 * gtk1 treebox fixes for re-entrancy
 *
 * Revision 1.1.1.1.2.22  2007/08/22 02:06:39  gtkwave
 * merge in treebox() similar to treeboxframe()
 *
 * Revision 1.1.1.1.2.21  2007/08/21 23:49:27  gtkwave
 * set_size_request doesn't allow window shrinkage so commented out for now
 *
 * Revision 1.1.1.1.2.20  2007/08/21 23:29:17  gtkwave
 * merge in tree select state from old ctx
 *
 * Revision 1.1.1.1.2.19  2007/08/21 22:51:35  gtkwave
 * add tree hadj state merge
 *
 * Revision 1.1.1.1.2.18  2007/08/21 22:35:39  gtkwave
 * prelim tree state merge
 *
 * Revision 1.1.1.1.2.17  2007/08/19 23:13:53  kermin
 * -o flag will now target the original file (in theory reloaded), compress it to lxt2, and then reload the new compressed file.
 *
 * Revision 1.1.1.1.2.16  2007/08/18 22:14:55  gtkwave
 * missing itemfactory pointer caused crash on check/uncheck in menus
 *
 * Revision 1.1.1.1.2.15  2007/08/18 21:56:15  gtkwave
 * remove visual noise on resize/pos on reload as some windowmanagers handle
 * this as a hint rather than an absolute (i.e., set "ignore" rc's to true)
 *
 * Revision 1.1.1.1.2.14  2007/08/18 21:51:57  gtkwave
 * widget destroys and teardown of file formats which use external loaders
 * and are outside of malloc_2/free_2 control
 *
 * Revision 1.1.1.1.2.13  2007/08/17 03:11:29  kermin
 * Correct lengths on reloaded files
 *
 * Revision 1.1.1.1.2.12  2007/08/16 03:29:07  kermin
 * Reload the SST tree
 *
 * Revision 1.1.1.1.2.11  2007/08/16 00:26:17  gtkwave
 * removes drawable != NULL warning on reload
 *
 * Revision 1.1.1.1.2.10  2007/08/15 23:33:52  gtkwave
 * added in rc.c context copy in reload
 *
 * Revision 1.1.1.1.2.9  2007/08/15 04:08:34  kermin
 * fixed from/to box issue
 *
 * Revision 1.1.1.1.2.8  2007/08/15 03:26:01  kermin
 * Reload button does not cause a fault, however, state is still somehow incorrect.
 *
 * Revision 1.1.1.1.2.7  2007/08/07 05:11:18  gtkwave
 * update strdup to strdup_2()
 *
 * Revision 1.1.1.1.2.6  2007/08/07 03:18:55  kermin
 * Changed to pointer based GLOBAL structure and added initialization function
 *
 * Revision 1.1.1.1.2.4  2007/08/05 02:27:21  kermin
 * Semi working global struct
 *
 * Revision 1.1.1.1.2.3  2007/07/31 03:18:01  kermin
 * Merge Complete - I hope
 *
 * Revision 1.1.1.1.2.2  2007/07/28 19:50:40  kermin
 * Merged in the main line
 *
 * Revision 1.3  2007/07/23 23:13:08  gtkwave
 * adds for color tags in filtered trace data
 *
 * Revision 1.1.1.1  2007/05/30 04:27:40  gtkwave
 * Imported sources
 *
 * Revision 1.4  2007/05/28 00:55:06  gtkwave
 * added support for arrays as a first class dumpfile datatype
 *
 * Revision 1.3  2007/04/21 21:02:02  gtkwave
 * changed vertex to vermin to avoid name clash
 *
 * Revision 1.2  2007/04/20 02:08:13  gtkwave
 * initial release
 *
 */
