#include"globals.h"/* 
 * Copyright (c) Tony Bybell 1999-2006
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "currenttime.h"
#include "print.h"
#include "menu.h"
#include <errno.h>








/*
 * button/menu/entry activations..
 */
static void render_clicked(GtkWidget *widget, gpointer which)
{
int i;
char *which_char;

for(i=0;i<2;i++) GLOBALS.target_mutex_renderopt_c_1[i]=0;
which_char=(char *)which;
*which_char=1;                  /* mark our choice */

DEBUG(printf("picked: %s\n", render_targets[which_char-target_mutex]));
}

static void pagesize_clicked(GtkWidget *widget, gpointer which)
{
int i;
char *which_char;

for(i=0;i<5;i++) GLOBALS.page_mutex_renderopt_c_1[i]=0;
which_char=(char *)which;
*which_char=1;                  /* mark our choice */

GLOBALS.page_size_type_renderopt_c_1=which_char-GLOBALS.page_mutex_renderopt_c_1;

DEBUG(printf("picked: %s\n", page_size[which_char-page_mutex]));
}

static void rendertype_clicked(GtkWidget *widget, gpointer which)
{
int i;
char *which_char;

for(i=0;i<2;i++) GLOBALS.render_mutex_renderopt_c_1[i]=0;
which_char=(char *)which;
*which_char=1;                  /* mark our choice */

DEBUG(printf("picked: %s\n", render_type[which_char-render_mutex]));
}


static void
ps_print_cleanup(GtkWidget *widget, gpointer data)
{
FILE *wave;

if(GLOBALS.filesel_ok)
        {
        DEBUG(printf("PS Print Fini: %s\n", *fileselbox_text));
                
        if(!(wave=fopen(*GLOBALS.fileselbox_text,"wb")))
                {
                fprintf(stderr, "Error opening PS output file '%s' for writing.\n",*GLOBALS.fileselbox_text);
                perror("Why");
                errno=0;
                }
                else
                {
                print_ps_image(wave,GLOBALS.px_renderopt_c_1[GLOBALS.page_size_type_renderopt_c_1],GLOBALS.py_renderopt_c_1[GLOBALS.page_size_type_renderopt_c_1]);
                fclose(wave);
                }
        }  
}

static void
mif_print_cleanup(GtkWidget *widget, gpointer data)
{
FILE *wave;

if(GLOBALS.filesel_ok)
        {
        DEBUG(printf("MIF Print Fini: %s\n", *fileselbox_text));
                
        if(!(wave=fopen(*GLOBALS.fileselbox_text,"wb")))
                {
                fprintf(stderr, "Error opening MIF output file '%s' for writing.\n",*GLOBALS.fileselbox_text);
                perror("Why");
                errno=0;
                }
                else
                {
		print_mif_image(wave,GLOBALS.px_renderopt_c_1[GLOBALS.page_size_type_renderopt_c_1],GLOBALS.py_renderopt_c_1[GLOBALS.page_size_type_renderopt_c_1]);
                fclose(wave);
                }
        }  
}


static void ok_callback(void)
{
GLOBALS.ps_fullpage=GLOBALS.render_mutex_renderopt_c_1[0];
if(GLOBALS.target_mutex_renderopt_c_1[0])
	{
	fileselbox("Print To PS File",&GLOBALS.filesel_print_ps_renderopt_c_1,GTK_SIGNAL_FUNC(ps_print_cleanup), GTK_SIGNAL_FUNC(NULL), "*.ps", 1);
	}
	else
	{
	fileselbox("Print To MIF File (experimental)",&GLOBALS.filesel_print_mif_renderopt_c_1,GTK_SIGNAL_FUNC(mif_print_cleanup), GTK_SIGNAL_FUNC(NULL), "*.fm", 1);
	}
}

static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
  GLOBALS.is_active_renderopt_c_3=0;
  gtk_widget_destroy(GLOBALS.window_renderopt_c_6);
}


void renderbox(char *title)
{
    GtkWidget *menu, *menuitem, *optionmenu;
    GSList *group; 
    GtkWidget *vbox, *hbox, *small_hbox;
    GtkWidget *button1, *button2;
    int i;

    if(GLOBALS.script_handle)
        {
        char *s1 = NULL;
        char *s2 = NULL;
        char *s3 = NULL;

        while((!s1)&&(!feof(GLOBALS.script_handle))) s1 = fgetmalloc_stripspaces(GLOBALS.script_handle);
        while((!s2)&&(!feof(GLOBALS.script_handle))) s2 = fgetmalloc_stripspaces(GLOBALS.script_handle);
        while((!s3)&&(!feof(GLOBALS.script_handle))) s3 = fgetmalloc_stripspaces(GLOBALS.script_handle);

        if(s1 && s2 && s3)
                {
		memset(GLOBALS.target_mutex_renderopt_c_1, 0, 2); GLOBALS.target_mutex_renderopt_c_1[0] = 1; /* PS */
		for(i=0;i<2;i++)
			{
			if(!strcmp(s1, GLOBALS.render_targets_renderopt_c_1[i]))
				{
				fprintf(stderr, "GTKWAVE | Print using '%s'\n",  GLOBALS.render_targets_renderopt_c_1[i]);
				memset(GLOBALS.target_mutex_renderopt_c_1, 0, 2); GLOBALS.target_mutex_renderopt_c_1[i] = 1; break;
				}
			}

		memset(GLOBALS.page_mutex_renderopt_c_1, 0, 5); GLOBALS.page_mutex_renderopt_c_1[0] = 1; /* 8.5 x 11 */
		GLOBALS.page_size_type_renderopt_c_1 = 0;
		for(i=0;i<5;i++)
			{
			if(!strcmp(s2, GLOBALS.page_size_renderopt_c_1[i]))
				{
				fprintf(stderr, "GTKWAVE | Print using '%s'\n",  GLOBALS.page_size_renderopt_c_1[i]);
				memset(GLOBALS.page_mutex_renderopt_c_1, 0, 5); GLOBALS.page_mutex_renderopt_c_1[i] = 1; 
				GLOBALS.page_size_type_renderopt_c_1 = i;
				break;
				}
			}

		memset(GLOBALS.render_mutex_renderopt_c_1, 0, 2); GLOBALS.render_mutex_renderopt_c_1[0] = 1; /* Full */
		for(i=0;i<2;i++)
			{
			if(!strcmp(s3, GLOBALS.render_type_renderopt_c_1[i]))
				{
				fprintf(stderr, "GTKWAVE | Print using '%s'\n",  GLOBALS.render_type_renderopt_c_1[i]);
				memset(GLOBALS.render_mutex_renderopt_c_1, 0, 2); GLOBALS.render_mutex_renderopt_c_1[i] = 1; break;
				}
			}

		free_2(s1); free_2(s2); free_2(s3);
		ok_callback();
                }
                else
                {
		fprintf(stderr, "Missing script entries for renderbox, exiting.\n");
		exit(255);
                }

        return;
        }



    if(GLOBALS.is_active_renderopt_c_3) 
	{
	gdk_window_raise(GLOBALS.window_renderopt_c_6->window);
	return;
	}
    GLOBALS.is_active_renderopt_c_3=1;

    /* create a new window */
    GLOBALS.window_renderopt_c_6 = gtk_window_new(GLOBALS.disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW (GLOBALS.window_renderopt_c_6), title);
    gtk_widget_set_usize( GTK_WIDGET (GLOBALS.window_renderopt_c_6), 420, -1); 
    gtk_signal_connect(GTK_OBJECT (GLOBALS.window_renderopt_c_6), "delete_event",(GtkSignalFunc) destroy_callback, NULL);
    gtk_window_set_policy(GTK_WINDOW(GLOBALS.window_renderopt_c_6), FALSE, FALSE, FALSE);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (GLOBALS.window_renderopt_c_6), vbox);
    gtk_widget_show (vbox);

    small_hbox = gtk_hbox_new (TRUE, 0);
    gtk_widget_show (small_hbox);

    menu = gtk_menu_new ();
    group=NULL;

    for(i=0;i<2;i++)
	{
    	menuitem = gtk_radio_menu_item_new_with_label (group, GLOBALS.render_targets_renderopt_c_1[i]);
    	group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    	gtk_menu_append (GTK_MENU (menu), menuitem);
    	gtk_widget_show (menuitem);
        gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                                 GTK_SIGNAL_FUNC(render_clicked),
                                 &GLOBALS.target_mutex_renderopt_c_1[i]);
	GLOBALS.target_mutex_renderopt_c_1[i]=0;
	}

	GLOBALS.target_mutex_renderopt_c_1[0]=1;	/* "ps" */

	optionmenu = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
	gtk_box_pack_start (GTK_BOX (small_hbox), optionmenu, TRUE, FALSE, 0);
	gtk_widget_show (optionmenu);

    menu = gtk_menu_new ();
    group=NULL;

    for(i=0;i<5;i++)
	{
    	menuitem = gtk_radio_menu_item_new_with_label (group, GLOBALS.page_size_renderopt_c_1[i]);
    	group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    	gtk_menu_append (GTK_MENU (menu), menuitem);
    	gtk_widget_show (menuitem);
        gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                                 GTK_SIGNAL_FUNC(pagesize_clicked),
                                 &GLOBALS.page_mutex_renderopt_c_1[i]);
	GLOBALS.page_mutex_renderopt_c_1[i]=0;
	}

	GLOBALS.page_mutex_renderopt_c_1[0]=1;	/* "letter" */

	optionmenu = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
	gtk_box_pack_start (GTK_BOX (small_hbox), optionmenu, TRUE, FALSE, 0);
	gtk_widget_show (optionmenu);


	gtk_box_pack_start (GTK_BOX (vbox), small_hbox, FALSE, FALSE, 0);

    menu = gtk_menu_new ();
    group=NULL;

    for(i=0;i<2;i++)
	{
    	menuitem = gtk_radio_menu_item_new_with_label (group, GLOBALS.render_type_renderopt_c_1[i]);
    	group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    	gtk_menu_append (GTK_MENU (menu), menuitem);
    	gtk_widget_show (menuitem);
        gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                                 GTK_SIGNAL_FUNC(rendertype_clicked),
                                 &GLOBALS.render_mutex_renderopt_c_1[i]);
	GLOBALS.render_mutex_renderopt_c_1[i]=0;
	}

	GLOBALS.render_mutex_renderopt_c_1[0]=1;	/* "full" */

	optionmenu = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
	gtk_box_pack_start (GTK_BOX (small_hbox), optionmenu, TRUE, FALSE, 0);
	gtk_widget_show (optionmenu);


    hbox = gtk_hbox_new (TRUE, 0);
    gtk_widget_show (hbox);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    button1 = gtk_button_new_with_label ("Select Output File");
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

    button2 = gtk_button_new_with_label ("Exit");
    gtk_widget_set_usize(button2, 100, -1);
    gtk_signal_connect(GTK_OBJECT (button2), "clicked",
			       GTK_SIGNAL_FUNC(destroy_callback),
			       NULL);
    GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
    gtk_widget_show (button2);
    gtk_container_add (GTK_CONTAINER (hbox), button2);

    gtk_widget_show(GLOBALS.window_renderopt_c_6);
}

/*
 * $Id$
 * $Log$
 * Revision 1.1.1.1.2.3  2007/07/31 03:18:01  kermin
 * Merge Complete - I hope
 *
 * Revision 1.1.1.1.2.2  2007/07/28 19:50:40  kermin
 * Merged in the main line
 *
 * Revision 1.1.1.1  2007/05/30 04:27:26  gtkwave
 * Imported sources
 *
 * Revision 1.2  2007/04/20 02:08:17  gtkwave
 * initial release
 *
 */

