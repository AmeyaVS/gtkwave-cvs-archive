/* 
 * Copyright (c) Tony Bybell 1999-2005.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "gtk12compat.h"
#include <gtk/gtk.h>
#include "menu.h"
#include "debug.h"
#include "pixmaps.h"

static GtkWidget *window = NULL;
static void (*cleanup)(GtkWidget *, gpointer);

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
  DEBUG(printf("OK\n"));
  gtk_grab_remove(window);
  gtk_widget_destroy(window);
  window = NULL;
  if(cleanup)cleanup(NULL,(gpointer)1);
}

static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
  DEBUG(printf("Cancel\n"));
  gtk_grab_remove(window);
  gtk_widget_destroy(window);
  window = NULL;
  if(cleanup)cleanup(NULL,NULL);
}

void simplereqbox(char *title, int width, char *default_text,
	char *oktext, char *canceltext, GtkSignalFunc func, int is_alert)
{
    GtkWidget *vbox, *hbox;
    GtkWidget *button1, *button2;
    GtkWidget *label, *separator;
    GtkWidget *pixmapwid1;

    if(window) return; /* only should happen with GtkPlug */

    cleanup=WAVE_GTK_SFUNCAST(func);

    if(script_handle)
	{
	if(cleanup)cleanup(NULL,(gpointer)1);
	return;
	}

    /* create a new modal window */
    window = gtk_window_new(disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    gtk_grab_add(window);
    gtk_widget_set_usize( GTK_WIDGET (window), width, 55 + 52);
    gtk_window_set_title(GTK_WINDOW (window), title);
    gtk_signal_connect(GTK_OBJECT (window), "delete_event",
                       (GtkSignalFunc) destroy_callback, NULL);
    gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_widget_show (vbox);

    label=gtk_label_new(default_text);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    if(is_alert)
	{
	pixmapwid1=gtk_pixmap_new(wave_alert_pixmap, wave_alert_mask);
	}
	else
	{
	pixmapwid1=gtk_pixmap_new(wave_info_pixmap, wave_info_mask);
	}
    gtk_widget_show(pixmapwid1);
    gtk_container_add (GTK_CONTAINER (vbox), pixmapwid1);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 0);
    gtk_widget_show (separator);

    hbox = gtk_hbox_new (FALSE, 1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label (oktext);
    gtk_widget_set_usize(button1, 100, -1);
    gtk_signal_connect(GTK_OBJECT (button1), "clicked",
			       GTK_SIGNAL_FUNC(ok_callback),
			       NULL);
    gtk_widget_show (button1);
    gtk_container_add (GTK_CONTAINER (hbox), button1);
    GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
    gtk_signal_connect_object (GTK_OBJECT (button1),
                                "realize",
                             (GtkSignalFunc) gtk_widget_grab_default,
                             GTK_OBJECT (button1));

    if(canceltext)
	{
    	button2 = gtk_button_new_with_label (canceltext);
    	gtk_widget_set_usize(button2, 100, -1);
    	gtk_signal_connect(GTK_OBJECT (button2), "clicked",
			       GTK_SIGNAL_FUNC(destroy_callback),
			       NULL);
    	GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
    	gtk_widget_show (button2);
    	gtk_container_add (GTK_CONTAINER (hbox), button2);
	}

    gtk_widget_show(window);
}

/*
 * $Id$
 * $Log$
 */

