/* 
 * Copyright (c) Tony Bybell 1999-2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include <gtk/gtk.h>
#include "gtk12compat.h"
#include "analyzer.h"
#include "tree.h"
#include "symbol.h"
#include "vcd.h"
#include "lx2.h"
#include "debug.h"

void dnd_setup(GtkWidget *src, GtkWidget *w, int enable_receive)
{
	GtkWidget *win = w;
	GtkTargetEntry target_entry[3];

	/* Realize the clist widget and make sure it has a window,
	 * this will be for DND setup.
	 */
        if(!GTK_WIDGET_NO_WINDOW(w))
	{
		/* DND: Set up the clist as a potential DND destination.
		 * First we set up target_entry which is a sequence of of
		 * structure which specify the kinds (which we define) of
		 * drops accepted on this widget.
		 */

		/* Set up the list of data format types that our DND
		 * callbacks will accept.
		 */
		target_entry[0].target = WAVE_DRAG_TAR_NAME_0;
		target_entry[0].flags = 0;
		target_entry[0].info = WAVE_DRAG_TAR_INFO_0;
                target_entry[1].target = WAVE_DRAG_TAR_NAME_1;
                target_entry[1].flags = 0;
                target_entry[1].info = WAVE_DRAG_TAR_INFO_1;
                target_entry[2].target = WAVE_DRAG_TAR_NAME_2;
                target_entry[2].flags = 0;
                target_entry[2].info = WAVE_DRAG_TAR_INFO_2;

		/* Set the drag destination for this widget, using the
		 * above target entry types, accept move's and coppies'.
		 */

		/* required gtk1 hack */
		gtk_object_set_data(GTK_OBJECT(w), "gtk-drag-dest", NULL);

		gtk_drag_dest_set(
			w,
			GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT |
			GTK_DEST_DEFAULT_DROP,
			target_entry,
			sizeof(target_entry) / sizeof(GtkTargetEntry),
			GDK_ACTION_MOVE | GDK_ACTION_COPY
		);

		/* Set the drag source for this widget, allowing the user
		 * to drag items off of this clist.
		 */
		if(src)
		gtk_drag_source_set(
			src,
			GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                        target_entry,
                        sizeof(target_entry) / sizeof(GtkTargetEntry),
			GDK_ACTION_MOVE | GDK_ACTION_COPY
		);
	}
}

void treeview_select_all_callback(void)
{
/* nothing, no treeview for gtk1 implemented yet */
}

void treeview_unselect_all_callback(void)
{
/* nothing, no treeview for gtk1 implemented yet */
}


static void select_row_callback(GtkWidget *widget, gint row, gint column,
        GdkEventButton *event, gpointer data)
{
struct tree *t;

t=(struct tree *)gtk_clist_get_row_data(GTK_CLIST(GLOBALS->ctree_main), row);
DEBUG(printf("TS: %08x %s\n",t,t->name));
GLOBALS->selectedtree_treesearch_gtk1_c=t;
}

static void unselect_row_callback(GtkWidget *widget, gint row, gint column,
        GdkEventButton *event, gpointer data)
{
struct tree *t;

t=(struct tree *)gtk_clist_get_row_data(GTK_CLIST(GLOBALS->ctree_main), row);
DEBUG(printf("TU: %08x %s\n",t,t->name));
GLOBALS->selectedtree_treesearch_gtk1_c=NULL;
}


int treebox_is_active(void)
{
return(GLOBALS->is_active_treesearch_gtk1_c);
}

static void enter_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
  G_CONST_RETURN gchar *entry_text;
  int len;
  entry_text = gtk_entry_get_text(GTK_ENTRY(GLOBALS->entry_a_treesearch_gtk1_c));
  DEBUG(printf("Entry contents: %s\n", entry_text));
  if(!(len=strlen(entry_text))) GLOBALS->entrybox_text_local_treesearch_gtk1_c=NULL;
	else strcpy((GLOBALS->entrybox_text_local_treesearch_gtk1_c=(char *)malloc_2(len+1)),entry_text);

  gtk_grab_remove(GLOBALS->window1_treesearch_gtk1_c);
  gtk_widget_destroy(GLOBALS->window1_treesearch_gtk1_c);
  GLOBALS->window1_treesearch_gtk1_c = NULL;

  GLOBALS->cleanup_e_treesearch_gtk1_c();
}

static void destroy_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
  DEBUG(printf("Entry Cancel\n"));
  GLOBALS->entrybox_text_local_treesearch_gtk1_c=NULL;
  gtk_grab_remove(GLOBALS->window1_treesearch_gtk1_c);
  gtk_widget_destroy(GLOBALS->window1_treesearch_gtk1_c);
  GLOBALS->window1_treesearch_gtk1_c = NULL;
}

static void entrybox_local(char *title, int width, char *default_text, int maxch, GtkSignalFunc func)
{
    GtkWidget *vbox, *hbox;
    GtkWidget *button1, *button2;

    GLOBALS->cleanup_e_treesearch_gtk1_c=func;

    /* create a new modal window */
    GLOBALS->window1_treesearch_gtk1_c = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window1_treesearch_gtk1_c, ((char *)&GLOBALS->window1_treesearch_gtk1_c) - ((char *)GLOBALS));

    gtk_widget_set_usize( GTK_WIDGET (GLOBALS->window1_treesearch_gtk1_c), width, 60);
    gtk_window_set_title(GTK_WINDOW (GLOBALS->window1_treesearch_gtk1_c), title);
    gtkwave_signal_connect(GTK_OBJECT (GLOBALS->window1_treesearch_gtk1_c), "delete_event", (GtkSignalFunc) destroy_callback_e, NULL);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (GLOBALS->window1_treesearch_gtk1_c), vbox);
    gtk_widget_show (vbox);

    GLOBALS->entry_a_treesearch_gtk1_c = gtk_entry_new_with_max_length (maxch);
    gtkwave_signal_connect(GTK_OBJECT(GLOBALS->entry_a_treesearch_gtk1_c), "activate", GTK_SIGNAL_FUNC(enter_callback_e), GLOBALS->entry_a_treesearch_gtk1_c);
    gtk_entry_set_text (GTK_ENTRY (GLOBALS->entry_a_treesearch_gtk1_c), default_text);
    gtk_entry_select_region (GTK_ENTRY (GLOBALS->entry_a_treesearch_gtk1_c),
			     0, GTK_ENTRY(GLOBALS->entry_a_treesearch_gtk1_c)->text_length);
    gtk_box_pack_start (GTK_BOX (vbox), GLOBALS->entry_a_treesearch_gtk1_c, TRUE, TRUE, 0);
    gtk_widget_show (GLOBALS->entry_a_treesearch_gtk1_c);

    hbox = gtk_hbox_new (FALSE, 1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("OK");
    gtk_widget_set_usize(button1, 100, -1);
    gtkwave_signal_connect(GTK_OBJECT (button1), "clicked", GTK_SIGNAL_FUNC(enter_callback_e), NULL);
    gtk_widget_show (button1);
    gtk_container_add (GTK_CONTAINER (hbox), button1);
    GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
    gtkwave_signal_connect_object (GTK_OBJECT (button1), "realize", (GtkSignalFunc) gtk_widget_grab_default, GTK_OBJECT (button1));

    button2 = gtk_button_new_with_label ("Cancel");
    gtk_widget_set_usize(button2, 100, -1);
    gtkwave_signal_connect(GTK_OBJECT (button2), "clicked", GTK_SIGNAL_FUNC(destroy_callback_e), NULL);
    GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
    gtk_widget_show (button2);
    gtk_container_add (GTK_CONTAINER (hbox), button2);

    gtk_widget_show(GLOBALS->window1_treesearch_gtk1_c);
    gtk_grab_add(GLOBALS->window1_treesearch_gtk1_c);
}

/***************************************************************************/

struct tree *fetchhigh(struct tree *t)
{
while(t->child) t=t->child;
return(t);
}

struct tree *fetchlow(struct tree *t)
{
if(t->child) 
	{
	t=t->child;

	for(;;)
		{
		while(t->next) t=t->next;
		if(t->child) t=t->child; else break;
		}
	}
return(t);
}

static void fetchvex2(struct tree *t, char direction, char level)
{
while(t)
	{
	if(t->child)
		{
		if(t->child->child)
			{
			fetchvex2(t->child, direction, 1);
			}
			else
			{
			add_vector_range(NULL, fetchlow(t)->t_which,
				fetchhigh(t)->t_which, direction);
			}
		}
	if(level) { t=t->next; } else { break; }
	}
}

void fetchvex(struct tree *t, char direction)
{
if(t)
	{
	if(t->child)
		{
		fetchvex2(t, direction, 0);
		}
		else
		{
		add_vector_range(NULL, fetchlow(t)->t_which, 
			fetchhigh(t)->t_which, direction);
		}
	}
}


/* call cleanup() on ok/insert functions */

static void
bundle_cleanup(GtkWidget *widget, gpointer data)
{ 
if(GLOBALS->entrybox_text_local_treesearch_gtk1_c) 
        {
        char *efix;
 
	if(!strlen(GLOBALS->entrybox_text_local_treesearch_gtk1_c))
		{
	        DEBUG(printf("Bundle name is not specified--recursing into hierarchy.\n"));
		fetchvex(GLOBALS->selectedtree_treesearch_gtk1_c, GLOBALS->bundle_direction_treesearch_gtk1_c);
		}
		else
		{
	        efix=GLOBALS->entrybox_text_local_treesearch_gtk1_c;
	        while(*efix)
	                {
	                if(*efix==' ')
	                        {
	                        *efix='_';
	                        }
	                efix++;
	                }
	 
	        DEBUG(printf("Bundle name is: %s\n",GLOBALS->entrybox_text_local_treesearch_gtk1_c));
	        add_vector_range(GLOBALS->entrybox_text_local_treesearch_gtk1_c, 
				fetchlow(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which,
				fetchhigh(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which, 
				GLOBALS->bundle_direction_treesearch_gtk1_c);
		}
        free_2(GLOBALS->entrybox_text_local_treesearch_gtk1_c);
        }
	else
	{
        DEBUG(printf("Bundle name is not specified--recursing into hierarchy.\n"));
	fetchvex(GLOBALS->selectedtree_treesearch_gtk1_c, GLOBALS->bundle_direction_treesearch_gtk1_c);
	}

MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}
 
static void
bundle_callback_generic(void)
{
if(GLOBALS->selectedtree_treesearch_gtk1_c)
	{
	if(!GLOBALS->autoname_bundles)
	        {
	        entrybox_local("Enter Bundle Name",300,"",128,GTK_SIGNAL_FUNC(bundle_cleanup));
	        }
	        else
	        {
	        GLOBALS->entrybox_text_local_treesearch_gtk1_c=NULL;
	        bundle_cleanup(NULL, NULL);
	        }
	}
}

static void
bundle_callback_up(GtkWidget *widget, gpointer data)
{
GLOBALS->bundle_direction_treesearch_gtk1_c=0;
bundle_callback_generic();
}

static void
bundle_callback_down(GtkWidget *widget, gpointer data)
{
GLOBALS->bundle_direction_treesearch_gtk1_c=1;
bundle_callback_generic();
}

static void insert_callback(GtkWidget *widget, GtkWidget *nothing)
{
Traces tcache;
int i;

if(!GLOBALS->selectedtree_treesearch_gtk1_c) return;

memcpy(&tcache,&GLOBALS->traces,sizeof(Traces));
GLOBALS->traces.total=0;
GLOBALS->traces.first=GLOBALS->traces.last=NULL;

set_window_busy(widget);

for(i=fetchlow(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i<=fetchhigh(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i++)
        {
        struct symbol *s;  
        s=GLOBALS->facs[i];
	if(s->vec_root)
		{
		set_s_selected(s->vec_root, GLOBALS->autocoalesce);
		}
        }

/* LX2 */
if(GLOBALS->is_lx2)
        {
        int pre_import = 0;

        for(i=fetchlow(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i<=fetchhigh(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i++)
                {
                struct symbol *s, *t;
                s=GLOBALS->facs[i];
                t=s->vec_root;
                if((t)&&(GLOBALS->autocoalesce))
                        {
                        if(get_s_selected(t))
                                {
                                while(t)
                                        {
                                        if(t->n->mv.mvlfac)
                                                {
                                                lx2_set_fac_process_mask(t->n);
                                                pre_import++;
                                                }
                                        t=t->vec_chain;
                                        }
                                }
                        }
                        else
                        {
                        if(s->n->mv.mvlfac)
                                {
                                lx2_set_fac_process_mask(s->n);
                                pre_import++;
                                }
                        }
                }

        if(pre_import)
                {
                lx2_import_masked();
                }
        }
/* LX2 */

for(i=fetchlow(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i<=fetchhigh(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i++)
        {
	int len;
        struct symbol *s, *t;  
        s=GLOBALS->facs[i];
	t=s->vec_root;
	if((t)&&(GLOBALS->autocoalesce))
		{
		if(get_s_selected(t))
			{
			set_s_selected(t, 0);
			len=0;
			while(t)
				{
				len++;
				t=t->vec_chain;
				}
			if(len) add_vector_chain(s->vec_root, len);
			}
		}
		else
		{
	        AddNodeUnroll(s->n, NULL);  
		}
        }

set_window_idle(widget);

GLOBALS->traces.buffercount=GLOBALS->traces.total;
GLOBALS->traces.buffer=GLOBALS->traces.first;
GLOBALS->traces.bufferlast=GLOBALS->traces.last;
GLOBALS->traces.first=tcache.first;
GLOBALS->traces.last=tcache.last;
GLOBALS->traces.total=tcache.total;

PasteBuffer();

GLOBALS->traces.buffercount=tcache.buffercount;
GLOBALS->traces.buffer=tcache.buffer;
GLOBALS->traces.bufferlast=tcache.bufferlast;

MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}

static void replace_callback(GtkWidget *widget, GtkWidget *nothing)
{
Traces tcache;
int i;
Trptr tfirst=NULL, tlast=NULL;

if(!GLOBALS->selectedtree_treesearch_gtk1_c) return;

memcpy(&tcache,&GLOBALS->traces,sizeof(Traces));
GLOBALS->traces.total=0;
GLOBALS->traces.first=GLOBALS->traces.last=NULL;

set_window_busy(widget);

for(i=fetchlow(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i<=fetchhigh(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i++)
        {
        struct symbol *s;  
        s=GLOBALS->facs[i];
	if(s->vec_root)
		{
		set_s_selected(s->vec_root, GLOBALS->autocoalesce);
		}
        }

/* LX2 */
if(GLOBALS->is_lx2)
        {
        int pre_import = 0;

        for(i=fetchlow(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i<=fetchhigh(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i++)
                {
                struct symbol *s, *t;
                s=GLOBALS->facs[i];
                t=s->vec_root;
                if((t)&&(GLOBALS->autocoalesce))
                        {
                        if(get_s_selected(t))
                                {
                                while(t)
                                        {
                                        if(t->n->mv.mvlfac)
                                                {
                                                lx2_set_fac_process_mask(t->n);
                                                pre_import++;
                                                }
                                        t=t->vec_chain;
                                        }
                                }
                        }
                        else
                        {
                        if(s->n->mv.mvlfac)
                                {
                                lx2_set_fac_process_mask(s->n);
                                pre_import++;
                                }
                        }
                }

        if(pre_import)
                {
                lx2_import_masked();
                }
        }
/* LX2 */

for(i=fetchlow(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i<=fetchhigh(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i++)
        {
	int len;
        struct symbol *s, *t;  
        s=GLOBALS->facs[i];
	t=s->vec_root;
	if((t)&&(GLOBALS->autocoalesce))
		{
		if(get_s_selected(t))
			{
			set_s_selected(t, 0);
			len=0;
			while(t)
				{
				len++;
				t=t->vec_chain;
				}
			if(len) add_vector_chain(s->vec_root, len);
			}
		}
		else
		{
	        AddNodeUnroll(s->n, NULL);  
		}
        }

set_window_idle(widget);

tfirst=GLOBALS->traces.first; tlast=GLOBALS->traces.last;	/* cache for highlighting */

GLOBALS->traces.buffercount=GLOBALS->traces.total;
GLOBALS->traces.buffer=GLOBALS->traces.first;
GLOBALS->traces.bufferlast=GLOBALS->traces.last;
GLOBALS->traces.first=tcache.first;
GLOBALS->traces.last=tcache.last;
GLOBALS->traces.total=tcache.total;

PasteBuffer();

GLOBALS->traces.buffercount=tcache.buffercount;
GLOBALS->traces.buffer=tcache.buffer;
GLOBALS->traces.bufferlast=tcache.bufferlast;

CutBuffer();

while(tfirst)
	{
	tfirst->flags |= TR_HIGHLIGHT;
	if(tfirst==tlast) break;
	tfirst=tfirst->t_next;
	}

MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
int i;

if(!GLOBALS->selectedtree_treesearch_gtk1_c) return;

set_window_busy(widget);

for(i=fetchlow(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i<=fetchhigh(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i++)
        {
        struct symbol *s;  
        s=GLOBALS->facs[i];
	if(s->vec_root)
		{
		set_s_selected(s->vec_root, GLOBALS->autocoalesce);
		}
        }

/* LX2 */
if(GLOBALS->is_lx2)
        {
        int pre_import = 0;

        for(i=fetchlow(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i<=fetchhigh(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i++)
                {
                struct symbol *s, *t;
                s=GLOBALS->facs[i];
                t=s->vec_root;
                if((t)&&(GLOBALS->autocoalesce))
                        {
                        if(get_s_selected(t))
                                {
                                while(t)
                                        {
                                        if(t->n->mv.mvlfac)
                                                {
                                                lx2_set_fac_process_mask(t->n);
                                                pre_import++;
                                                }
                                        t=t->vec_chain;
                                        }
                                }
                        }
                        else
                        {
                        if(s->n->mv.mvlfac)
                                {
                                lx2_set_fac_process_mask(s->n);
                                pre_import++;
                                }
                        }
                }

        if(pre_import)
                {
                lx2_import_masked();
                }
        }
/* LX2 */

for(i=fetchlow(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i<=fetchhigh(GLOBALS->selectedtree_treesearch_gtk1_c)->t_which;i++)
        {
	int len;
        struct symbol *s, *t;  
        s=GLOBALS->facs[i];
	t=s->vec_root;
	if((t)&&(GLOBALS->autocoalesce))
		{
		if(get_s_selected(t))
			{
			set_s_selected(t, 0);
			len=0;
			while(t)
				{
				len++;
				t=t->vec_chain;
				}
			if(len) add_vector_chain(s->vec_root, len);
			}
		}
		else
		{
	        AddNodeUnroll(s->n, NULL);  
		}
        }

set_window_idle(widget);

GLOBALS->traces.scroll_top = GLOBALS->traces.scroll_bottom = GLOBALS->traces.last;
MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}


static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
  GLOBALS->is_active_treesearch_gtk1_c=0;
  gtk_widget_destroy(GLOBALS->window_treesearch_gtk1_c);
  GLOBALS->window_treesearch_gtk1_c = NULL;
}



/*
 * mainline..
 */
void treebox(char *title, GtkSignalFunc func, GtkWidget *old_window)
{
    GtkWidget *scrolled_win;
    GtkWidget *hbox;
    GtkWidget *button1, *button2, *button3, *button3a, *button4, *button5;
    GtkWidget *frame2, *frameh;
    GtkWidget *table;
    GtkTooltips *tooltips;
    GtkCList  *clist;

    if(GLOBALS->is_active_treesearch_gtk1_c) 
	{
	gdk_window_raise(GLOBALS->window_treesearch_gtk1_c->window);
	return;
	}

    GLOBALS->is_active_treesearch_gtk1_c=1;
    GLOBALS->cleanup_treesearch_gtk1_c=func;

    /* create a new modal window */
    GLOBALS->window_treesearch_gtk1_c = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window_treesearch_gtk1_c, ((char *)&GLOBALS->window_treesearch_gtk1_c) - ((char *)GLOBALS));

    gtk_window_set_title(GTK_WINDOW (GLOBALS->window_treesearch_gtk1_c), title);
    gtkwave_signal_connect(GTK_OBJECT (GLOBALS->window_treesearch_gtk1_c), "delete_event", (GtkSignalFunc) destroy_callback, NULL); 

    tooltips=gtk_tooltips_new_2();

    table = gtk_table_new (256, 1, FALSE);
    gtk_widget_show (table);

    frame2 = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frame2), 3);
    gtk_widget_show(frame2);

    gtk_table_attach (GTK_TABLE (table), frame2, 0, 1, 0, 255,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);

    GLOBALS->tree_treesearch_gtk1_c=gtk_ctree_new(1,0);
    GLOBALS->ctree_main=GTK_CTREE(GLOBALS->tree_treesearch_gtk1_c);

    gtk_clist_set_column_auto_resize (GTK_CLIST (GLOBALS->tree_treesearch_gtk1_c), 0, TRUE);
    gtk_widget_show(GLOBALS->tree_treesearch_gtk1_c);

    clist=GTK_CLIST(GLOBALS->tree_treesearch_gtk1_c);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "select_row", GTK_SIGNAL_FUNC(select_row_callback), NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "unselect_row", GTK_SIGNAL_FUNC(unselect_row_callback), NULL);

    gtk_clist_freeze(clist);
    gtk_clist_clear(clist);

    maketree(NULL, GLOBALS->treeroot);

    gtk_clist_thaw(clist);
    GLOBALS->selectedtree_treesearch_gtk1_c=NULL;

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize( GTK_WIDGET (scrolled_win), -1, 300);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_win);
    gtk_container_add (GTK_CONTAINER (scrolled_win), GTK_WIDGET (GLOBALS->tree_treesearch_gtk1_c));
    gtk_container_add (GTK_CONTAINER (frame2), scrolled_win);

    frameh = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frameh), 3);
    gtk_widget_show(frameh);
    gtk_table_attach (GTK_TABLE (table), frameh, 0, 1, 255, 256,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);


    hbox = gtk_hbox_new (FALSE, 1);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("Append");
    gtk_container_border_width (GTK_CONTAINER (button1), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button1), "clicked",  GTK_SIGNAL_FUNC(ok_callback), GTK_OBJECT (GLOBALS->window_treesearch_gtk1_c));
    gtk_widget_show (button1);
    gtk_tooltips_set_tip_2(tooltips, button1, 
		"Add selected signal hierarchy to end of the display on the main window.",NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button1, TRUE, FALSE, 0);

    button2 = gtk_button_new_with_label (" Insert ");
    gtk_container_border_width (GTK_CONTAINER (button2), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button2), "clicked", GTK_SIGNAL_FUNC(insert_callback), GTK_OBJECT (GLOBALS->window_treesearch_gtk1_c));
    gtk_widget_show (button2);
    gtk_tooltips_set_tip_2(tooltips, button2, 
		"Add selected signal hierarchy after last highlighted signal on the main window.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button2, TRUE, FALSE, 0);

    if(GLOBALS->vcd_explicit_zero_subscripts>=0)
	{
    	button3 = gtk_button_new_with_label (" Bundle Up ");
    	gtk_container_border_width (GTK_CONTAINER (button3), 3);
    	gtkwave_signal_connect_object (GTK_OBJECT (button3), "clicked", GTK_SIGNAL_FUNC(bundle_callback_up), GTK_OBJECT (GLOBALS->window_treesearch_gtk1_c));
    	gtk_widget_show (button3);
    	gtk_tooltips_set_tip_2(tooltips, button3, 
		"Bundle selected signal hierarchy into a single bit "
		"vector with the topmost signal as the LSB and the "
		"lowest as the MSB.  Entering a zero length bundle "
		"name will reconstruct the individual vectors "
		"in the hierarchy.  Otherwise, all the bits in "
		"the hierarchy will be coalesced with the supplied "
		"name into a single vector.",NULL);
    	gtk_box_pack_start (GTK_BOX (hbox), button3, TRUE, FALSE, 0);

    	button3a = gtk_button_new_with_label (" Bundle Down ");
    	gtk_container_border_width (GTK_CONTAINER (button3a), 3);
    	gtkwave_signal_connect_object (GTK_OBJECT (button3a), "clicked", GTK_SIGNAL_FUNC(bundle_callback_down), GTK_OBJECT (GLOBALS->window_treesearch_gtk1_c));
    	gtk_widget_show (button3a);
    	gtk_tooltips_set_tip_2(tooltips, button3a, 
		"Bundle selected signal hierarchy into a single bit "
		"vector with the topmost signal as the MSB and the "
		"lowest as the LSB.  Entering a zero length bundle "
		"name will reconstruct the individual vectors "
		"in the hierarchy.  Otherwise, all the bits in "
		"the hierarchy will be coalesced with the supplied "
		"name into a single vector.",NULL);
   	gtk_box_pack_start (GTK_BOX (hbox), button3a, TRUE, FALSE, 0);
	}

    button4 = gtk_button_new_with_label (" Replace ");
    gtk_container_border_width (GTK_CONTAINER (button4), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button4), "clicked", GTK_SIGNAL_FUNC(replace_callback), GTK_OBJECT (GLOBALS->window_treesearch_gtk1_c));
    gtk_widget_show (button4);
    gtk_tooltips_set_tip_2(tooltips, button4, 
		"Replace highlighted signals on the main window with signals selected above.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button4, TRUE, FALSE, 0);

    button5 = gtk_button_new_with_label (" Exit ");
    gtk_container_border_width (GTK_CONTAINER (button5), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button5), "clicked", GTK_SIGNAL_FUNC(destroy_callback), GTK_OBJECT (GLOBALS->window_treesearch_gtk1_c));
    gtk_tooltips_set_tip_2(tooltips, button5, 
		"Do nothing and return to the main window.",NULL);
    gtk_widget_show (button5);
    gtk_box_pack_start (GTK_BOX (hbox), button5, TRUE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (frameh), hbox);
    gtk_container_add (GTK_CONTAINER (GLOBALS->window_treesearch_gtk1_c), table);

    gtk_widget_show(GLOBALS->window_treesearch_gtk1_c);
}

/*
 * $Id$
 * $Log$
 * Revision 1.14  2010/05/27 06:07:25  gtkwave
 * Moved gtk_grab_add() after gtk_widget_show() as newer gtk needs that order.
 *
 * Revision 1.13  2010/03/16 21:01:11  gtkwave
 * remove selected member of struct symbol
 *
 * Revision 1.12  2008/12/18 01:31:30  gtkwave
 * integrated experimental autoscroll code on signal adds
 *
 * Revision 1.11  2008/10/04 15:15:20  gtkwave
 * gtk1 compatibility fixes
 *
 * Revision 1.10  2008/09/16 00:01:27  gtkwave
 * prelim drag and drop from external apps (now disabled)
 *
 * Revision 1.9  2008/05/08 20:10:04  gtkwave
 * 2.4 not required for dnd so requirement relaxed
 *
 * Revision 1.8  2008/02/12 23:35:42  gtkwave
 * preparing for 3.1.5 revision bump
 *
 * Revision 1.7  2008/01/30 21:12:23  gtkwave
 * dnd for gtk1
 *
 * Revision 1.6  2008/01/08 18:59:04  gtkwave
 * gtk1 fixes for dnd
 *
 * Revision 1.5  2008/01/08 04:01:12  gtkwave
 * more accelerator key ergonomic updates
 *
 * Revision 1.4  2007/09/12 17:26:45  gtkwave
 * experimental ctx_swap_watchdog added...still tracking down mouse thrash crashes
 *
 * Revision 1.3  2007/09/10 18:08:49  gtkwave
 * tabs selection can swap dynamically based on external window focus
 *
 * Revision 1.2  2007/08/26 21:35:46  gtkwave
 * integrated global context management from SystemOfCode2007 branch
 *
 * Revision 1.1.1.1.2.5  2007/08/25 19:43:46  gtkwave
 * header cleanups
 *
 * Revision 1.1.1.1.2.4  2007/08/22 02:17:13  gtkwave
 * gtk1 treebox fixes for re-entrancy
 *
 * Revision 1.1.1.1.2.3  2007/08/18 21:51:57  gtkwave
 * widget destroys and teardown of file formats which use external loaders
 * and are outside of malloc_2/free_2 control
 *
 * Revision 1.1.1.1.2.2  2007/08/07 03:18:55  kermin
 * Changed to pointer based GLOBAL structure and added initialization function
 *
 * Revision 1.1.1.1.2.1  2007/08/06 03:50:50  gtkwave
 * globals support for ae2, gtk1, cygwin, mingw.  also cleaned up some machine
 * generated structs, etc.
 *
 * Revision 1.1.1.1  2007/05/30 04:27:58  gtkwave
 * Imported sources
 *
 * Revision 1.4  2007/05/28 00:55:06  gtkwave
 * added support for arrays as a first class dumpfile datatype
 *
 * Revision 1.3  2007/04/29 04:13:49  gtkwave
 * changed anon union defined in struct Node to a named one as anon unions
 * are a gcc extension
 *
 * Revision 1.2  2007/04/20 02:08:17  gtkwave
 * initial release
 *
 */

