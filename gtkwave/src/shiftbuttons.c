/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "currenttime.h"
#include "pixmaps.h"
#include "debug.h"

void
service_left_shift(GtkWidget *text, gpointer data)
{
GtkAdjustment *hadj;
gfloat inc;
TimeType ntinc;

if(helpbox_is_active)
        {
        help_text_bold("\n\nShift Left");
        help_text(
                " scrolls the display window left one tick worth of data."
                "  The net action is that the data scrolls right a tick."
#ifdef WAVE_USE_GTK2
                " Ctrl + Scrollwheel Up also performs this function."
#endif
        );
        return;
        }


if(nspx>1.0) inc=nspx; else inc=1.0;

hadj=GTK_ADJUSTMENT(wave_hslider);
if((hadj->value-inc)>tims.first) hadj->value=hadj->value-inc;
	else hadj->value=tims.first;

ntinc=(TimeType)inc;
if((tims.start-ntinc)>tims.first) tims.timecache=tims.start-ntinc;
	else tims.timecache=tims.first;

time_update();

DEBUG(printf("Left Shift\n"));
}

void
service_right_shift(GtkWidget *text, gpointer data)
{
GtkAdjustment *hadj;
gfloat inc;
TimeType ntinc, pageinc;

if(helpbox_is_active)
        {
        help_text_bold("\n\nShift Right");
        help_text(
                " scrolls the display window right one tick worth of data."
                "  The net action is that the data scrolls left a tick."
#ifdef WAVE_USE_GTK2
                " Ctrl + Scrollwheel Down also performs this function."
#endif
        );
        return;
        }

if(nspx>1.0) inc=nspx; else inc=1.0;
ntinc=(TimeType)inc;

hadj=GTK_ADJUSTMENT(wave_hslider);
if((hadj->value+inc)<tims.last) hadj->value=hadj->value+inc;
	else hadj->value=tims.last-inc;

pageinc=(TimeType)(((gdouble)wavewidth)*nspx);

if((tims.start+ntinc)<(tims.last-pageinc+1)) tims.timecache=tims.start+ntinc;
	else 
	{
	tims.timecache=tims.last-pageinc+1;
	if(tims.timecache<tims.first) tims.timecache=tims.first;
	}

time_update();

DEBUG(printf("Right Shift\n"));
}

/* Create shift buttons */
GtkWidget *
create_shift_buttons (void)
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

pixmapwid1=gtk_pixmap_new(larrow_pixmap, larrow_mask);
gtk_widget_show(pixmapwid1);
pixmapwid2=gtk_pixmap_new(rarrow_pixmap, rarrow_mask);
gtk_widget_show(pixmapwid2);
   
/* Create a table to hold the text widget and scrollbars */
table = gtk_table_new (1, 1, FALSE);

main_vbox = gtk_vbox_new (FALSE, 1);
gtk_container_border_width (GTK_CONTAINER (main_vbox), 1);
gtk_container_add (GTK_CONTAINER (table), main_vbox);

frame = gtk_frame_new ("Shift ");
gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

gtk_widget_show (frame);
gtk_widget_show (main_vbox);

table2 = gtk_table_new (2, 1, FALSE);

b1 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b1), pixmapwid1);
gtk_table_attach (GTK_TABLE (table2), b1, 0, 1, 0, 1,
			GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b1), "clicked",
              		GTK_SIGNAL_FUNC(service_left_shift), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b1, "Shift Left 1 Tick", NULL);
gtk_widget_show(b1);

b2 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b2), pixmapwid2);
gtk_table_attach (GTK_TABLE (table2), b2, 0, 1, 1, 2,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b2), "clicked",
               		GTK_SIGNAL_FUNC(service_right_shift), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b2, "Shift Right 1 Tick", NULL);
gtk_widget_show(b2);

gtk_container_add (GTK_CONTAINER (frame), table2);
gtk_widget_show(table2);
return(table);
}
   
/*
 * $Id$
 * $Log$
 */

