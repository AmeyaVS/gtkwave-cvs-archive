/* 
 * Copyright (c) Tony Bybell 1999-2005.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include "currenttime.h"
#include "pixmaps.h"
#include "debug.h"


void
service_left_page(GtkWidget *text, gpointer data)
{
TimeType ntinc, ntfrac;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nPage Left");
        help_text(
                " scrolls the display window left one page worth of data."
		"  The net action is that the data scrolls right a page."
#ifdef WAVE_USE_GTK2
                " Scrollwheel Up also performs this function."
#endif
        );
        return;
        }

ntinc=(TimeType)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);	/* really don't need this var but the speed of ui code is human dependent.. */
ntfrac=ntinc*GLOBALS->page_divisor;
if((ntfrac<1)||(ntinc<1)) ntfrac=ntinc=1;

if((GLOBALS->tims.start-ntfrac)>GLOBALS->tims.first) GLOBALS->tims.timecache=GLOBALS->tims.start-ntfrac;
	else GLOBALS->tims.timecache=GLOBALS->tims.first;

GTK_ADJUSTMENT(GLOBALS->wave_hslider)->value=GLOBALS->tims.timecache;
time_update();

DEBUG(printf("Left Page\n"));
}

void
service_right_page(GtkWidget *text, gpointer data)
{
TimeType ntinc, ntfrac;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nPage Right");
        help_text(
		" scrolls the display window right one page worth of data."
		"  The net action is that the data scrolls left a page."
#ifdef WAVE_USE_GTK2
                " Scrollwheel Down also performs this function."
#endif
        );
        return;
        }

ntinc=(TimeType)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
ntfrac=ntinc*GLOBALS->page_divisor;
if((ntfrac<1)||(ntinc<1)) ntfrac=ntinc=1;

if((GLOBALS->tims.start+ntfrac)<(GLOBALS->tims.last-ntinc+1)) 
	{
	GLOBALS->tims.timecache=GLOBALS->tims.start+ntfrac;
	}
        else 
	{
	GLOBALS->tims.timecache=GLOBALS->tims.last-ntinc+1;
	if(GLOBALS->tims.timecache<GLOBALS->tims.first) GLOBALS->tims.timecache=GLOBALS->tims.first;
	}

GTK_ADJUSTMENT(GLOBALS->wave_hslider)->value=GLOBALS->tims.timecache;
time_update();

DEBUG(printf("Right Page\n"));
}


/* Create page buttons */
GtkWidget *
create_page_buttons (void)
{
GtkWidget *table;
GtkWidget *table2;
GtkWidget *frame;
GtkWidget *main_vbox;
GtkWidget *b1;
GtkWidget *b2;
GtkWidget *pixmapwid1, *pixmapwid2;

GtkTooltips *tooltips;

tooltips=gtk_tooltips_new_2();
gtk_tooltips_set_delay_2(tooltips,1500);

pixmapwid1=gtk_pixmap_new(GLOBALS->prev_page_pixmap, GLOBALS->prev_page_mask);
gtk_widget_show(pixmapwid1);
pixmapwid2=gtk_pixmap_new(GLOBALS->next_page_pixmap, GLOBALS->next_page_mask);
gtk_widget_show(pixmapwid2);
   
/* Create a table to hold the text widget and scrollbars */
table = gtk_table_new (1, 1, FALSE);

main_vbox = gtk_vbox_new (FALSE, 1);
gtk_container_border_width (GTK_CONTAINER (main_vbox), 1);
gtk_container_add (GTK_CONTAINER (table), main_vbox);

frame = gtk_frame_new ("Page "); 
gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

gtk_widget_show (frame);
gtk_widget_show (main_vbox);

table2 = gtk_table_new (2, 1, FALSE);

b1 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b1), pixmapwid1);
gtk_table_attach (GTK_TABLE (table2), b1, 0, 1, 0, 1,
			GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b1), "clicked", GTK_SIGNAL_FUNC(service_left_page), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b1, "Scroll Window Left One Page", NULL);
gtk_widget_show(b1);

b2 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b2), pixmapwid2);
gtk_table_attach (GTK_TABLE (table2), b2, 0, 1, 1, 2,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b2), "clicked", GTK_SIGNAL_FUNC(service_right_page), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b2, "Scroll Window Right One Page", NULL);
gtk_widget_show(b2);

gtk_container_add (GTK_CONTAINER (frame), table2);
gtk_widget_show(table2);
return(table);
}
   
/*
 * $Id$
 * $Log$
 * Revision 1.3  2007/09/12 17:26:45  gtkwave
 * experimental ctx_swap_watchdog added...still tracking down mouse thrash crashes
 *
 * Revision 1.2  2007/08/26 21:35:43  gtkwave
 * integrated global context management from SystemOfCode2007 branch
 *
 * Revision 1.1.1.1.2.3  2007/08/07 03:18:55  kermin
 * Changed to pointer based GLOBAL structure and added initialization function
 *
 * Revision 1.1.1.1.2.2  2007/08/06 03:50:48  gtkwave
 * globals support for ae2, gtk1, cygwin, mingw.  also cleaned up some machine
 * generated structs, etc.
 *
 * Revision 1.1.1.1.2.1  2007/08/05 02:27:21  kermin
 * Semi working global struct
 *
 * Revision 1.1.1.1  2007/05/30 04:27:22  gtkwave
 * Imported sources
 *
 * Revision 1.2  2007/04/20 02:08:13  gtkwave
 * initial release
 *
 */

