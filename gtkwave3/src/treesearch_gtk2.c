/* 
 * Copyright (c) Tristan Gingold and Tony Bybell 2006-2011.
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
#include "tcl_helper.h"

WAVE_NODEVARTYPE_STR

enum { VIEW_DRAG_INACTIVE, TREE_TO_VIEW_DRAG_ACTIVE, SEARCH_TO_VIEW_DRAG_ACTIVE };

/* Treesearch is a pop-up window used to select signals.
   It is composed of two main areas:
   * A tree area to select the hierarchy [tree area]
   * The (filtered) list of signals contained in the hierarchy [signal area].
*/

/* SIG_ROOT is the branch currently selected.
   Signals of SIG_ROOT are displayed in the signals window.  */

/* Only signals which match the filter are displayed in the signal area.  */

/* The signal area is based on a tree view which requires a store model.
   This store model contains the list of signals to be displayed.
*/
enum { NAME_COLUMN, TREE_COLUMN, TYPE_COLUMN, N_COLUMNS };

/* list of autocoalesced (synthesized) filter names that need to be freed at some point) */

void free_afl(void)
{
struct autocoalesce_free_list *at;

while(GLOBALS->afl_treesearch_gtk2_c_1)
	{
	if(GLOBALS->afl_treesearch_gtk2_c_1->name) free_2(GLOBALS->afl_treesearch_gtk2_c_1->name);
	at = GLOBALS->afl_treesearch_gtk2_c_1->next;			
	free_2(GLOBALS->afl_treesearch_gtk2_c_1);
	GLOBALS->afl_treesearch_gtk2_c_1 = at;
	}	
}


/* point to pure signame (remove hierarchy) for fill_sig_store() */
static char *prune_hierarchy(char *nam)
{
char cmpchar = GLOBALS->alt_hier_delimeter ? GLOBALS->alt_hier_delimeter : '.';
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
void
fill_sig_store (void)
{
  struct tree *t;
  GtkTreeIter iter;

  if(GLOBALS->selected_sig_name)
	{
	free_2(GLOBALS->selected_sig_name);
	GLOBALS->selected_sig_name = NULL;
	}

  free_afl();
  gtk_list_store_clear (GLOBALS->sig_store_treesearch_gtk2_c_1);

  for (t = GLOBALS->sig_root_treesearch_gtk2_c_1; t != NULL; t = t->next)
	{
	int i = t->t_which;
	char *s, *tmp2;
	int vartype;

	if(i < 0) continue;

	vartype = GLOBALS->facs[i]->n->vartype;
	if((vartype < 0) || (vartype > ND_VARTYPE_MAX))
		{
		vartype = 0;
		}

        if(!GLOBALS->facs[i]->vec_root)
		{
		s = t->name;
                }
                else
                {
                if(GLOBALS->autocoalesce)
                	{
			char *p;
                        if(GLOBALS->facs[i]->vec_root!=GLOBALS->facs[i]) continue;

                        tmp2=makename_chain(GLOBALS->facs[i]);
			p = prune_hierarchy(tmp2);
                        s=(char *)malloc_2(strlen(p)+4);
                        strcpy(s,"[] ");
                        strcpy(s+3, p);
                        free_2(tmp2);
                        }
                        else
                        {
			char *p = prune_hierarchy(GLOBALS->facs[i]->name);
                        s=(char *)malloc_2(strlen(p)+4);
                        strcpy(s,"[] ");
                        strcpy(s+3, p);
                        }
                }

	if (GLOBALS->filter_str_treesearch_gtk2_c_1 == NULL || wave_regex_match(t->name, WAVE_REGEX_TREE))
      		{
		gtk_list_store_prepend (GLOBALS->sig_store_treesearch_gtk2_c_1, &iter);
		if(s == t->name)
			{
			gtk_list_store_set (GLOBALS->sig_store_treesearch_gtk2_c_1, &iter,
				    NAME_COLUMN, t->name,
				    TREE_COLUMN, t,
				    TYPE_COLUMN, vartype_strings[vartype],
				    -1);
			}
			else
			{
			struct autocoalesce_free_list *a = calloc_2(1, sizeof(struct autocoalesce_free_list));
			a->name = s;
			a->next = GLOBALS->afl_treesearch_gtk2_c_1;
			GLOBALS->afl_treesearch_gtk2_c_1 = a;			

			gtk_list_store_set (GLOBALS->sig_store_treesearch_gtk2_c_1, &iter,
				    NAME_COLUMN, s,
				    TREE_COLUMN, t,
				    TYPE_COLUMN, vartype_strings[vartype],
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

/*
 * tree open/close handling
 */
static void create_sst_nodes_if_necessary(GtkCTreeNode *node)
{
#ifndef WAVE_DISABLE_FAST_TREE
if(node)
	{
	GtkCTreeRow *gctr = GTK_CTREE_ROW(node);
	struct tree *t = (struct tree *)(gctr->row.data);

	if(t->child)
		{
		GtkCTree *ctree = GLOBALS->ctree_main;
		GtkCTreeNode *n = gctr->children;
	
		gtk_clist_freeze(GTK_CLIST(ctree));

		while(n)
			{
			GtkCTreeRow *g = GTK_CTREE_ROW(n);

			t = (struct tree *)(g->row.data);
			if(t->t_which < 0)
				{
				if(!t->children_in_gui)
					{
					t->children_in_gui = 1;
					maketree2(n, t, 0, n);
					}
				}
	
			n = GTK_CTREE_NODE_NEXT(n);
			}
	
		gtk_clist_thaw(GTK_CLIST(ctree));
		}
	}
#endif
}


int force_open_tree_node(char *name) {
  GtkCTree *ctree = GLOBALS->ctree_main;
  int rv = 1 ;			/* can possible open */
  if(ctree) {
    int namlen = strlen(name);
    char *namecache = wave_alloca(namlen+1);
    char *name_end = name + namlen - 1;
    char *zap = name;
    GtkCTreeNode *node = GLOBALS->any_tree_node;
    GtkCTreeRow *gctr = GTK_CTREE_ROW(node);
    int depth = 1;
    
    strcpy(namecache, name);
    while(gctr->parent)	{
      node = gctr->parent;
      gctr = GTK_CTREE_ROW(node);
    } 
    for(;;) {
      struct tree *t = gctr->row.data;
      while(*zap) {
	if(*zap != GLOBALS->hier_delimeter) {
	  zap++;
	}
	else {
	  *zap = 0;
	  break;
	}
      }
      if(!strcmp(t->name, name)) {
	if(zap == name_end) {
	  GtkCTreeNode **nodehist = wave_alloca(depth * sizeof(GtkCTreeNode *));
	  int *exp1 = wave_alloca(depth * sizeof(int));
	  int i = depth-1;
	  nodehist[i] = node;
	  exp1[i--] = 1;
	  /* now work backwards up to parent getting the node open/close history*/
	  gctr = GTK_CTREE_ROW(node);
	  while(gctr->parent) {        
	    node = gctr->parent;
	    gctr = GTK_CTREE_ROW(node);
	    nodehist[i] = node;
	    exp1[i--] = gctr->expanded;
	  }        
	  gtk_clist_freeze(GTK_CLIST(ctree));
	  /* fully expand down */
	  for(i=0;i<depth;i++) {
	    gtk_ctree_expand(ctree, nodehist[i]);
	  }
	  /* work backwards and close up nodes that were originally closed */
	  for(i=depth-1;i>=0;i--) {
	    if(exp1[i]) {
	      gtk_ctree_expand(ctree, nodehist[i]);
	    }
	    else {
	      gtk_ctree_collapse(ctree, nodehist[i]);
	    }
	  }
	  gtk_clist_thaw(GTK_CLIST(ctree));

	  gtk_ctree_node_moveto(ctree, nodehist[depth-1],
				depth /*column*/,
				0.5 /*row_align*/,
				0.5 /*col_align*/);
	  /* printf("[treeopennode] '%s' ok\n", name); */
	  rv = 0 ;		/* opened */
	  GLOBALS->open_tree_nodes = xl_insert(namecache, GLOBALS->open_tree_nodes, NULL);
	  return rv;
	}
	else {
	  depth++;
	  create_sst_nodes_if_necessary(node);
	  node = gctr->children;
	  if(!node) break;
	  gctr = GTK_CTREE_ROW(node);
	  if(!gctr) break;
	  name = ++zap;
	  continue;
	}
      }
      node = gctr->sibling;
      if(!node) break;
      gctr = GTK_CTREE_ROW(node);
    }
     /* printf("[treeopennode] '%s' failed\n", name); */
  } else {
    rv = -1 ;			/* tree does not exist */
  }
  return rv ;
}

void dump_open_tree_nodes(FILE *wave, xl_Tree *t)
{
if(t->left)
	{
	dump_open_tree_nodes(wave, t->left);
	}

fprintf(wave, "[treeopen] %s\n", t->item);

if(t->right)
	{
	dump_open_tree_nodes(wave, t->right);
	}
}

static void generic_tree_expand_collapse_callback(int is_expand, GtkCTreeNode *node) 
{
GtkCTreeRow **gctr;
int depth, i;
int len = 1;
char *tstring;
char hier_suffix[2];
int found;

hier_suffix[0] = GLOBALS->hier_delimeter;
hier_suffix[1] = 0;

if(!node) return;

depth = GTK_CTREE_ROW(node)->level;
gctr = wave_alloca(depth * sizeof(GtkCTreeRow *));

for(i=depth-1;i>=0;i--)
	{
	struct tree *t;
	gctr[i] = GTK_CTREE_ROW(node);
	t = gctr[i]->row.data;
	len += (strlen(t->name) + 1);
	node = gctr[i]->parent;
	}

tstring = wave_alloca(len);
memset(tstring, 0, len);

for(i=0;i<depth;i++)
	{
	struct tree *t = gctr[i]->row.data;
	strcat(tstring, t->name);
	strcat(tstring, hier_suffix);
	}


if(GLOBALS->open_tree_nodes) /* cut down on chatter to Tcl clients */
	{
	GLOBALS->open_tree_nodes = xl_splay(tstring, GLOBALS->open_tree_nodes);
	if(!strcmp(GLOBALS->open_tree_nodes->item, tstring))
		{
		found = 1;
		}
		else
		{
		found = 0;
		}
	}
	else
	{
	found = 0;
	}

if(is_expand)
	{
	GLOBALS->open_tree_nodes = xl_insert(tstring, GLOBALS->open_tree_nodes, NULL);
	if(!found)
		{
		gtkwavetcl_setvar(WAVE_TCLCB_TREE_EXPAND, tstring, WAVE_TCLCB_TREE_EXPAND_FLAGS);
		}
	}
	else
	{
	GLOBALS->open_tree_nodes = xl_delete(tstring, GLOBALS->open_tree_nodes);
	if(found)
		{
		gtkwavetcl_setvar(WAVE_TCLCB_TREE_COLLAPSE, tstring, WAVE_TCLCB_TREE_COLLAPSE_FLAGS);
		}
	}
}

static void tree_expand_callback(GtkCTree *ctree, GtkCTreeNode *node, gpointer user_data)
{
create_sst_nodes_if_necessary(node);
generic_tree_expand_collapse_callback(1, node);
}
 
static void tree_collapse_callback(GtkCTree *ctree, GtkCTreeNode *node, gpointer user_data) 
{
generic_tree_expand_collapse_callback(0, node);
}


void select_tree_node(char *name)
{
GtkCTree *ctree = GLOBALS->ctree_main;

if(ctree)
	{
	int namlen = strlen(name);
	char *namecache = wave_alloca(namlen+1);
	char *name_end = name + namlen - 1;
	char *zap = name;
	GtkCTreeNode *node = GLOBALS->any_tree_node;
	GtkCTreeRow *gctr = GTK_CTREE_ROW(node);

	strcpy(namecache, name);

	while(gctr->parent)
		{
		node = gctr->parent;
		gctr = GTK_CTREE_ROW(node);
		} 

	for(;;)
		{
		struct tree *t = gctr->row.data;

		while(*zap)
			{
			if(*zap != GLOBALS->hier_delimeter)
				{
				zap++;
				}
				else
				{
				*zap = 0;
				break;
				}
			}

		if(!strcmp(t->name, name))
			{
			if(zap == name_end)
				{
				/* printf("[treeselectnode] '%s' ok\n", name); */
				gtk_ctree_select(ctree, node);

				GLOBALS->sig_root_treesearch_gtk2_c_1 = t->child;
				fill_sig_store ();

				return;
				}
				else
				{
				node = gctr->children;
				gctr = GTK_CTREE_ROW(node);
				if(!gctr) break;

				name = ++zap;
				continue;
				}
			}

		node = gctr->sibling;
		if(!node) break;
		gctr = GTK_CTREE_ROW(node);
		}

	/* printf("[treeselectnode] '%s' failed\n", name); */
	}
}



/* Callbacks for tree area when a row is selected/deselected.  */
static void select_row_callback(GtkWidget *widget, gint row, gint column,
        GdkEventButton *event, gpointer data)
{
struct tree *t;
GtkCTreeNode* node = gtk_ctree_node_nth(GLOBALS->ctree_main, row);

if(node)
	{
	GtkCTreeRow **gctr;
	int depth, i;
	int len = 1;
	char *tstring;
	char hier_suffix[2];

	hier_suffix[0] = GLOBALS->hier_delimeter;
	hier_suffix[1] = 0;
                
	depth = GTK_CTREE_ROW(node)->level;
	gctr = wave_alloca(depth * sizeof(GtkCTreeRow *));
         
	for(i=depth-1;i>=0;i--)
	        {
	        gctr[i] = GTK_CTREE_ROW(node);
	        t = gctr[i]->row.data;
	        len += (strlen(t->name) + 1);
	        node = gctr[i]->parent;
	        }        

	tstring = wave_alloca(len);
	memset(tstring, 0, len);

	for(i=0;i<depth;i++)
	        {
	        t = gctr[i]->row.data;
	        strcat(tstring, t->name);
	        strcat(tstring, hier_suffix);
	        }

	if(GLOBALS->selected_hierarchy_name)
		{
		free_2(GLOBALS->selected_hierarchy_name);
		}
	GLOBALS->selected_hierarchy_name = strdup_2(tstring);
	}

t=(struct tree *)gtk_clist_get_row_data(GTK_CLIST(GLOBALS->ctree_main), row);
DEBUG(printf("TS: %08x %s\n",t,t->name));
 GLOBALS->sig_root_treesearch_gtk2_c_1 = t->child;
 fill_sig_store ();
 gtkwavetcl_setvar(WAVE_TCLCB_TREE_SELECT, GLOBALS->selected_hierarchy_name, WAVE_TCLCB_TREE_SELECT_FLAGS);
}

static void unselect_row_callback(GtkWidget *widget, gint row, gint column,
        GdkEventButton *event, gpointer data)
{
struct tree *t;

if(GLOBALS->selected_hierarchy_name)
	{
	gtkwavetcl_setvar(WAVE_TCLCB_TREE_UNSELECT, GLOBALS->selected_hierarchy_name, WAVE_TCLCB_TREE_UNSELECT_FLAGS);
	free_2(GLOBALS->selected_hierarchy_name);
	GLOBALS->selected_hierarchy_name = NULL;
	}

t=(struct tree *)gtk_clist_get_row_data(GTK_CLIST(GLOBALS->ctree_main), row);
DEBUG(printf("TU: %08x %s\n",t,t->name));
 GLOBALS->sig_root_treesearch_gtk2_c_1 = GLOBALS->treeroot;
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
      if(GLOBALS->filter_str_treesearch_gtk2_c_1)
	{
      	free_2((char *)GLOBALS->filter_str_treesearch_gtk2_c_1);
	GLOBALS->filter_str_treesearch_gtk2_c_1 = NULL;
	}
      t = gtk_entry_get_text (GTK_ENTRY (widget));
      if (t == NULL || *t == 0)
	GLOBALS->filter_str_treesearch_gtk2_c_1 = NULL;
      else
	{
	GLOBALS->filter_str_treesearch_gtk2_c_1 = malloc_2(strlen(t) + 1);
	strcpy(GLOBALS->filter_str_treesearch_gtk2_c_1, t);
	wave_regex_compile(GLOBALS->filter_str_treesearch_gtk2_c_1, WAVE_REGEX_TREE);
	}
      fill_sig_store ();
    }
  return FALSE;
}


/*
 * for dynamic updates, simply fake the return key to the function above
 */
static
void press_callback (GtkWidget *widget, gpointer *data)
{
GdkEventKey ev;

ev.keyval = GDK_Return;

filter_edit_cb (widget, &ev, data);
}


/*
 * select/unselect all in treeview
 */
void treeview_select_all_callback(void)
{
GtkTreeSelection* ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(GLOBALS->dnd_sigview));
gtk_tree_selection_select_all(ts);
}

void treeview_unselect_all_callback(void)
{
GtkTreeSelection* ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(GLOBALS->dnd_sigview));
gtk_tree_selection_unselect_all(ts);
}


int treebox_is_active(void)
{
return(GLOBALS->is_active_treesearch_gtk2_c_6);
}

static void enter_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
  G_CONST_RETURN gchar *entry_text;
  int len;
  entry_text = gtk_entry_get_text(GTK_ENTRY(GLOBALS->entry_a_treesearch_gtk2_c_2));
  DEBUG(printf("Entry contents: %s\n", entry_text));
  if(!(len=strlen(entry_text))) GLOBALS->entrybox_text_local_treesearch_gtk2_c_3=NULL;
	else strcpy((GLOBALS->entrybox_text_local_treesearch_gtk2_c_3=(char *)malloc_2(len+1)),entry_text);

  gtk_grab_remove(GLOBALS->window1_treesearch_gtk2_c_3);
  gtk_widget_destroy(GLOBALS->window1_treesearch_gtk2_c_3);
  GLOBALS->window1_treesearch_gtk2_c_3 = NULL;

  GLOBALS->cleanup_e_treesearch_gtk2_c_3();
}

static void destroy_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
  DEBUG(printf("Entry Cancel\n"));
  GLOBALS->entrybox_text_local_treesearch_gtk2_c_3=NULL;
  gtk_grab_remove(GLOBALS->window1_treesearch_gtk2_c_3);
  gtk_widget_destroy(GLOBALS->window1_treesearch_gtk2_c_3);
  GLOBALS->window1_treesearch_gtk2_c_3 = NULL;
}

static void entrybox_local(char *title, int width, char *default_text, int maxch, GtkSignalFunc func)
{
    GtkWidget *vbox, *hbox;
    GtkWidget *button1, *button2;

    GLOBALS->cleanup_e_treesearch_gtk2_c_3=func;

    /* create a new modal window */
    GLOBALS->window1_treesearch_gtk2_c_3 = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window1_treesearch_gtk2_c_3, ((char *)&GLOBALS->window1_treesearch_gtk2_c_3) - ((char *)GLOBALS));

    gtk_widget_set_usize( GTK_WIDGET (GLOBALS->window1_treesearch_gtk2_c_3), width, 60);
    gtk_window_set_title(GTK_WINDOW (GLOBALS->window1_treesearch_gtk2_c_3), title);
    gtkwave_signal_connect(GTK_OBJECT (GLOBALS->window1_treesearch_gtk2_c_3), "delete_event",(GtkSignalFunc) destroy_callback_e, NULL);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (GLOBALS->window1_treesearch_gtk2_c_3), vbox);
    gtk_widget_show (vbox);

    GLOBALS->entry_a_treesearch_gtk2_c_2 = gtk_entry_new_with_max_length (maxch);
    gtkwave_signal_connect(GTK_OBJECT(GLOBALS->entry_a_treesearch_gtk2_c_2), "activate",GTK_SIGNAL_FUNC(enter_callback_e),GLOBALS->entry_a_treesearch_gtk2_c_2);
    gtk_entry_set_text (GTK_ENTRY (GLOBALS->entry_a_treesearch_gtk2_c_2), default_text);
    gtk_entry_select_region (GTK_ENTRY (GLOBALS->entry_a_treesearch_gtk2_c_2),0, GTK_ENTRY(GLOBALS->entry_a_treesearch_gtk2_c_2)->text_length);
    gtk_box_pack_start (GTK_BOX (vbox), GLOBALS->entry_a_treesearch_gtk2_c_2, TRUE, TRUE, 0);
    gtk_widget_show (GLOBALS->entry_a_treesearch_gtk2_c_2);

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

    gtk_widget_show(GLOBALS->window1_treesearch_gtk2_c_3);
    gtk_grab_add(GLOBALS->window1_treesearch_gtk2_c_3);
}

/***************************************************************************/



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
bundle_cleanup_foreach (GtkTreeModel *model,
			GtkTreePath *path,
			GtkTreeIter *iter,
			gpointer data)
{
  struct tree *sel;

  /* Extract the tree.  */
  gtk_tree_model_get (model, iter, TREE_COLUMN, &sel, -1);

  if(!sel) return;

if(GLOBALS->entrybox_text_local_treesearch_gtk2_c_3) 
        {
        char *efix;
 
	if(!strlen(GLOBALS->entrybox_text_local_treesearch_gtk2_c_3))
		{
	        DEBUG(printf("Bundle name is not specified--recursing into hierarchy.\n"));
		fetchvex(sel, GLOBALS->bundle_direction_treesearch_gtk2_c_3);
		}
		else
		{
	        efix=GLOBALS->entrybox_text_local_treesearch_gtk2_c_3;
	        while(*efix)
	                {
	                if(*efix==' ')
	                        {
	                        *efix='_';
	                        }
	                efix++;
	                }
	 
	        DEBUG(printf("Bundle name is: %s\n",entrybox_text_local));
	        add_vector_range(GLOBALS->entrybox_text_local_treesearch_gtk2_c_3, 
				fetchlow(sel)->t_which,
				fetchhigh(sel)->t_which, 
				GLOBALS->bundle_direction_treesearch_gtk2_c_3);
		}
        free_2(GLOBALS->entrybox_text_local_treesearch_gtk2_c_3);
        }
	else
	{
        DEBUG(printf("Bundle name is not specified--recursing into hierarchy.\n"));
	fetchvex(sel, GLOBALS->bundle_direction_treesearch_gtk2_c_3);
	}
}

static void
bundle_cleanup(GtkWidget *widget, gpointer data)
{ 
  gtk_tree_selection_selected_foreach
    (GLOBALS->sig_selection_treesearch_gtk2_c_1, &bundle_cleanup_foreach, NULL);

MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}
 
static void
bundle_callback_generic(void)
{
  if(!GLOBALS->autoname_bundles)
    {
      if (gtk_tree_selection_count_selected_rows (GLOBALS->sig_selection_treesearch_gtk2_c_1) != 1)
	return;
      entrybox_local("Enter Bundle Name",300,"",128,
		     GTK_SIGNAL_FUNC(bundle_cleanup));
    }
  else
    {
      GLOBALS->entrybox_text_local_treesearch_gtk2_c_3=NULL;
      bundle_cleanup(NULL, NULL);
    }
}

static void
bundle_callback_up(GtkWidget *widget, gpointer data)
{
GLOBALS->bundle_direction_treesearch_gtk2_c_3=0;
bundle_callback_generic();
}

static void
bundle_callback_down(GtkWidget *widget, gpointer data)
{
GLOBALS->bundle_direction_treesearch_gtk2_c_3=1;
bundle_callback_generic();
}

/* Callback for insert/replace/append buttions.
   This call-back is called for every signal selected.  */
enum cb_action { ACTION_INSERT, ACTION_REPLACE, ACTION_APPEND, ACTION_PREPEND };



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

  low = fetchlow(sel)->t_which;
  high = fetchhigh(sel)->t_which;

  /* Add signals and vectors.  */
  for(i=low;i<=high;i++)
        {
	int len;
        struct symbol *s, *t;  
        s=GLOBALS->facs[i];
	t=s->vec_root;
	if((t)&&(GLOBALS->autocoalesce))
		{
		if(get_s_selected(t))
			{
			set_s_selected(t,0);
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
	 tfirst=GLOBALS->traces.first; tlast=GLOBALS->traces.last; /* cache for highlighting */
       }

     GLOBALS->traces.buffercount=GLOBALS->traces.total;
     GLOBALS->traces.buffer=GLOBALS->traces.first;
     GLOBALS->traces.bufferlast=GLOBALS->traces.last;
     GLOBALS->traces.first=GLOBALS->tcache_treesearch_gtk2_c_2.first;
     GLOBALS->traces.last=GLOBALS->tcache_treesearch_gtk2_c_2.last;
     GLOBALS->traces.total=GLOBALS->tcache_treesearch_gtk2_c_2.total;

     if(action == ACTION_PREPEND)
	{
	PrependBuffer();
	}
	else
	{
	PasteBuffer();
	}

     GLOBALS->traces.buffercount=GLOBALS->tcache_treesearch_gtk2_c_2.buffercount;
     GLOBALS->traces.buffer=GLOBALS->tcache_treesearch_gtk2_c_2.buffer;
     GLOBALS->traces.bufferlast=GLOBALS->tcache_treesearch_gtk2_c_2.bufferlast;
     
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

  low = fetchlow(sel)->t_which;
  high = fetchhigh(sel)->t_which;

  /* If signals are vectors, coalesces vectors if so.  */
  for(i=low;i<=high;i++)
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
        for(i=low;i<=high;i++)
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
                                                GLOBALS->pre_import_treesearch_gtk2_c_1++;
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
                                GLOBALS->pre_import_treesearch_gtk2_c_1++;
                                }
                        }
                }
        }
  /* LX2 */
}


static void
action_callback(enum cb_action action)
{
  GLOBALS->pre_import_treesearch_gtk2_c_1 = 0;

  /* once through to mass gather lx2 traces... */
  gtk_tree_selection_selected_foreach
    (GLOBALS->sig_selection_treesearch_gtk2_c_1, &sig_selection_foreach_preload_lx2, (void *)action);
  if(GLOBALS->pre_import_treesearch_gtk2_c_1)
	{
        lx2_import_masked();
        }

  /* then do */
  if (action == ACTION_INSERT || action == ACTION_REPLACE || action == ACTION_PREPEND)
    {
      /* Save and clear current traces.  */
      memcpy(&GLOBALS->tcache_treesearch_gtk2_c_2,&GLOBALS->traces,sizeof(Traces));
      GLOBALS->traces.total=0;
      GLOBALS->traces.first=GLOBALS->traces.last=NULL;
    }

  gtk_tree_selection_selected_foreach
    (GLOBALS->sig_selection_treesearch_gtk2_c_1, &sig_selection_foreach, (void *)action);

  sig_selection_foreach_finalize((void *)action);

  if(action == ACTION_APPEND)
	{
	GLOBALS->traces.scroll_top = GLOBALS->traces.scroll_bottom = GLOBALS->traces.last;
	}
  MaxSignalLength();
  signalarea_configure_event(GLOBALS->signalarea, NULL);
  wavearea_configure_event(GLOBALS->wavearea, NULL);
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
  GLOBALS->is_active_treesearch_gtk2_c_6=0;
  gtk_widget_destroy(GLOBALS->window_treesearch_gtk2_c_12);
  GLOBALS->window_treesearch_gtk2_c_12 = NULL;
  GLOBALS->dnd_sigview = NULL;
  GLOBALS->gtk2_tree_frame = NULL;
  GLOBALS->ctree_main = NULL;
  free_afl();

  if(GLOBALS->selected_hierarchy_name)
	{
	free_2(GLOBALS->selected_hierarchy_name);
	GLOBALS->selected_hierarchy_name = NULL;
	}
}

/**********************************************************************/

static gboolean
  view_selection_func (GtkTreeSelection *selection,
                       GtkTreeModel     *model,
                       GtkTreePath      *path,
                       gboolean          path_currently_selected,
                       gpointer          userdata)
  {
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
      gchar *name;

      gtk_tree_model_get(model, &iter, 0, &name, -1);

      if(GLOBALS->selected_sig_name)
	{
	free_2(GLOBALS->selected_sig_name);
	GLOBALS->selected_sig_name = NULL;
	}

      if (!path_currently_selected)
      {
	GLOBALS->selected_sig_name = strdup_2(name);
	gtkwavetcl_setvar(WAVE_TCLCB_TREE_SIG_SELECT, name, WAVE_TCLCB_TREE_SIG_SELECT_FLAGS);
      }
      else
      {
	gtkwavetcl_setvar(WAVE_TCLCB_TREE_SIG_UNSELECT, name, WAVE_TCLCB_TREE_SIG_UNSELECT_FLAGS);
      }

      g_free(name);
    }
    return TRUE; /* allow selection state to change */
  }

/**********************************************************************/

static gint button_press_event_std(GtkWidget *widget, GdkEventButton *event)
{
if(event->type == GDK_2BUTTON_PRESS)
	{
	if(GLOBALS->selected_hierarchy_name && GLOBALS->selected_sig_name)
		{
		char *sstr = wave_alloca(strlen(GLOBALS->selected_hierarchy_name) + strlen(GLOBALS->selected_sig_name) + 1);
		strcpy(sstr, GLOBALS->selected_hierarchy_name);
		strcat(sstr, GLOBALS->selected_sig_name);

		gtkwavetcl_setvar(WAVE_TCLCB_TREE_SIG_DOUBLE_CLICK, sstr, WAVE_TCLCB_TREE_SIG_DOUBLE_CLICK_FLAGS);
		}
	}

return(FALSE);
}

/**********************************************************************/

/*
 * mainline..
 */
void treebox(char *title, GtkSignalFunc func, GtkWidget *old_window)
{
    GtkWidget *scrolled_win, *sig_scroll_win;
    GtkWidget *hbox;
    GtkWidget *button1, *button2, *button3, *button3a, *button4, *button5;
    GtkWidget *frameh, *sig_frame;
    GtkWidget *vbox, *vpan, *filter_hbox;
    GtkWidget *filter_label;
    GtkWidget *sig_view;
    GtkTooltips *tooltips;
    GtkCList  *clist;

    if(old_window)
	{
	GLOBALS->is_active_treesearch_gtk2_c_6=1;
    	GLOBALS->cleanup_treesearch_gtk2_c_8=func;
	goto do_tooltips;
	}

    if(GLOBALS->is_active_treesearch_gtk2_c_6) 
	{
	if(GLOBALS->window_treesearch_gtk2_c_12) 
		{
		gdk_window_raise(GLOBALS->window_treesearch_gtk2_c_12->window);
		}
		else
		{
#if GTK_CHECK_VERSION(2,4,0)
		if(GLOBALS->expanderwindow)
			{
			gtk_expander_set_expanded(GTK_EXPANDER(GLOBALS->expanderwindow), TRUE);
			}
#endif
		}
	return;
	}

    GLOBALS->is_active_treesearch_gtk2_c_6=1;
    GLOBALS->cleanup_treesearch_gtk2_c_8=func;

    /* create a new modal window */
    GLOBALS->window_treesearch_gtk2_c_12 = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window_treesearch_gtk2_c_12, ((char *)&GLOBALS->window_treesearch_gtk2_c_12) - ((char *)GLOBALS));

    gtk_window_set_title(GTK_WINDOW (GLOBALS->window_treesearch_gtk2_c_12), title);
    gtkwave_signal_connect(GTK_OBJECT (GLOBALS->window_treesearch_gtk2_c_12), "delete_event",(GtkSignalFunc) destroy_callback, NULL);

do_tooltips:
    tooltips=gtk_tooltips_new_2();

    GLOBALS->treesearch_gtk2_window_vbox = vbox = gtk_vbox_new (FALSE, 1);
    gtk_widget_show (vbox);

    vpan = gtk_vpaned_new ();
    gtk_widget_show (vpan);
    gtk_box_pack_start (GTK_BOX (vbox), vpan, TRUE, TRUE, 1);

    /* Hierarchy.  */
    GLOBALS->gtk2_tree_frame = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (GLOBALS->gtk2_tree_frame), 3);
    gtk_widget_show(GLOBALS->gtk2_tree_frame);

    gtk_paned_pack1 (GTK_PANED (vpan), GLOBALS->gtk2_tree_frame, TRUE, FALSE);

    GLOBALS->tree_treesearch_gtk2_c_1=gtk_ctree_new(1,0);
    GLOBALS->ctree_main=GTK_CTREE(GLOBALS->tree_treesearch_gtk2_c_1);
    gtk_clist_set_column_auto_resize (GTK_CLIST (GLOBALS->tree_treesearch_gtk2_c_1), 0, TRUE);
    gtk_widget_show(GLOBALS->tree_treesearch_gtk2_c_1);

    gtk_clist_set_use_drag_icons(GTK_CLIST (GLOBALS->tree_treesearch_gtk2_c_1), FALSE);

    clist=GTK_CLIST(GLOBALS->tree_treesearch_gtk2_c_1);

    gtkwave_signal_connect_object (GTK_OBJECT (clist), "select_row", GTK_SIGNAL_FUNC(select_row_callback), NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "unselect_row", GTK_SIGNAL_FUNC(unselect_row_callback), NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "tree_expand", GTK_SIGNAL_FUNC(tree_expand_callback), NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "tree_collapse", GTK_SIGNAL_FUNC(tree_collapse_callback), NULL);

    gtk_clist_freeze(clist);
    gtk_clist_clear(clist);

    maketree(NULL, GLOBALS->treeroot);
    gtk_clist_thaw(clist);

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize( GTK_WIDGET (scrolled_win), -1, 50);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_win);
    gtk_container_add (GTK_CONTAINER (scrolled_win), GTK_WIDGET (GLOBALS->tree_treesearch_gtk2_c_1));
    gtk_container_add (GTK_CONTAINER (GLOBALS->gtk2_tree_frame), scrolled_win);


    /* Signal names.  */
    GLOBALS->sig_store_treesearch_gtk2_c_1 = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING);
    GLOBALS->sig_root_treesearch_gtk2_c_1 = GLOBALS->treeroot;
    fill_sig_store ();

    sig_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (GLOBALS->sig_store_treesearch_gtk2_c_1));

    /* The view now holds a reference.  We can get rid of our own reference */
    g_object_unref (G_OBJECT (GLOBALS->sig_store_treesearch_gtk2_c_1));


      {
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new ();

	switch(GLOBALS->loaded_file_type)
		{
		case FST_FILE:
		case VCD_FILE:
		case VCD_RECODER_FILE:
		case DUMPLESS_FILE:
					column = gtk_tree_view_column_new_with_attributes ("Type",
							   renderer,
							   "text", TYPE_COLUMN,
							   NULL);
					gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);
					break;
		default:
			 		break;
		}

	column = gtk_tree_view_column_new_with_attributes ("Signals",
							   renderer,
							   "text", NAME_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);


	/* Setup the selection handler */
	GLOBALS->sig_selection_treesearch_gtk2_c_1 = gtk_tree_view_get_selection (GTK_TREE_VIEW (sig_view));
	gtk_tree_selection_set_mode (GLOBALS->sig_selection_treesearch_gtk2_c_1, GTK_SELECTION_MULTIPLE);
        gtk_tree_selection_set_select_function (GLOBALS->sig_selection_treesearch_gtk2_c_1,
                                                view_selection_func, NULL, NULL);

	gtkwave_signal_connect(GTK_OBJECT(sig_view), "button_press_event",GTK_SIGNAL_FUNC(button_press_event_std), NULL);
      }

    GLOBALS->dnd_sigview = sig_view;
    dnd_setup(GLOBALS->dnd_sigview, GLOBALS->signalarea, 0);

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

    GLOBALS->filter_entry = gtk_entry_new ();
    if(GLOBALS->filter_str_treesearch_gtk2_c_1) 
	{ 
	gtk_entry_set_text(GTK_ENTRY(GLOBALS->filter_entry), GLOBALS->filter_str_treesearch_gtk2_c_1); 
	}

    gtk_widget_show (GLOBALS->filter_entry);

    gtkwave_signal_connect(GTK_OBJECT(GLOBALS->filter_entry), "activate", GTK_SIGNAL_FUNC(press_callback), NULL);
    if(!GLOBALS->do_dynamic_treefilter)
	{
    	gtkwave_signal_connect(GTK_OBJECT (GLOBALS->filter_entry), "key_press_event", (GtkSignalFunc) filter_edit_cb, NULL);
	gtk_tooltips_set_tip_2(tooltips, GLOBALS->filter_entry,
			   "Add a POSIX filter. "
			   "'.*' matches any number of characters,"
			   " '.' matches any character.  Hit Return to apply.",
			   NULL);
	}
	else
	{
    	gtkwave_signal_connect(GTK_OBJECT(GLOBALS->filter_entry), "changed", GTK_SIGNAL_FUNC(press_callback), NULL);
	gtk_tooltips_set_tip_2(tooltips, GLOBALS->filter_entry,
			   "Add a POSIX filter. "
			   "'.*' matches any number of characters,"
			   " '.' matches any character.",
			   NULL);
	}


    gtk_box_pack_start (GTK_BOX (filter_hbox), GLOBALS->filter_entry, FALSE, FALSE, 1);

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
    gtkwave_signal_connect_object (GTK_OBJECT (button1), "clicked",GTK_SIGNAL_FUNC(ok_callback),GTK_OBJECT (GLOBALS->window_treesearch_gtk2_c_12));
    gtk_widget_show (button1);
    gtk_tooltips_set_tip_2(tooltips, button1, 
		"Add selected signal hierarchy to end of the display on the main window.",NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button1, TRUE, FALSE, 0);

    button2 = gtk_button_new_with_label (" Insert ");
    gtk_container_border_width (GTK_CONTAINER (button2), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button2), "clicked",GTK_SIGNAL_FUNC(insert_callback),GTK_OBJECT (GLOBALS->window_treesearch_gtk2_c_12));
    gtk_widget_show (button2);
    gtk_tooltips_set_tip_2(tooltips, button2, 
		"Add selected signal hierarchy after last highlighted signal on the main window.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button2, TRUE, FALSE, 0);

    if(GLOBALS->vcd_explicit_zero_subscripts>=0)
	{
    	button3 = gtk_button_new_with_label (" Bundle Up ");
    	gtk_container_border_width (GTK_CONTAINER (button3), 3);
    	gtkwave_signal_connect_object (GTK_OBJECT (button3), "clicked",GTK_SIGNAL_FUNC(bundle_callback_up),GTK_OBJECT (GLOBALS->window_treesearch_gtk2_c_12));
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
    	gtkwave_signal_connect_object (GTK_OBJECT (button3a), "clicked",GTK_SIGNAL_FUNC(bundle_callback_down),GTK_OBJECT (GLOBALS->window_treesearch_gtk2_c_12));
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
    gtkwave_signal_connect_object (GTK_OBJECT (button4), "clicked",GTK_SIGNAL_FUNC(replace_callback),GTK_OBJECT (GLOBALS->window_treesearch_gtk2_c_12));
    gtk_widget_show (button4);
    gtk_tooltips_set_tip_2(tooltips, button4, 
		"Replace highlighted signals on the main window with signals selected above.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button4, TRUE, FALSE, 0);

    button5 = gtk_button_new_with_label (" Exit ");
    gtk_container_border_width (GTK_CONTAINER (button5), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button5), "clicked",GTK_SIGNAL_FUNC(destroy_callback),GTK_OBJECT (GLOBALS->window_treesearch_gtk2_c_12));
    gtk_tooltips_set_tip_2(tooltips, button5, 
		"Do nothing and return to the main window.",NULL);
    gtk_widget_show (button5);
    gtk_box_pack_start (GTK_BOX (hbox), button5, TRUE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (frameh), hbox);
    gtk_container_add (GTK_CONTAINER (GLOBALS->window_treesearch_gtk2_c_12), vbox);

    gtk_window_set_default_size (GTK_WINDOW (GLOBALS->window_treesearch_gtk2_c_12), 200, 400);
    gtk_widget_show(GLOBALS->window_treesearch_gtk2_c_12);
}


/*
 * for use with expander in gtk2.4 and higher...
 */
GtkWidget* treeboxframe(char *title, GtkSignalFunc func)
{
    GtkWidget *scrolled_win, *sig_scroll_win;
    GtkWidget *hbox;
    GtkWidget *button1, *button2, *button3, *button3a, *button4;
    GtkWidget *frameh, *sig_frame;
    GtkWidget *vbox, *vpan, *filter_hbox;
    GtkWidget *filter_label;
    GtkWidget *sig_view;
    GtkTooltips *tooltips;
    GtkCList  *clist;

    GLOBALS->is_active_treesearch_gtk2_c_6=1;
    GLOBALS->cleanup_treesearch_gtk2_c_8=func;

    /* create a new modal window */
    tooltips=gtk_tooltips_new_2();

    vbox = gtk_vbox_new (FALSE, 1);
    gtk_widget_show (vbox);

    vpan = gtk_vpaned_new ();
    gtk_widget_show (vpan);
    gtk_box_pack_start (GTK_BOX (vbox), vpan, TRUE, TRUE, 1);

    /* Hierarchy.  */
    GLOBALS->gtk2_tree_frame = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (GLOBALS->gtk2_tree_frame), 3);
    gtk_widget_show(GLOBALS->gtk2_tree_frame);

    gtk_paned_pack1 (GTK_PANED (vpan), GLOBALS->gtk2_tree_frame, TRUE, FALSE);

    GLOBALS->tree_treesearch_gtk2_c_1=gtk_ctree_new(1,0);
    GLOBALS->ctree_main=GTK_CTREE(GLOBALS->tree_treesearch_gtk2_c_1);
    gtk_clist_set_column_auto_resize (GTK_CLIST (GLOBALS->tree_treesearch_gtk2_c_1), 0, TRUE);
    gtk_widget_show(GLOBALS->tree_treesearch_gtk2_c_1);

    clist=GTK_CLIST(GLOBALS->tree_treesearch_gtk2_c_1);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "select_row", GTK_SIGNAL_FUNC(select_row_callback), NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "unselect_row", GTK_SIGNAL_FUNC(unselect_row_callback), NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "tree_expand", GTK_SIGNAL_FUNC(tree_expand_callback), NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "tree_collapse", GTK_SIGNAL_FUNC(tree_collapse_callback), NULL);

    gtk_clist_freeze(clist);
    gtk_clist_clear(clist);

    maketree(NULL, GLOBALS->treeroot);
    gtk_clist_thaw(clist);

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize( GTK_WIDGET (scrolled_win), -1, 50);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_win);
    gtk_container_add (GTK_CONTAINER (scrolled_win), GTK_WIDGET (GLOBALS->tree_treesearch_gtk2_c_1));
    gtk_container_add (GTK_CONTAINER (GLOBALS->gtk2_tree_frame), scrolled_win);


    /* Signal names.  */
    GLOBALS->sig_store_treesearch_gtk2_c_1 = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING);
    GLOBALS->sig_root_treesearch_gtk2_c_1 = GLOBALS->treeroot;
    fill_sig_store ();

    sig_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (GLOBALS->sig_store_treesearch_gtk2_c_1));

    /* The view now holds a reference.  We can get rid of our own reference */
    g_object_unref (G_OBJECT (GLOBALS->sig_store_treesearch_gtk2_c_1));


      {
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new ();

	switch(GLOBALS->loaded_file_type)
		{
		case FST_FILE:
		case VCD_FILE:
		case VCD_RECODER_FILE:
		case DUMPLESS_FILE:
					column = gtk_tree_view_column_new_with_attributes ("Type",
							   renderer,
							   "text", TYPE_COLUMN,
							   NULL);
					gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);
					break;
		default:
					break;
		}

	column = gtk_tree_view_column_new_with_attributes ("Signals",
							   renderer,
							   "text", NAME_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);

	/* Setup the selection handler */
	GLOBALS->sig_selection_treesearch_gtk2_c_1 = gtk_tree_view_get_selection (GTK_TREE_VIEW (sig_view));
	gtk_tree_selection_set_mode (GLOBALS->sig_selection_treesearch_gtk2_c_1, GTK_SELECTION_MULTIPLE);
        gtk_tree_selection_set_select_function (GLOBALS->sig_selection_treesearch_gtk2_c_1,
                                                view_selection_func, NULL, NULL);

	gtkwave_signal_connect(GTK_OBJECT(sig_view), "button_press_event",GTK_SIGNAL_FUNC(button_press_event_std), NULL);
      }

    GLOBALS->dnd_sigview = sig_view;

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

    GLOBALS->filter_entry = gtk_entry_new ();
    if(GLOBALS->filter_str_treesearch_gtk2_c_1) { gtk_entry_set_text(GTK_ENTRY(GLOBALS->filter_entry), GLOBALS->filter_str_treesearch_gtk2_c_1); }
    gtk_widget_show (GLOBALS->filter_entry);

    gtkwave_signal_connect(GTK_OBJECT(GLOBALS->filter_entry), "activate", GTK_SIGNAL_FUNC(press_callback), NULL);
    if(!GLOBALS->do_dynamic_treefilter)
	{
    	gtkwave_signal_connect(GTK_OBJECT (GLOBALS->filter_entry), "key_press_event", (GtkSignalFunc) filter_edit_cb, NULL);
	gtk_tooltips_set_tip_2(tooltips, GLOBALS->filter_entry,
			   "Add a POSIX filter. "
			   "'.*' matches any number of characters,"
			   " '.' matches any character.  Hit Return to apply.",
			   NULL);
	}
	else
	{
	gtkwave_signal_connect(GTK_OBJECT(GLOBALS->filter_entry), "changed", GTK_SIGNAL_FUNC(press_callback), NULL);
	gtk_tooltips_set_tip_2(tooltips, GLOBALS->filter_entry,
			   "Add a POSIX filter. "
			   "'.*' matches any number of characters,"
			   " '.' matches any character.",
			   NULL);
	}

    gtk_box_pack_start (GTK_BOX (filter_hbox), GLOBALS->filter_entry, FALSE, FALSE, 1);

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
    gtkwave_signal_connect_object (GTK_OBJECT (button1), "clicked", GTK_SIGNAL_FUNC(ok_callback), GTK_OBJECT (GLOBALS->gtk2_tree_frame));
    gtk_widget_show (button1);
    gtk_tooltips_set_tip_2(tooltips, button1, 
		"Add selected signal hierarchy to end of the display on the main window.",NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button1, TRUE, FALSE, 0);

    button2 = gtk_button_new_with_label (" Insert ");
    gtk_container_border_width (GTK_CONTAINER (button2), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button2), "clicked", GTK_SIGNAL_FUNC(insert_callback), GTK_OBJECT (GLOBALS->gtk2_tree_frame));
    gtk_widget_show (button2);
    gtk_tooltips_set_tip_2(tooltips, button2, 
		"Add selected signal hierarchy after last highlighted signal on the main window.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button2, TRUE, FALSE, 0);

    if(GLOBALS->vcd_explicit_zero_subscripts>=0)
	{
    	button3 = gtk_button_new_with_label (" Bundle Up ");
    	gtk_container_border_width (GTK_CONTAINER (button3), 3);
    	gtkwave_signal_connect_object (GTK_OBJECT (button3), "clicked", GTK_SIGNAL_FUNC(bundle_callback_up), GTK_OBJECT (GLOBALS->gtk2_tree_frame));
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
    	gtkwave_signal_connect_object (GTK_OBJECT (button3a), "clicked", GTK_SIGNAL_FUNC(bundle_callback_down), GTK_OBJECT (GLOBALS->gtk2_tree_frame));
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
    gtkwave_signal_connect_object (GTK_OBJECT (button4), "clicked", GTK_SIGNAL_FUNC(replace_callback), GTK_OBJECT (GLOBALS->gtk2_tree_frame));
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
	if(!GLOBALS->dnd_state)
		{
		if(widget == GLOBALS->clist_search_c_3)
			{
			GLOBALS->tree_dnd_begin = SEARCH_TO_VIEW_DRAG_ACTIVE;
			}
			else
			{
			GLOBALS->tree_dnd_begin = TREE_TO_VIEW_DRAG_ACTIVE;
			}
		}
}

/*
 *      DND "drag_end" handler, this is called when a drag and drop has
 *	completed. So this function is the last one to be called in
 *	any given DND operation.
 */
static void DNDEndCB_2(
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

/* Put any needed drag end cleanup code here. */

if(GLOBALS->dnd_tgt_on_signalarea_treesearch_gtk2_c_1)
	{
	WAVE_GDK_GET_POINTER(GLOBALS->signalarea->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;

	if((x<0)||(y<0)||(x>=GLOBALS->signalarea->allocation.width)||(y>=GLOBALS->signalarea->allocation.height)) return;
	}
else
if(GLOBALS->dnd_tgt_on_wavearea_treesearch_gtk2_c_1)
	{
	WAVE_GDK_GET_POINTER(GLOBALS->wavearea->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;

	if((x<0)||(y<0)||(x>=GLOBALS->wavearea->allocation.width)||(y>=GLOBALS->wavearea->allocation.height)) return;
	}
else
	{
	return;
	}


if((t=GLOBALS->traces.first))
        {       
        while(t)
                {
                t->flags&=~TR_HIGHLIGHT;
                t=t->t_next;
                }
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);
	}

trtarget = ((int)y / (int)GLOBALS->fontheight) - 2; 
if(trtarget < 0) 
	{
	Trptr tp = GLOBALS->topmost_trace ? GivePrevTrace(GLOBALS->topmost_trace): NULL;
	trtarget = 0;

	if(tp)
		{
		t = tp;
		}
		else
		{
		if(GLOBALS->tree_dnd_begin == SEARCH_TO_VIEW_DRAG_ACTIVE)
			{
			if(GLOBALS->window_search_c_7)
				{
				search_insert_callback(GLOBALS->window_search_c_7, 1 /* is prepend */);
				}
			}
			else
			{
			action_callback(ACTION_PREPEND);  /* prepend in this widget only ever used by this function call */
			}
		goto dnd_import_fini;
		}
	}
	else
	{
	t=GLOBALS->topmost_trace;
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

if(GLOBALS->tree_dnd_begin == SEARCH_TO_VIEW_DRAG_ACTIVE)
	{
	if(GLOBALS->window_search_c_7)
		{
		search_insert_callback(GLOBALS->window_search_c_7, 0 /* is insert */);
		}
	}
	else
	{
	action_callback (ACTION_INSERT);
	}

if(t)
	{
	t->flags &= ~TR_HIGHLIGHT;
	}

dnd_import_fini:

MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}


static void DNDEndCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
if((widget == NULL) || (dc == NULL)) 
	{
	GLOBALS->tree_dnd_begin = VIEW_DRAG_INACTIVE;
	return;
	}

if(GLOBALS->tree_dnd_begin == VIEW_DRAG_INACTIVE) 
	{
	return; /* to keep cut and paste in signalwindow from conflicting */
	}

if(GLOBALS->tree_dnd_requested)
	{
	GLOBALS->tree_dnd_requested = 0;

	if(GLOBALS->is_lx2 == LXT2_IS_VLIST)
		{
		set_window_busy(NULL);
		}
	DNDEndCB_2(widget, dc, data);
	if(GLOBALS->is_lx2 == LXT2_IS_VLIST)
		{
		set_window_idle(NULL);
		}
	}

GLOBALS->tree_dnd_begin = VIEW_DRAG_INACTIVE;
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
		{
                gdk_drag_status(dc, 0, t);
                return(FALSE);
		}

	/* Get source widget and target widget. */
	src_widget = gtk_drag_get_source_widget(dc);
	tar_widget = widget;

	/* Note if source widget is the same as the target. */
	same_widget = (src_widget == tar_widget) ? TRUE : FALSE;

	GLOBALS->dnd_tgt_on_signalarea_treesearch_gtk2_c_1 = (tar_widget == GLOBALS->signalarea);
	GLOBALS->dnd_tgt_on_wavearea_treesearch_gtk2_c_1 = (tar_widget == GLOBALS->wavearea);

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
int upd = 0;
GLOBALS->tree_dnd_requested = 1;  /* indicate that a request for data occurred... */

if(widget == GLOBALS->clist_search_c_3)	/* from search */
	{
	char *text = add_dnd_from_searchbox();
	if(text)
		{
		gtk_selection_data_set(selection_data,GDK_SELECTION_TYPE_STRING, 8, (guchar*)text, strlen(text));
		free_2(text);
		}
	upd = 1;
	}
else if(widget == GLOBALS->signalarea)
	{
	char *text = add_dnd_from_signal_window();
	if(text)
		{
		char *text2 = emit_gtkwave_savefile_formatted_entries_in_tcl_list(GLOBALS->traces.first, TRUE);
		if(text2)
			{
			int textlen = strlen(text);
			int text2len = strlen(text2);
			char *pnt = calloc_2(1, textlen + text2len + 1);
			
			memcpy(pnt, text, textlen);
			memcpy(pnt + textlen, text2, text2len);

			free_2(text2);
			free_2(text);
			text = pnt;
			}

		gtk_selection_data_set(selection_data,GDK_SELECTION_TYPE_STRING, 8, (guchar*)text, strlen(text));
		free_2(text);
		}
	upd = 1;
	}
else if(widget == GLOBALS->dnd_sigview)
	{
	char *text = add_dnd_from_tree_window();
	if(text)
		{
		gtk_selection_data_set(selection_data,GDK_SELECTION_TYPE_STRING, 8, (guchar*)text, strlen(text));
		free_2(text);
		}
	upd = 1;
	}

if(upd)
	{
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}

/*
 *      DND "drag_data_received" handler. When DNDDataRequestCB()
 *	calls gtk_selection_data_set() to send out the data, this function
 *	receives it and is responsible for handling it.
 *
 *	This is also the only DND callback function where the given
 *	inputs may reflect those of the drop target so we need to check
 *	if this is the same structure or not.
 */
static void DNDDataReceivedCB(
	GtkWidget *widget, GdkDragContext *dc,
	gint x, gint y, GtkSelectionData *selection_data,
	guint info, guint t, gpointer data) {
    gboolean same;
    GtkWidget *source_widget;

    if((widget == NULL) || (data == NULL) || (dc == NULL)) return;

    /* Important, check if we actually got data.  Sometimes errors
     * occure and selection_data will be NULL.
     */
    if(selection_data == NULL)     return;
    if(selection_data->length < 0) return;

    /* Source and target widgets are the same? */
    source_widget = gtk_drag_get_source_widget(dc);
    same = (source_widget == widget) ? TRUE : FALSE;

    if(source_widget)
    if((source_widget == GLOBALS->clist_search_c_3) || /* from search */
       (source_widget == GLOBALS->signalarea) ||
       (source_widget == GLOBALS->dnd_sigview))
		{
		/* use internal mechanism instead of passing names around... */
		return;
		}

    GLOBALS->dnd_state = 0;
    GLOBALS->tree_dnd_requested = 0;

    /* Now check if the data format type is one that we support
     * (remember, data format type, not data type).
     *
     * We check this by testing if info matches one of the info
     * values that we have defined.
     *
     * Note that we can also iterate through the atoms in:
     *	GList *glist = dc->targets;
     *
     *	while(glist != NULL)
     *	{
     *	    gchar *name = gdk_atom_name((GdkAtom)glist->data);
     *	     * strcmp the name to see if it matches
     *	     * one that we support
     *	     *
     *	    glist = glist->next;
     *	}
     */
    if((info == WAVE_DRAG_TAR_INFO_0) ||
       (info == WAVE_DRAG_TAR_INFO_1) ||
       (info == WAVE_DRAG_TAR_INFO_2))
    {
    int num_found ;

    /* printf("XXX %08x '%s'\n", selection_data->data, selection_data->data); */
    if(!(num_found = process_url_list((char *)selection_data->data)))
	{
    	num_found = process_tcl_list((char *)selection_data->data, TRUE);
	}
	
    if(num_found)
	{
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
    }
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
		gtk_drag_dest_set(
			w,
			GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT |
			GTK_DEST_DEFAULT_DROP,
			target_entry,
			sizeof(target_entry) / sizeof(GtkTargetEntry),
			GDK_ACTION_MOVE | GDK_ACTION_COPY
		);
		gtkwave_signal_connect(GTK_OBJECT(w), "drag_motion", GTK_SIGNAL_FUNC(DNDDragMotionCB), win);

		/* Set the drag source for this widget, allowing the user
		 * to drag items off of this clist.
		 */
		if(src)
			{
			gtk_drag_source_set(
				src,
				GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
	                        target_entry,
	                        sizeof(target_entry) / sizeof(GtkTargetEntry),
				GDK_ACTION_MOVE | GDK_ACTION_COPY
			);
			/* Set DND signals on clist. */
			gtkwave_signal_connect(GTK_OBJECT(src), "drag_begin", GTK_SIGNAL_FUNC(DNDBeginCB), win);
	                gtkwave_signal_connect(GTK_OBJECT(src), "drag_end", GTK_SIGNAL_FUNC(DNDEndCB), win);
	                gtkwave_signal_connect(GTK_OBJECT(src), "drag_data_get", GTK_SIGNAL_FUNC(DNDDataRequestCB), win);
	                gtkwave_signal_connect(GTK_OBJECT(src), "drag_data_delete", GTK_SIGNAL_FUNC(DNDDataDeleteCB), win);
			}

                if(enable_receive) gtkwave_signal_connect(GTK_OBJECT(w),   "drag_data_received", GTK_SIGNAL_FUNC(DNDDataReceivedCB), win);
	}
}

/*
 * $Id$
 * $Log$
 * Revision 1.51  2011/01/17 19:24:21  gtkwave
 * tree modifications to support decorated internal hierarchy nodes
 *
 * Revision 1.50  2010/09/27 18:54:31  gtkwave
 * add check for null node in dynamic sst creation
 *
 * Revision 1.49  2010/09/27 18:02:35  gtkwave
 * force open tree node fix for dynamic trees
 *
 * Revision 1.48  2010/09/23 22:04:55  gtkwave
 * added incremental SST build code
 *
 * Revision 1.47  2010/07/29 20:54:29  gtkwave
 * add cbTreeExpand and cbTreeCollapse
 *
 * Revision 1.46  2010/07/28 19:18:26  gtkwave
 * added double-click trapping via gtkwave::cbTreeSigDoubleClick
 *
 * Revision 1.45  2010/07/27 19:25:41  gtkwave
 * initial tcl callback function adds
 *
 * Revision 1.44  2010/05/27 06:07:25  gtkwave
 * Moved gtk_grab_add() after gtk_widget_show() as newer gtk needs that order.
 *
 * Revision 1.43  2010/03/18 17:12:37  gtkwave
 * pedantic warning cleanups
 *
 * Revision 1.42  2010/03/16 21:01:12  gtkwave
 * remove selected member of struct symbol
 *
 * Revision 1.41  2009/09/28 05:58:05  gtkwave
 * changes to support signal_change_list
 *
 * Revision 1.40  2009/09/20 21:45:50  gtkwave
 * tree force open node handling changed for tcl
 *
 * Revision 1.39  2009/07/23 20:00:55  gtkwave
 * added type info to signal window mouseover
 *
 * Revision 1.38  2009/07/06 21:41:36  gtkwave
 * evcd support issues
 *
 * Revision 1.37  2009/06/29 18:16:23  gtkwave
 * adding framework for module type annotation on inner tree nodes
 *
 * Revision 1.36  2009/06/27 23:10:32  gtkwave
 * added display of type info for variables in tree view
 *
 * Revision 1.35  2009/01/21 02:24:15  gtkwave
 * gtk1 compile fixes, ensure ctree_main is available for force_open_tree_node
 *
 * Revision 1.34  2008/12/25 04:07:29  gtkwave
 * -Wshadow warning fixes
 *
 * Revision 1.33  2008/12/18 01:31:30  gtkwave
 * integrated experimental autoscroll code on signal adds
 *
 * Revision 1.32  2008/12/17 16:22:31  gtkwave
 * removed clearing of dnd timer in tree drag code
 *
 * Revision 1.31  2008/12/16 18:09:49  gtkwave
 * add countdown timer to remove dnd cursor in signalwindow
 *
 * Revision 1.30  2008/12/04 17:22:33  gtkwave
 * fix crash on invalid node for force tree open
 *
 * Revision 1.29  2008/11/25 18:07:32  gtkwave
 * added cut copy paste functionality that survives reload and can do
 * multiple pastes on the same cut buffer
 *
 * Revision 1.28  2008/10/02 00:52:25  gtkwave
 * added dnd of external filetypes into viewer
 *
 * Revision 1.27  2008/10/01 01:31:38  gtkwave
 * more dnd fixes
 *
 * Revision 1.26  2008/09/29 22:46:39  gtkwave
 * complex dnd handling with gtkwave trace attributes
 *
 * Revision 1.25  2008/09/27 19:08:39  gtkwave
 * compiler warning fixes
 *
 * Revision 1.24  2008/09/27 06:26:35  gtkwave
 * twinwave (XEmbed) fixes for self-dnd in signal window
 *
 * Revision 1.23  2008/09/25 01:41:36  gtkwave
 * drag from tree clist window into external process
 *
 * Revision 1.22  2008/09/24 23:41:25  gtkwave
 * drag from signal window into external process
 *
 * Revision 1.21  2008/09/24 18:54:00  gtkwave
 * drag from search widget into external processes
 *
 * Revision 1.20  2008/09/24 02:22:49  gtkwave
 * added (commented out) prelim support for dragging names from viewer
 *
 * Revision 1.19  2008/09/23 18:23:03  gtkwave
 * adding more support for DnD from external apps
 *
 * Revision 1.18  2008/09/22 03:27:03  gtkwave
 * more DND adds
 *
 * Revision 1.17  2008/09/16 00:01:27  gtkwave
 * prelim drag and drop from external apps (now disabled)
 *
 * Revision 1.16  2008/08/18 16:10:54  gtkwave
 * adding sticky click semantics on already selected entries
 *
 * Revision 1.15  2008/05/11 03:05:25  gtkwave
 * add gating flag on dnd data request so moving in/out of search window
 * doesn't cause spurious insert ops into the signal/wavewindow
 *
 * Revision 1.14  2008/05/06 19:37:55  gtkwave
 * DND tweaking for drag status
 *
 * Revision 1.13  2008/05/05 19:27:06  gtkwave
 * support for DND from regex search window to sig/waveareas
 *
 * Revision 1.12  2008/02/12 23:35:42  gtkwave
 * preparing for 3.1.5 revision bump
 *
 * Revision 1.11  2008/01/13 19:16:54  gtkwave
 * add busy on vcd_recoder dnd (not normally activated)
 *
 * Revision 1.10  2008/01/08 04:01:12  gtkwave
 * more accelerator key ergonomic updates
 *
 * Revision 1.9  2008/01/05 22:25:46  gtkwave
 * degate busy during treeview dnd as it disrupts focus; dnd cleanups
 *
 * Revision 1.8  2008/01/03 21:55:45  gtkwave
 * various cleanups
 *
 * Revision 1.7  2008/01/03 05:02:14  gtkwave
 * added dnd into wavewindow for both click modes
 *
 * Revision 1.6  2008/01/03 00:09:17  gtkwave
 * preliminary dnd support for use_standard_clicking mode
 *
 * Revision 1.5  2007/12/29 20:19:33  gtkwave
 * added dynamic string updates for entrybox in pattern search and sst
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
 * Revision 1.1.1.1.2.15  2007/08/26 04:56:10  gtkwave
 * remove unused var in expand/collapse
 *
 * Revision 1.1.1.1.2.14  2007/08/26 04:50:04  gtkwave
 * tree clist freeze to remove visual noise
 *
 * Revision 1.1.1.1.2.13  2007/08/26 04:42:59  gtkwave
 * mangled tree fix
 *
 * Revision 1.1.1.1.2.12  2007/08/25 19:43:46  gtkwave
 * header cleanups
 *
 * Revision 1.1.1.1.2.11  2007/08/22 02:06:39  gtkwave
 * merge in treebox() similar to treeboxframe()
 *
 * Revision 1.1.1.1.2.10  2007/08/21 23:29:17  gtkwave
 * merge in tree select state from old ctx
 *
 * Revision 1.1.1.1.2.9  2007/08/21 22:35:40  gtkwave
 * prelim tree state merge
 *
 * Revision 1.1.1.1.2.8  2007/08/18 21:51:57  gtkwave
 * widget destroys and teardown of file formats which use external loaders
 * and are outside of malloc_2/free_2 control
 *
 * Revision 1.1.1.1.2.7  2007/08/07 04:54:59  gtkwave
 * slight modifications to global initialization scheme
 *
 * Revision 1.1.1.1.2.6  2007/08/07 03:18:55  kermin
 * Changed to pointer based GLOBAL structure and added initialization function
 *
 * Revision 1.1.1.1.2.5  2007/08/06 03:50:50  gtkwave
 * globals support for ae2, gtk1, cygwin, mingw.  also cleaned up some machine
 * generated structs, etc.
 *
 * Revision 1.1.1.1.2.4  2007/08/05 02:27:27  kermin
 * Semi working global struct
 *
 * Revision 1.1.1.1.2.3  2007/07/31 03:18:02  kermin
 * Merge Complete - I hope
 *
 * Revision 1.1.1.1.2.2  2007/07/28 19:50:40  kermin
 * Merged in the main line
 *
 * Revision 1.1.1.1  2007/05/30 04:27:55  gtkwave
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
