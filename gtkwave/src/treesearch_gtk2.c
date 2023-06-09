/* 
 * Copyright (c) Tristan Gingold and Tony Bybell 2006.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <config.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "gtk12compat.h"
#include "analyzer.h"
#include "tree.h"
#include "symbol.h"
#include "vcd.h"
#include "lx2.h"
#include "busy.h"
#include "debug.h"

/* Treesearch is a pop-up window used to select signals.
   It is composed of two main areas:
   * A tree area to select the hierarchy [tree area]
   * The (filtered) list of signals contained in the hierarchy [signal area].
*/

char autoname_bundles=0;

static GtkWidget *window1;
static GtkWidget *entry_a;
static char *entrybox_text_local=NULL;
static GtkSignalFunc cleanup_e;


/* SIG_ROOT is the branch currently selected.
   Signals of SIG_ROOT are displayed in the signals window.  */
static struct tree *sig_root = NULL;

/* Only signals which match the filter are displayed in the signal area.  */
static char *filter_str = NULL;

/* The signal area is based on a tree view which requires a store model.
   This store model contains the list of signals to be displayed.
*/
enum { NAME_COLUMN, TREE_COLUMN, N_COLUMNS };
static GtkListStore *sig_store;

/* Selection in the store model.  */
static GtkTreeSelection *sig_selection;

static int is_active=0;
GtkCTree *ctree_main=NULL;

/* Return 0 if the signal is not selected (ie filtered out).
*/
static int
filter_signal (const char *name, const char *filter)
{
  while (1)
    {
      char f = *filter;
      char n = *name;

      if (f == '*')
	{
	  /* The most complex first!:
	     '*' matches any number of characters.  */
	  filter++;

	  /* If '*' is the last char, then ok.  */
	  if (*filter == 0)
	    return 1;

	  for (; *name; name++)
	    if (filter_signal (name, filter))
	      return 1;

	  return 0;
	}
      else if (f == 0)
	{
	  /* End of filter.  */
	  return n == 0;
	}
      else if (f == '?')
	{
	  if (n == 0)
	    return 0;
	}
      else if (f != n)
	{
	  /* This also catch n == 0.  */
	  return 0;
	}

      name++;
      filter++;
    }
}


/* list of autocoalesced (synthesized) filter names that need to be freed at some point) */
static struct autocoalesce_free_list *afl = NULL;

static void free_afl(void)
{
struct autocoalesce_free_list *at;

while(afl)
	{
	if(afl->name) free_2(afl->name);
	at = afl->next;			
	free_2(afl);
	afl = at;
	}	
}


/* point to pure signame (remove hierarchy) for fill_sig_store() */
static char *prune_hierarchy(char *nam)
{
char cmpchar = alt_hier_delimeter ? alt_hier_delimeter : '.';
char *t = nam;
char *lastmatch = NULL;

while(t && *t)
	{
	if(*t == cmpchar) { lastmatch = t+1; }
	t++;
	}

return(lastmatch ? lastmatch : nam);
}


/* Fill the store model using current SIG_ROOT and FILTER_STR.  */
static void
fill_sig_store (void)
{
  struct tree *t;
  GtkTreeIter iter;

  free_afl();
  gtk_list_store_clear (sig_store);

  for (t = sig_root; t != NULL; t = t->next)
	{
	int i = t->which;
	char *s, *tmp2;

	if(i == -1) continue;

        if(!facs[i]->vec_root)
		{
		s = t->name;
                }
                else
                {
                if(autocoalesce)
                	{
			char *p;
                        if(facs[i]->vec_root!=facs[i]) continue;

                        tmp2=makename_chain(facs[i]);
			p = prune_hierarchy(tmp2);
                        s=(char *)malloc_2(strlen(p)+4);
                        strcpy(s,"[] ");
                        strcpy(s+3, p);
                        free_2(tmp2);
                        }
                        else
                        {
			char *p = prune_hierarchy(facs[i]->name);
                        s=(char *)malloc_2(strlen(p)+4);
                        strcpy(s,"[] ");
                        strcpy(s+3, p);
                        }
                }

	if (filter_str == NULL || wave_regex_match(t->name, WAVE_REGEX_TREE))
      		{
		gtk_list_store_prepend (sig_store, &iter);
		if(s == t->name)
			{
			gtk_list_store_set (sig_store, &iter,
				    NAME_COLUMN, t->name,
				    TREE_COLUMN, t,
				    -1);
			}
			else
			{
			struct autocoalesce_free_list *a = calloc_2(1, sizeof(struct autocoalesce_free_list));
			a->name = s;
			a->next = afl;
			afl = a;			

			gtk_list_store_set (sig_store, &iter,
				    NAME_COLUMN, s,
				    TREE_COLUMN, t,
				    -1);
			}
      		}
		else
		{
		if(s != t->name)
			{
			free_2(s);
			}
		}
	}
}


/* Callbacks for tree area when a row is selected/deselected.  */
static void select_row_callback(GtkWidget *widget, gint row, gint column,
        GdkEventButton *event, gpointer data)
{
struct tree *t;

t=(struct tree *)gtk_clist_get_row_data(GTK_CLIST(ctree_main), row);
DEBUG(printf("TS: %08x %s\n",t,t->name));
 sig_root = t->child;
 fill_sig_store ();
}

static void unselect_row_callback(GtkWidget *widget, gint row, gint column,
        GdkEventButton *event, gpointer data)
{
struct tree *t;

t=(struct tree *)gtk_clist_get_row_data(GTK_CLIST(ctree_main), row);
DEBUG(printf("TU: %08x %s\n",t,t->name));
 sig_root = treeroot;
 fill_sig_store ();
}

/* Signal callback for the filter widget.
   This catch the return key to update the signal area.  */
static
gboolean filter_edit_cb (GtkWidget *widget, GdkEventKey *ev, gpointer *data)
{
  /* Maybe this test is too strong ?  */
  if (ev->keyval == GDK_Return)
    {
      const char *t;

      /* Get the filter string, save it and change the store.  */
      if(filter_str)
	{
      	free ((char *)filter_str);
	}
      t = gtk_entry_get_text (GTK_ENTRY (widget));
      if (t == NULL || *t == 0)
	filter_str = NULL;
      else
	{
	filter_str = malloc_2(strlen(t) + 1);
	strcpy(filter_str, t);
	wave_regex_compile(filter_str, WAVE_REGEX_TREE);
	}
      fill_sig_store ();
    }
  return FALSE;
}



int treebox_is_active(void)
{
return(is_active);
}

static void enter_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
  G_CONST_RETURN gchar *entry_text;
  int len;
  entry_text = gtk_entry_get_text(GTK_ENTRY(entry_a));
  DEBUG(printf("Entry contents: %s\n", entry_text));
  if(!(len=strlen(entry_text))) entrybox_text_local=NULL;
	else strcpy((entrybox_text_local=(char *)malloc_2(len+1)),entry_text);

  gtk_grab_remove(window1);
  gtk_widget_destroy(window1);

  cleanup_e();
}

static void destroy_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
  DEBUG(printf("Entry Cancel\n"));
  entrybox_text_local=NULL;
  gtk_grab_remove(window1);
  gtk_widget_destroy(window1);
}

static void entrybox_local(char *title, int width, char *default_text, int maxch, GtkSignalFunc func)
{
    GtkWidget *vbox, *hbox;
    GtkWidget *button1, *button2;

    cleanup_e=func;

    /* create a new modal window */
    window1 = gtk_window_new(disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    gtk_grab_add(window1);
    gtk_widget_set_usize( GTK_WIDGET (window1), width, 60);
    gtk_window_set_title(GTK_WINDOW (window1), title);
    gtk_signal_connect(GTK_OBJECT (window1), "delete_event",
                       (GtkSignalFunc) destroy_callback_e, NULL);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window1), vbox);
    gtk_widget_show (vbox);

    entry_a = gtk_entry_new_with_max_length (maxch);
    gtk_signal_connect(GTK_OBJECT(entry_a), "activate",
		       GTK_SIGNAL_FUNC(enter_callback_e),
		       entry_a);
    gtk_entry_set_text (GTK_ENTRY (entry_a), default_text);
    gtk_entry_select_region (GTK_ENTRY (entry_a),
			     0, GTK_ENTRY(entry_a)->text_length);
    gtk_box_pack_start (GTK_BOX (vbox), entry_a, TRUE, TRUE, 0);
    gtk_widget_show (entry_a);

    hbox = gtk_hbox_new (FALSE, 1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("OK");
    gtk_widget_set_usize(button1, 100, -1);
    gtk_signal_connect(GTK_OBJECT (button1), "clicked",
			       GTK_SIGNAL_FUNC(enter_callback_e),
			       NULL);
    gtk_widget_show (button1);
    gtk_container_add (GTK_CONTAINER (hbox), button1);
    GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
    gtk_signal_connect_object (GTK_OBJECT (button1),
                                "realize",
                             (GtkSignalFunc) gtk_widget_grab_default,
                             GTK_OBJECT (button1));

    button2 = gtk_button_new_with_label ("Cancel");
    gtk_widget_set_usize(button2, 100, -1);
    gtk_signal_connect(GTK_OBJECT (button2), "clicked",
			       GTK_SIGNAL_FUNC(destroy_callback_e),
			       NULL);
    GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
    gtk_widget_show (button2);
    gtk_container_add (GTK_CONTAINER (hbox), button2);

    gtk_widget_show(window1);
}

/***************************************************************************/

static GtkWidget *window = NULL;
static GtkWidget *tree;
static char bundle_direction=0;
static GtkSignalFunc cleanup;


/* Get the highest signal from T.  */
struct tree *fetchhigh(struct tree *t)
{
while(t->child) t=t->child;
return(t);
}

/* Get the lowest signal from T.  */
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
			add_vector_range(NULL, fetchlow(t)->which,
				fetchhigh(t)->which, direction);
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
		add_vector_range(NULL, fetchlow(t)->which, 
			fetchhigh(t)->which, direction);
		}
	}
}


/* call cleanup() on ok/insert functions */

static void
bundle_cleanup_foreach (GtkTreeModel *model,
			GtkTreePath *path,
			GtkTreeIter *iter,
			gpointer data)
{
  struct tree *sel;

  /* Extract the tree.  */
  gtk_tree_model_get (model, iter, TREE_COLUMN, &sel, -1);

  if(!sel) return;

if(entrybox_text_local) 
        {
        char *efix;
 
	if(!strlen(entrybox_text_local))
		{
	        DEBUG(printf("Bundle name is not specified--recursing into hierarchy.\n"));
		fetchvex(sel, bundle_direction);
		}
		else
		{
	        efix=entrybox_text_local;
	        while(*efix)
	                {
	                if(*efix==' ')
	                        {
	                        *efix='_';
	                        }
	                efix++;
	                }
	 
	        DEBUG(printf("Bundle name is: %s\n",entrybox_text_local));
	        add_vector_range(entrybox_text_local, 
				fetchlow(sel)->which,
				fetchhigh(sel)->which, 
				bundle_direction);
		}
        free_2(entrybox_text_local);
        }
	else
	{
        DEBUG(printf("Bundle name is not specified--recursing into hierarchy.\n"));
	fetchvex(sel, bundle_direction);
	}
}

static void
bundle_cleanup(GtkWidget *widget, gpointer data)
{ 
  gtk_tree_selection_selected_foreach
    (sig_selection, &bundle_cleanup_foreach, NULL);

MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
}
 
static void
bundle_callback_generic(void)
{
  if(!autoname_bundles)
    {
      if (gtk_tree_selection_count_selected_rows (sig_selection) != 1)
	return;
      entrybox_local("Enter Bundle Name",300,"",128,
		     GTK_SIGNAL_FUNC(bundle_cleanup));
    }
  else
    {
      entrybox_text_local=NULL;
      bundle_cleanup(NULL, NULL);
    }
}

static void
bundle_callback_up(GtkWidget *widget, gpointer data)
{
bundle_direction=0;
bundle_callback_generic();
}

static void
bundle_callback_down(GtkWidget *widget, gpointer data)
{
bundle_direction=1;
bundle_callback_generic();
}

/* Callback for insert/replace/append buttions.
   This call-back is called for every signal selected.  */
enum cb_action { ACTION_INSERT, ACTION_REPLACE, ACTION_APPEND, ACTION_PREPEND };

static int pre_import = 0;

static Traces tcache;

static void
sig_selection_foreach (GtkTreeModel *model,
		       GtkTreePath *path,
		       GtkTreeIter *iter,
		       gpointer data)
{
  struct tree *sel;
  /* const enum cb_action action = (enum cb_action)data; */
  int i;
  int low, high;

  /* Get the tree.  */
  gtk_tree_model_get (model, iter, TREE_COLUMN, &sel, -1);

  if(!sel) return;

  low = fetchlow(sel)->which;
  high = fetchhigh(sel)->which;

  /* Add signals and vectors.  */
  for(i=low;i<=high;i++)
        {
	int len;
        struct symbol *s, *t;  
        s=facs[i];
	t=s->vec_root;
	if((t)&&(autocoalesce))
		{
		if(t->selected)
			{
			t->selected=0;
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
}

static void
sig_selection_foreach_finalize (gpointer data)
{
 const enum cb_action action = (enum cb_action)data;

 if (action == ACTION_REPLACE || action == ACTION_INSERT || action == ACTION_PREPEND)
   { 
     Trptr tfirst=NULL, tlast=NULL;

     if (action == ACTION_REPLACE)
       {
	 tfirst=traces.first; tlast=traces.last; /* cache for highlighting */
       }

     traces.buffercount=traces.total;
     traces.buffer=traces.first;
     traces.bufferlast=traces.last;
     traces.first=tcache.first;
     traces.last=tcache.last;
     traces.total=tcache.total;

     if(action == ACTION_PREPEND)
	{
	PrependBuffer();
	}
	else
	{
	PasteBuffer();
	}

     traces.buffercount=tcache.buffercount;
     traces.buffer=tcache.buffer;
     traces.bufferlast=tcache.bufferlast;
     
     if (action == ACTION_REPLACE)
       {
	 CutBuffer();

	 while(tfirst)
	   {
	     tfirst->flags |= TR_HIGHLIGHT;
	     if(tfirst==tlast) break;
	     tfirst=tfirst->t_next;
	   }
       }
   }
}

static void
sig_selection_foreach_preload_lx2
		      (GtkTreeModel *model,
		       GtkTreePath *path,
		       GtkTreeIter *iter,
		       gpointer data)
{
  struct tree *sel;
  /* const enum cb_action action = (enum cb_action)data; */
  int i;
  int low, high;

  /* Get the tree.  */
  gtk_tree_model_get (model, iter, TREE_COLUMN, &sel, -1);

  if(!sel) return;

  low = fetchlow(sel)->which;
  high = fetchhigh(sel)->which;

  /* If signals are vectors, coalesces vectors if so.  */
  for(i=low;i<=high;i++)
        {
        struct symbol *s;  
        s=facs[i];
	if(s->vec_root)
		{
		s->vec_root->selected=autocoalesce;
		}
        }

  /* LX2 */
  if(is_lx2)
        {
        for(i=low;i<=high;i++)
                {
                struct symbol *s, *t;
                s=facs[i];
                t=s->vec_root;
                if((t)&&(autocoalesce))
                        {
                        if(t->selected)
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
        }
  /* LX2 */
}


static void
action_callback(enum cb_action action)
{

  pre_import = 0;

  /* once through to mass gather lx2 traces... */
  gtk_tree_selection_selected_foreach
    (sig_selection, &sig_selection_foreach_preload_lx2, (void *)action);
  if(pre_import)
	{
        lx2_import_masked();
        }

  /* then do */
  if (action == ACTION_INSERT || action == ACTION_REPLACE || action == ACTION_PREPEND)
    {
      /* Save and clear current traces.  */
      memcpy(&tcache,&traces,sizeof(Traces));
      traces.total=0;
      traces.first=traces.last=NULL;
    }

  gtk_tree_selection_selected_foreach
    (sig_selection, &sig_selection_foreach, (void *)action);

  sig_selection_foreach_finalize((void *)action);

  MaxSignalLength();
  signalarea_configure_event(signalarea, NULL);
  wavearea_configure_event(wavearea, NULL);
}

static void insert_callback(GtkWidget *widget, GtkWidget *nothing)
{
  set_window_busy(widget);
  action_callback (ACTION_INSERT);
  set_window_idle(widget);
}

static void replace_callback(GtkWidget *widget, GtkWidget *nothing)
{
  set_window_busy(widget);
  action_callback (ACTION_REPLACE);
  set_window_idle(widget);
}

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
  set_window_busy(widget);
  action_callback (ACTION_APPEND);
  set_window_idle(widget);
}


static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
  is_active=0;
  gtk_widget_destroy(window);
  free_afl();
}



/*
 * mainline..
 */
void treebox(char *title, GtkSignalFunc func)
{
    GtkWidget *scrolled_win, *sig_scroll_win;
    GtkWidget *hbox;
    GtkWidget *button1, *button2, *button3, *button3a, *button4, *button5;
    GtkWidget *frame2, *frameh, *sig_frame;
    GtkWidget *vbox, *vpan, *filter_hbox;
    GtkWidget *filter_label, *filter_entry;
    GtkWidget *sig_view;
    GtkTooltips *tooltips;
    GtkCList  *clist;

    if(is_active) 
	{
	if(window) 
		{
		gdk_window_raise(window->window);
		}
		else
		{
#if GTK_CHECK_VERSION(2,4,0)
		if(expanderwindow)
			{
			gtk_expander_set_expanded(GTK_EXPANDER(expanderwindow), TRUE);
			}
#endif
		}
	return;
	}

    is_active=1;
    cleanup=func;

    /* create a new modal window */
    window = gtk_window_new(disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW (window), title);
    gtk_signal_connect(GTK_OBJECT (window), "delete_event",
                       (GtkSignalFunc) destroy_callback, NULL);

    tooltips=gtk_tooltips_new_2();

    vbox = gtk_vbox_new (FALSE, 1);
    gtk_widget_show (vbox);

    vpan = gtk_vpaned_new ();
    gtk_widget_show (vpan);
    gtk_box_pack_start (GTK_BOX (vbox), vpan, TRUE, TRUE, 1);

    /* Hierarchy.  */
    frame2 = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frame2), 3);
    gtk_widget_show(frame2);

    gtk_paned_pack1 (GTK_PANED (vpan), frame2, TRUE, FALSE);

    tree=gtk_ctree_new(1,0);
    ctree_main=GTK_CTREE(tree);
    gtk_clist_set_column_auto_resize (GTK_CLIST (tree), 0, TRUE);
    gtk_widget_show(tree);

    clist=GTK_CLIST(tree);
    gtk_signal_connect_object (GTK_OBJECT (clist), "select_row",
                               GTK_SIGNAL_FUNC(select_row_callback),
                               NULL);
    gtk_signal_connect_object (GTK_OBJECT (clist), "unselect_row",
                               GTK_SIGNAL_FUNC(unselect_row_callback),
                               NULL);

    gtk_clist_freeze(clist);
    gtk_clist_clear(clist);

    maketree(NULL, treeroot);
    gtk_clist_thaw(clist);

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize( GTK_WIDGET (scrolled_win), -1, 50);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_win);
    gtk_container_add (GTK_CONTAINER (scrolled_win), GTK_WIDGET (tree));
    gtk_container_add (GTK_CONTAINER (frame2), scrolled_win);


    /* Signal names.  */
    sig_store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
    sig_root = treeroot;
    fill_sig_store ();

    sig_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (sig_store));

    /* The view now holds a reference.  We can get rid of our own reference */
    g_object_unref (G_OBJECT (sig_store));


      {
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Signals",
							   renderer,
							   "text", NAME_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);


	/* Setup the selection handler */
	sig_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (sig_view));
	gtk_tree_selection_set_mode (sig_selection, GTK_SELECTION_MULTIPLE);
      }

    dnd_setup(sig_view);

    sig_frame = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (sig_frame), 3);
    gtk_widget_show(sig_frame);

    gtk_paned_pack2 (GTK_PANED (vpan), sig_frame, TRUE, FALSE);

    sig_scroll_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize (GTK_WIDGET (sig_scroll_win), 80, 100);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sig_scroll_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(sig_scroll_win);
    gtk_container_add (GTK_CONTAINER (sig_frame), sig_scroll_win);
    gtk_container_add (GTK_CONTAINER (sig_scroll_win), sig_view);
    gtk_widget_show (sig_view);


    /* Filter.  */
    filter_hbox = gtk_hbox_new (FALSE, 1);
    gtk_widget_show (filter_hbox);

    filter_label = gtk_label_new ("Filter:");
    gtk_widget_show (filter_label);
    gtk_box_pack_start (GTK_BOX (filter_hbox), filter_label, FALSE, FALSE, 1);

    filter_entry = gtk_entry_new ();
    if(filter_str) { gtk_entry_set_text(GTK_ENTRY(filter_entry), filter_str); }
    gtk_widget_show (filter_entry);

    gtk_signal_connect(GTK_OBJECT (filter_entry), "key_press_event",
                       (GtkSignalFunc) filter_edit_cb, NULL);
    gtk_tooltips_set_tip_2(tooltips, filter_entry,
			   "Add a POSIX filter. "
			   "'.*' matches any number of characters,"
			   " '.' matches any character.  Hit Return to apply",
			   NULL);

    gtk_box_pack_start (GTK_BOX (filter_hbox), filter_entry, FALSE, FALSE, 1);

    gtk_box_pack_start (GTK_BOX (vbox), filter_hbox, FALSE, FALSE, 1);

    /* Buttons.  */
    frameh = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frameh), 3);
    gtk_widget_show(frameh);
    gtk_box_pack_start (GTK_BOX (vbox), frameh, FALSE, FALSE, 1);


    hbox = gtk_hbox_new (FALSE, 1);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("Append");
    gtk_container_border_width (GTK_CONTAINER (button1), 3);
    gtk_signal_connect_object (GTK_OBJECT (button1), "clicked",
			       GTK_SIGNAL_FUNC(ok_callback),
			       GTK_OBJECT (window));
    gtk_widget_show (button1);
    gtk_tooltips_set_tip_2(tooltips, button1, 
		"Add selected signal hierarchy to end of the display on the main window.",NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button1, TRUE, FALSE, 0);

    button2 = gtk_button_new_with_label (" Insert ");
    gtk_container_border_width (GTK_CONTAINER (button2), 3);
    gtk_signal_connect_object (GTK_OBJECT (button2), "clicked",
			       GTK_SIGNAL_FUNC(insert_callback),
			       GTK_OBJECT (window));
    gtk_widget_show (button2);
    gtk_tooltips_set_tip_2(tooltips, button2, 
		"Add selected signal hierarchy after last highlighted signal on the main window.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button2, TRUE, FALSE, 0);

    if(vcd_explicit_zero_subscripts>=0)
	{
    	button3 = gtk_button_new_with_label (" Bundle Up ");
    	gtk_container_border_width (GTK_CONTAINER (button3), 3);
    	gtk_signal_connect_object (GTK_OBJECT (button3), "clicked",
			       GTK_SIGNAL_FUNC(bundle_callback_up),
			       GTK_OBJECT (window));
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
    	gtk_signal_connect_object (GTK_OBJECT (button3a), "clicked",
			       GTK_SIGNAL_FUNC(bundle_callback_down),
			       GTK_OBJECT (window));
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
    gtk_signal_connect_object (GTK_OBJECT (button4), "clicked",
			       GTK_SIGNAL_FUNC(replace_callback),
			       GTK_OBJECT (window));
    gtk_widget_show (button4);
    gtk_tooltips_set_tip_2(tooltips, button4, 
		"Replace highlighted signals on the main window with signals selected above.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button4, TRUE, FALSE, 0);

    button5 = gtk_button_new_with_label (" Exit ");
    gtk_container_border_width (GTK_CONTAINER (button5), 3);
    gtk_signal_connect_object (GTK_OBJECT (button5), "clicked",
			       GTK_SIGNAL_FUNC(destroy_callback),
			       GTK_OBJECT (window));
    gtk_tooltips_set_tip_2(tooltips, button5, 
		"Do nothing and return to the main window.",NULL);
    gtk_widget_show (button5);
    gtk_box_pack_start (GTK_BOX (hbox), button5, TRUE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (frameh), hbox);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    gtk_window_set_default_size (GTK_WINDOW (window), 200, 400);
    gtk_widget_show(window);
}


/*
 * for use with expander in gtk2.4 and higher...
 */
GtkWidget* treeboxframe(char *title, GtkSignalFunc func)
{
    GtkWidget *scrolled_win, *sig_scroll_win;
    GtkWidget *hbox;
    GtkWidget *button1, *button2, *button3, *button3a, *button4;
    GtkWidget *frame2, *frameh, *sig_frame;
    GtkWidget *vbox, *vpan, *filter_hbox;
    GtkWidget *filter_label, *filter_entry;
    GtkWidget *sig_view;
    GtkTooltips *tooltips;
    GtkCList  *clist;

    is_active=1;
    cleanup=func;

    /* create a new modal window */
    tooltips=gtk_tooltips_new_2();

    vbox = gtk_vbox_new (FALSE, 1);
    gtk_widget_show (vbox);

    vpan = gtk_vpaned_new ();
    gtk_widget_show (vpan);
    gtk_box_pack_start (GTK_BOX (vbox), vpan, TRUE, TRUE, 1);

    /* Hierarchy.  */
    frame2 = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frame2), 3);
    gtk_widget_show(frame2);

    gtk_paned_pack1 (GTK_PANED (vpan), frame2, TRUE, FALSE);

    tree=gtk_ctree_new(1,0);
    ctree_main=GTK_CTREE(tree);
    gtk_clist_set_column_auto_resize (GTK_CLIST (tree), 0, TRUE);
    gtk_widget_show(tree);

    clist=GTK_CLIST(tree);
    gtk_signal_connect_object (GTK_OBJECT (clist), "select_row",
                               GTK_SIGNAL_FUNC(select_row_callback),
                               NULL);
    gtk_signal_connect_object (GTK_OBJECT (clist), "unselect_row",
                               GTK_SIGNAL_FUNC(unselect_row_callback),
                               NULL);

    gtk_clist_freeze(clist);
    gtk_clist_clear(clist);

    maketree(NULL, treeroot);
    gtk_clist_thaw(clist);

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize( GTK_WIDGET (scrolled_win), -1, 50);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_win);
    gtk_container_add (GTK_CONTAINER (scrolled_win), GTK_WIDGET (tree));
    gtk_container_add (GTK_CONTAINER (frame2), scrolled_win);


    /* Signal names.  */
    sig_store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
    sig_root = treeroot;
    fill_sig_store ();

    sig_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (sig_store));

    /* The view now holds a reference.  We can get rid of our own reference */
    g_object_unref (G_OBJECT (sig_store));


      {
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Signals",
							   renderer,
							   "text", NAME_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);


	/* Setup the selection handler */
	sig_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (sig_view));
	gtk_tree_selection_set_mode (sig_selection, GTK_SELECTION_MULTIPLE);
      }

    dnd_setup(sig_view);

    sig_frame = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (sig_frame), 3);
    gtk_widget_show(sig_frame);

    gtk_paned_pack2 (GTK_PANED (vpan), sig_frame, TRUE, FALSE);

    sig_scroll_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize (GTK_WIDGET (sig_scroll_win), 80, 100);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sig_scroll_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(sig_scroll_win);
    gtk_container_add (GTK_CONTAINER (sig_frame), sig_scroll_win);
    gtk_container_add (GTK_CONTAINER (sig_scroll_win), sig_view);
    gtk_widget_show (sig_view);


    /* Filter.  */
    filter_hbox = gtk_hbox_new (FALSE, 1);
    gtk_widget_show (filter_hbox);

    filter_label = gtk_label_new ("Filter:");
    gtk_widget_show (filter_label);
    gtk_box_pack_start (GTK_BOX (filter_hbox), filter_label, FALSE, FALSE, 1);

    filter_entry = gtk_entry_new ();
    if(filter_str) { gtk_entry_set_text(GTK_ENTRY(filter_entry), filter_str); }
    gtk_widget_show (filter_entry);

    gtk_signal_connect(GTK_OBJECT (filter_entry), "key_press_event",
                       (GtkSignalFunc) filter_edit_cb, NULL);
    gtk_tooltips_set_tip_2(tooltips, filter_entry,
			   "Add a POSIX filter. "
			   "'.*' matches any number of characters,"
			   " '.' matches any character.  Hit Return to apply",
			   NULL);

    gtk_box_pack_start (GTK_BOX (filter_hbox), filter_entry, FALSE, FALSE, 1);

    gtk_box_pack_start (GTK_BOX (vbox), filter_hbox, FALSE, FALSE, 1);

    /* Buttons.  */
    frameh = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frameh), 3);
    gtk_widget_show(frameh);
    gtk_box_pack_start (GTK_BOX (vbox), frameh, FALSE, FALSE, 1);


    hbox = gtk_hbox_new (FALSE, 1);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("Append");
    gtk_container_border_width (GTK_CONTAINER (button1), 3);
    gtk_signal_connect_object (GTK_OBJECT (button1), "clicked",
			       GTK_SIGNAL_FUNC(ok_callback),
			       GTK_OBJECT (frame2));
    gtk_widget_show (button1);
    gtk_tooltips_set_tip_2(tooltips, button1, 
		"Add selected signal hierarchy to end of the display on the main window.",NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button1, TRUE, FALSE, 0);

    button2 = gtk_button_new_with_label (" Insert ");
    gtk_container_border_width (GTK_CONTAINER (button2), 3);
    gtk_signal_connect_object (GTK_OBJECT (button2), "clicked",
			       GTK_SIGNAL_FUNC(insert_callback),
			       GTK_OBJECT (frame2));
    gtk_widget_show (button2);
    gtk_tooltips_set_tip_2(tooltips, button2, 
		"Add selected signal hierarchy after last highlighted signal on the main window.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button2, TRUE, FALSE, 0);

    if(vcd_explicit_zero_subscripts>=0)
	{
    	button3 = gtk_button_new_with_label (" Bundle Up ");
    	gtk_container_border_width (GTK_CONTAINER (button3), 3);
    	gtk_signal_connect_object (GTK_OBJECT (button3), "clicked",
			       GTK_SIGNAL_FUNC(bundle_callback_up),
			       GTK_OBJECT (frame2));
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
    	gtk_signal_connect_object (GTK_OBJECT (button3a), "clicked",
			       GTK_SIGNAL_FUNC(bundle_callback_down),
			       GTK_OBJECT (frame2));
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
    gtk_signal_connect_object (GTK_OBJECT (button4), "clicked",
			       GTK_SIGNAL_FUNC(replace_callback),
			       GTK_OBJECT (frame2));
    gtk_widget_show (button4);
    gtk_tooltips_set_tip_2(tooltips, button4, 
		"Replace highlighted signals on the main window with signals selected above.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button4, TRUE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (frameh), hbox);
    gtk_widget_show(vbox);
    return vbox;
}


/****************************************************************
 **
 ** dnd
 **
 ****************************************************************/

#define DRAG_TAR_NAME_0		"text/plain"
#define DRAG_TAR_INFO_0		0

#define DRAG_TAR_NAME_1		"text/uri-list"		/* not url-list */
#define DRAG_TAR_INFO_1		1

#define DRAG_TAR_NAME_2		"STRING"
#define DRAG_TAR_INFO_2		2

static int dnd_tgt_on_signalarea = 0;

/*
 *	DND "drag_begin" handler, this is called whenever a drag starts.
 */
static void DNDBeginCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
        if((widget == NULL) || (dc == NULL))
		return;

	/* Put any needed drag begin setup code here. */
}

/*
 *      DND "drag_end" handler, this is called when a drag and drop has
 *	completed. So this function is the last one to be called in
 *	any given DND operation.
 */
static void DNDEndCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
Trptr t;
int trwhich, trtarget;
GdkModifierType state;
gdouble x, y;
#ifdef WAVE_USE_GTK2
gint xi, yi;
#endif

if((widget == NULL) || (dc == NULL)) return;

/* Put any needed drag end cleanup code here. */

if(!dnd_tgt_on_signalarea) return;

WAVE_GDK_GET_POINTER(signalarea->window, &x, &y, &xi, &yi, &state);
WAVE_GDK_GET_POINTER_COPY;

if((x<0)||(y<0)||(x>signalarea->allocation.width)||(y>signalarea->allocation.height)) return;

if((t=traces.first))
        {       
        while(t)
                {
                t->flags&=~TR_HIGHLIGHT;
                t=t->t_next;
                }
        signalarea_configure_event(signalarea, NULL);
        wavearea_configure_event(wavearea, NULL);
	}

trtarget = ((int)y / (int)fontheight) - 2; 
if(trtarget < 0) 
	{
	Trptr tp = topmost_trace ? GivePrevTrace(topmost_trace): NULL;
	trtarget = 0;

	if(tp)
		{
		t = tp;
		}
		else
		{
		action_callback(ACTION_PREPEND);  /* prepend in this widget only ever used by this function call */
		goto dnd_import_fini;
		}
	}
	else
	{
	t=topmost_trace;
	}

trwhich=0;
while(t)
	{
        if((trwhich<trtarget)&&(GiveNextTrace(t)))
        	{
                trwhich++;
                t=GiveNextTrace(t);
                }
                else
                {
                break;
                }
	}

if(t)
	{
	t->flags |= TR_HIGHLIGHT;
	}

action_callback (ACTION_INSERT);

if(t)
	{
	t->flags &= ~TR_HIGHLIGHT;
	}

dnd_import_fini:

MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);

}

/*
 *	DND "drag_motion" handler, this is called whenever the 
 *	pointer is dragging over the target widget.
 */
static gboolean DNDDragMotionCB(
        GtkWidget *widget, GdkDragContext *dc,
        gint x, gint y, guint t,
        gpointer data
)
{
	gboolean same_widget;
	GdkDragAction suggested_action;
	GtkWidget *src_widget, *tar_widget;
        if((widget == NULL) || (dc == NULL))
                return(FALSE);

	/* Get source widget and target widget. */
	src_widget = gtk_drag_get_source_widget(dc);
	tar_widget = widget;

	/* Note if source widget is the same as the target. */
	same_widget = (src_widget == tar_widget) ? TRUE : FALSE;

	dnd_tgt_on_signalarea = (tar_widget == signalarea);

	/* If this is the same widget, our suggested action should be
	 * move.  For all other case we assume copy.
	 */
	if(same_widget)
		suggested_action = GDK_ACTION_MOVE;
	else
		suggested_action = GDK_ACTION_COPY;

	/* Respond with default drag action (status). First we check
	 * the dc's list of actions. If the list only contains
	 * move, copy, or link then we select just that, otherwise we
	 * return with our default suggested action.
	 * If no valid actions are listed then we respond with 0.
	 */

        /* Only move? */
        if(dc->actions == GDK_ACTION_MOVE)
            gdk_drag_status(dc, GDK_ACTION_MOVE, t);
        /* Only copy? */
        else if(dc->actions == GDK_ACTION_COPY)
            gdk_drag_status(dc, GDK_ACTION_COPY, t);
        /* Only link? */
        else if(dc->actions == GDK_ACTION_LINK)
            gdk_drag_status(dc, GDK_ACTION_LINK, t);
        /* Other action, check if listed in our actions list? */
        else if(dc->actions & suggested_action)
            gdk_drag_status(dc, suggested_action, t);
        /* All else respond with 0. */
        else
            gdk_drag_status(dc, 0, t);

	return(FALSE);
}

/*
 *	DND "drag_data_get" handler, for handling requests for DND
 *	data on the specified widget. This function is called when
 *	there is need for DND data on the source, so this function is
 *	responsable for setting up the dynamic data exchange buffer
 *	(DDE as sometimes it is called) and sending it out.
 */
static void DNDDataRequestCB(
	GtkWidget *widget, GdkDragContext *dc,
	GtkSelectionData *selection_data, guint info, guint t,
	gpointer data
)
{
}

/*
 *      DND "drag_data_received" handler. When DNDDataRequestCB()
 *	calls gtk_selection_data_set() to send out the data, this function
 *	recieves it and is responsible for handling it.
 *
 *	This is also the only DND callback function where the given
 *	inputs may reflect those of the drop target so we need to check
 *	if this is the same structure or not.
 */
static void DNDDataRecievedCB(
	GtkWidget *widget, GdkDragContext *dc,
	gint x, gint y, GtkSelectionData *selection_data,
	guint info, guint t, gpointer data
)
{
}

/*
 *	DND "drag_data_delete" handler, this function is called when
 *	the data on the source `should' be deleted (ie if the DND was
 *	a move).
 */
static void DNDDataDeleteCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
/* nothing */
}  


/***********************/


void dnd_setup(GtkWidget *w)
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
		target_entry[0].target = DRAG_TAR_NAME_0;
		target_entry[0].flags = 0;
		target_entry[0].info = DRAG_TAR_INFO_0;
                target_entry[1].target = DRAG_TAR_NAME_1;
                target_entry[1].flags = 0;
                target_entry[1].info = DRAG_TAR_INFO_1;
                target_entry[2].target = DRAG_TAR_NAME_2;
                target_entry[2].flags = 0;
                target_entry[2].info = DRAG_TAR_INFO_2;

		/* Set the drag destination for this widget, using the
		 * above target entry types, accept move's and coppies'.
		 */
		gtk_drag_dest_set(
			w,
			GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT |
			GTK_DEST_DEFAULT_DROP,
			target_entry,
			sizeof(target_entry) / sizeof(GtkTargetEntry),
			GDK_ACTION_MOVE | GDK_ACTION_COPY
		);
		gtk_signal_connect(
			GTK_OBJECT(w), "drag_motion",
			GTK_SIGNAL_FUNC(DNDDragMotionCB),
			win
		);

		/* Set the drag source for this widget, allowing the user
		 * to drag items off of this clist.
		 */
		gtk_drag_source_set(
			w,
			GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                        target_entry,
                        sizeof(target_entry) / sizeof(GtkTargetEntry),
			GDK_ACTION_MOVE | GDK_ACTION_COPY
		);
		/* Set DND signals on clist. */
		gtk_signal_connect(
			GTK_OBJECT(w), "drag_begin",
			GTK_SIGNAL_FUNC(DNDBeginCB), win
		);
                gtk_signal_connect(
                        GTK_OBJECT(w), "drag_end",
                        GTK_SIGNAL_FUNC(DNDEndCB), win
                );
                gtk_signal_connect(
                        GTK_OBJECT(w), "drag_data_get",
                        GTK_SIGNAL_FUNC(DNDDataRequestCB), win
                );
                gtk_signal_connect(
                        GTK_OBJECT(w), "drag_data_received",
                        GTK_SIGNAL_FUNC(DNDDataRecievedCB), win
                );
                gtk_signal_connect(
                        GTK_OBJECT(w), "drag_data_delete",
                        GTK_SIGNAL_FUNC(DNDDataDeleteCB), win
                );
	}
}

/*
 * $Id$
 * $Log$
 * Revision 1.3  2007/04/29 04:13:49  gtkwave
 * changed anon union defined in struct Node to a named one as anon unions
 * are a gcc extension
 *
 * Revision 1.2  2007/04/20 02:08:17  gtkwave
 * initial release
 *
 */

