#include"globals.h"/* 
 * Copyright (c) Tony Bybell 1999-2007.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <config.h>
#include <ctype.h>
#include "gtk12compat.h"
#include "strace.h"
#include "currenttime.h"









/*
 * naive nonoptimized case insensitive strstr
 */
char *strstr_i(char *hay, char *needle)
{
char *ht, *nt;

while (*hay)
	{
	int offs = 0;
	ht = hay; nt = needle;
	while(*nt)
		{
		if(toupper(*(ht+offs)) != toupper(*nt)) break;
		offs++;
		nt++;
		}

	if(!*nt) return(hay);
	hay++;
	}

return(*hay ? hay : NULL);
}

/*
 * trap timescale overflows
 */
static TimeType adjust(TimeType a, TimeType b)
{
TimeType res=a+b;

if((b>LLDescriptor(0))&&(res<a))
	{
	return(MAX_HISTENT_TIME);
	}

return(res);
}


/*
 * free the straces...
 */
static void free_straces(void)
{
struct strace *s, *skill;
int i;
struct strace_defer_free *sd, *sd2;

s=GLOBALS.straces;

while(s)
	{
	for(i=0;i<WAVE_STYPE_COUNT;i++)
		{
		if(s->back[i]) free_2(s->back[i]);
		}
	if(s->string) free_2(s->string);
	skill=s;
	s=s->next;
	free_2(skill);
	}

GLOBALS.straces=NULL;

sd = GLOBALS.strace_defer_free_head;

while(sd)
	{
	FreeTrace(sd->defer);
	sd2 = sd->next;
	free_2(sd);
	sd = sd2;
	}
GLOBALS.strace_defer_free_head = NULL;
}

/*
 * button/menu/entry activations..
 */
static void logical_clicked(GtkWidget *widget, gpointer which)
{
int i;
char *which_char;

for(i=0;i<6;i++) GLOBALS.logical_mutex[i]=0;
which_char=(char *)which;
*which_char=1;			/* mark our choice */

DEBUG(printf("picked: %s\n", logical[which_char-logical_mutex]));
}

static void stype_clicked(GtkWidget *widget, gpointer back)
{
struct strace_back *b;
struct strace *s;

b=(struct strace_back *)back;
s=b->parent;

s->value=b->which;

DEBUG(printf("Trace %s Search Type: %s\n", s->trace->name, stype[(int)s->value]));
}

static void start_clicked(GtkWidget *widget, gpointer which)
{
GLOBALS.mark_idx_start=((struct item_mark_string*)which)->idx;
if (GLOBALS.mark_idx_start<0 || GLOBALS.mark_idx_start>=(sizeof(GLOBALS.item_mark_start_strings_strace_c_1)/sizeof(struct item_mark_string)))
	{
	fprintf(stderr, "Internal error: GLOBALS.mark_idx_start out of range %d\n", GLOBALS.mark_idx_start);
	exit(255);
	}
DEBUG(printf("start: %s\n", ((struct item_mark_string*)which)->str));
}

static void end_clicked(GtkWidget *widget, gpointer which)
{
GLOBALS.mark_idx_end=((struct item_mark_string*)which)->idx;
if (GLOBALS.mark_idx_end<0 || GLOBALS.mark_idx_end>=(sizeof(GLOBALS.item_mark_end_strings_strace_c_1)/sizeof(struct item_mark_string)))
	{
	fprintf(stderr, "Internal error: GLOBALS.mark_idx_end out of range %d\n",GLOBALS.mark_idx_end);
	exit(255);
	}
DEBUG(printf("end: %s\n", ((struct item_mark_string*)which)->str));
}

static void enter_callback(GtkWidget *widget, gpointer strace_tmp)
{
G_CONST_RETURN gchar *entry_text;
struct strace *s;
int i, len;

s=(struct strace *)strace_tmp;
if(s->string) { free_2(s->string); s->string=NULL; }

entry_text = gtk_entry_get_text(GTK_ENTRY(widget));
DEBUG(printf("Trace %s Entry contents: %s\n", s->trace->name, entry_text));

if(!(len=strlen(entry_text))) return;

gtk_entry_select_region (GTK_ENTRY (widget),
                             0, GTK_ENTRY(widget)->text_length);

strcpy((s->string=(char *)malloc_2(len+1)),entry_text);
for(i=0;i<len;i++)
	{
	char ch;
	ch=s->string[i];
	if((ch>='a')&&(ch<='z')) s->string[i]=ch-('a'-'A');
	}
}

static void forwards_callback(GtkWidget *widget, GtkWidget *nothing)
{
/* no cleanup necessary, but do real search */
DEBUG(printf("Searching Forward..\n"));
strace_search(STRACE_FORWARD);
}

static void backwards_callback(GtkWidget *widget, GtkWidget *nothing)
{
/* no cleanup necessary, but do real search */
DEBUG(printf("Searching Backward..\n"));
strace_search(STRACE_BACKWARD);
}

static void mark_callback(GtkWidget *widget, GtkWidget *nothing)
{
DEBUG(printf("Marking..\n"));
if(GLOBALS.shadow_straces)
	{
	delete_strace_context();
	}

strace_maketimetrace(1);
cache_actual_pattern_mark_traces();

MaxSignalLength();
signalarea_configure_event(GLOBALS.signalarea, NULL);
wavearea_configure_event(GLOBALS.wavearea, NULL);
}

static void clear_callback(GtkWidget *widget, GtkWidget *nothing)
{
DEBUG(printf("Clearing..\n"));
if(GLOBALS.shadow_straces)
	{
	delete_strace_context();
	}
strace_maketimetrace(0);

MaxSignalLength();
signalarea_configure_event(GLOBALS.signalarea, NULL);
wavearea_configure_event(GLOBALS.wavearea, NULL);
}

static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
  free_straces();
  GLOBALS.ptr_mark_count_label_strace_c_1=NULL;
  gtk_widget_destroy(GLOBALS.window_strace_c_10);
}

/* update mark count label on pattern search dialog */

static void update_mark_count_label()
{
if(GLOBALS.ptr_mark_count_label_strace_c_1)
    {
    char mark_count_buf[64];
    sprintf (mark_count_buf, "Mark Count: %d", GLOBALS.timearray_size);
    gtk_label_set_text (GTK_LABEL(GLOBALS.ptr_mark_count_label_strace_c_1), mark_count_buf);
    }
}

void tracesearchbox(char *title, GtkSignalFunc func)
{
    GtkWidget *menu, *menuitem, *optionmenu;
    GSList *group; 
    GtkWidget *entry;
    GtkWidget *vbox, *hbox, *small_hbox, *vbox_g, *label;
    GtkWidget *button1, *button1a, *button1b, *button1c, *button2, *scrolled_win, *frame, *separator;
    Trptr t;
    int i;
    int numtraces;

    if(GLOBALS.straces) 
	{
	gdk_window_raise(GLOBALS.window_strace_c_10->window);
	return; /* is already active */
	}

    GLOBALS.cleanup_strace_c_7=func;
    numtraces=0;

    /* create a new window */
    GLOBALS.window_strace_c_10 = gtk_window_new(GLOBALS.disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW (GLOBALS.window_strace_c_10), title);
    gtk_widget_set_usize( GTK_WIDGET (GLOBALS.window_strace_c_10), 420, -1); 
    gtk_signal_connect(GTK_OBJECT (GLOBALS.window_strace_c_10), "delete_event",(GtkSignalFunc) destroy_callback, NULL);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (GLOBALS.window_strace_c_10), vbox);
    gtk_widget_show (vbox);

    vbox_g = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox_g);

    frame = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frame), 3);
    gtk_widget_show(frame);

    small_hbox = gtk_hbox_new (TRUE, 0);
    gtk_widget_show (small_hbox);

    label=gtk_label_new("Logical Operation");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (small_hbox), label, TRUE, FALSE, 0);


    menu = gtk_menu_new ();
    group=NULL;

    for(i=0;i<6;i++)
	{
    	menuitem = gtk_radio_menu_item_new_with_label (group, GLOBALS.logical_strace_c_1[i]);
    	group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    	gtk_menu_append (GTK_MENU (menu), menuitem);
    	gtk_widget_show (menuitem);
        gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                                 GTK_SIGNAL_FUNC(logical_clicked),
                                 &GLOBALS.logical_mutex[i]);
	GLOBALS.logical_mutex[i]=0;
	}

	GLOBALS.logical_mutex[0]=1;	/* "and" */

	optionmenu = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
	gtk_box_pack_start (GTK_BOX (small_hbox), optionmenu, TRUE, FALSE, 0);
	gtk_widget_show (optionmenu);

	gtk_box_pack_start (GTK_BOX (vbox), small_hbox, FALSE, FALSE, 0);

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize( GTK_WIDGET (scrolled_win), -1, 300); 
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_win);
    gtk_container_add (GTK_CONTAINER (frame), scrolled_win);
    gtk_container_add (GTK_CONTAINER (vbox), frame);

    for(t=GLOBALS.traces.first;t;t=t->t_next)
    {
    struct strace *s;

    if ((t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH))||(!(t->flags&TR_HIGHLIGHT))||(!(t->name))) continue;

    numtraces++;
    if(numtraces==500) 
	{
	status_text("Limiting waveform display search to 500 traces.\n");
	break;
	}

    s=(struct strace *)calloc_2(1,sizeof(struct strace));
    s->next=GLOBALS.straces;
    GLOBALS.straces=s;
    s->trace=t;

    if(t!=GLOBALS.traces.first)
	{
    	separator = gtk_hseparator_new ();
    	gtk_widget_show (separator);
    	gtk_box_pack_start (GTK_BOX (vbox_g), separator, FALSE, FALSE, 0);
	}

    small_hbox = gtk_hbox_new (TRUE, 0);
    gtk_widget_show (small_hbox);

    label=gtk_label_new(t->name);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (vbox_g), label, FALSE, FALSE, 0);

    menu = gtk_menu_new ();
    group=NULL;

    for(i=0;i<WAVE_STYPE_COUNT;i++)
	{
	struct strace_back *b;

	b=(struct strace_back *)calloc_2(1,sizeof(struct strace_back));
	b->parent=s;
	b->which=i;
	s->back[i]=b;
    	menuitem = gtk_radio_menu_item_new_with_label (group, GLOBALS.stype_strace_c_1[i]);
    	group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    	gtk_menu_append (GTK_MENU (menu), menuitem);
    	gtk_widget_show (menuitem);
        gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                                 GTK_SIGNAL_FUNC(stype_clicked),
                                 b);
	}

	optionmenu = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
	gtk_box_pack_start (GTK_BOX (small_hbox), optionmenu, TRUE, FALSE, 0);
	gtk_widget_show (optionmenu);

    entry = gtk_entry_new_with_max_length (257); /* %+256ch */
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       GTK_SIGNAL_FUNC(enter_callback),
		       s);

    gtk_box_pack_start (GTK_BOX (small_hbox), entry, TRUE, FALSE, 0);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (vbox_g), small_hbox, FALSE, FALSE, 0);
    }

    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win), vbox_g);

    do		/* add GUI elements for displaying mark count and mark count start/end */
	{
	int idx;
	GtkWidget *ptr_mark_start_label, *ptr_mark_end_label;
	GtkWidget *mark_count_hbox_start,  *mark_count_hbox_end; 
	GtkWidget *count_vbox_left, *count_vbox_right, *count_vbox, *count_hbox;
	GtkWidget *ptr_mark_count_start, *ptr_mark_count_end;

	count_hbox=gtk_hbox_new (TRUE, 0);
	gtk_widget_show (count_hbox);
	gtk_box_pack_start (GTK_BOX(vbox),count_hbox,FALSE,FALSE,0);

		/* add a vertical box to display the mark count GUI elements */
	count_vbox_left=gtk_vbox_new (TRUE, 0);
	gtk_widget_show (count_vbox_left);
	gtk_box_pack_start (GTK_BOX(count_hbox),count_vbox_left,FALSE,FALSE,0);
	count_vbox=gtk_vbox_new (TRUE, 0);
	gtk_widget_show (count_vbox);
	gtk_box_pack_start (GTK_BOX(count_hbox),count_vbox,FALSE,FALSE,0);
	count_vbox_right=gtk_vbox_new (TRUE, 0);
	gtk_widget_show (count_vbox_right);
	gtk_box_pack_start (GTK_BOX(count_hbox),count_vbox_right,FALSE,FALSE,0);

		/* add mark start GUI element */
	mark_count_hbox_start=gtk_hbox_new (TRUE, 0);
	gtk_widget_show (mark_count_hbox_start);
	gtk_box_pack_start (GTK_BOX(count_vbox),mark_count_hbox_start,FALSE,FALSE,0);
	ptr_mark_start_label=gtk_label_new ("Marking Begins at:");
	gtk_widget_show (ptr_mark_start_label);
	gtk_box_pack_start (GTK_BOX (mark_count_hbox_start),ptr_mark_start_label,TRUE,FALSE,0);
	ptr_mark_count_start = gtk_menu_new ();
	group=NULL;
	for(idx=0; idx<sizeof(GLOBALS.item_mark_start_strings_strace_c_1)/sizeof(struct item_mark_string); ++idx)
		{
		GLOBALS.item_mark_start_strings_strace_c_1[idx].idx=idx;
    		menuitem = gtk_radio_menu_item_new_with_label (group, GLOBALS.item_mark_start_strings_strace_c_1[idx].str);
    		group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    		gtk_menu_append (GTK_MENU (ptr_mark_count_start), menuitem);
    		gtk_widget_show (menuitem);
        	gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                                 GTK_SIGNAL_FUNC(start_clicked),
                                 &GLOBALS.item_mark_start_strings_strace_c_1[idx]);
		}
	optionmenu = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), ptr_mark_count_start);
	gtk_box_pack_start (GTK_BOX (mark_count_hbox_start), optionmenu, TRUE, FALSE, 0);
	gtk_widget_show (optionmenu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu), GLOBALS.mark_idx_start);

		/* add mark end GUI element */
	mark_count_hbox_end=gtk_hbox_new (TRUE, 0);
	gtk_widget_show (mark_count_hbox_end);
	gtk_box_pack_start (GTK_BOX(count_vbox),mark_count_hbox_end,FALSE,FALSE,0);
	ptr_mark_end_label=gtk_label_new ("Marking Stops at:");
	gtk_widget_show (ptr_mark_end_label);
	gtk_box_pack_start (GTK_BOX (mark_count_hbox_end),ptr_mark_end_label,TRUE,FALSE,0);
	ptr_mark_count_end = gtk_menu_new ();
	group=NULL;
	for(idx=0; idx<sizeof(GLOBALS.item_mark_end_strings_strace_c_1)/sizeof(struct item_mark_string); ++idx)
		{
		GLOBALS.item_mark_end_strings_strace_c_1[idx].idx=idx;
    		menuitem = gtk_radio_menu_item_new_with_label (group, GLOBALS.item_mark_end_strings_strace_c_1[idx].str);
    		group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    		gtk_menu_append (GTK_MENU (ptr_mark_count_end), menuitem);
    		gtk_widget_show (menuitem);
        	gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                                 GTK_SIGNAL_FUNC(end_clicked),
                                 &GLOBALS.item_mark_end_strings_strace_c_1[idx]);
		}
	optionmenu = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), ptr_mark_count_end);
	gtk_box_pack_start (GTK_BOX (mark_count_hbox_end), optionmenu, TRUE, FALSE, 0);
	gtk_widget_show (optionmenu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu), GLOBALS.mark_idx_end);

		/* add mark count GUI element */
	GLOBALS.ptr_mark_count_label_strace_c_1=gtk_label_new ("");
	gtk_widget_show (GLOBALS.ptr_mark_count_label_strace_c_1);
	gtk_box_pack_start (GTK_BOX (count_vbox),GLOBALS.ptr_mark_count_label_strace_c_1,TRUE,FALSE,0);
	update_mark_count_label ();
	} while (0);

    hbox = gtk_hbox_new (FALSE, 1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("Fwd");
    gtk_widget_set_usize(button1, 75, -1);
    gtk_signal_connect(GTK_OBJECT (button1), "clicked",
			       GTK_SIGNAL_FUNC(forwards_callback),
			       NULL);
    gtk_widget_show (button1);
    gtk_container_add (GTK_CONTAINER (hbox), button1);
    GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
    gtk_signal_connect_object (GTK_OBJECT (button1),
                                "realize",
                             (GtkSignalFunc) gtk_widget_grab_default,
                             GTK_OBJECT (button1));


    button1a = gtk_button_new_with_label ("Bkwd");
    gtk_widget_set_usize(button1a, 75, -1);
    gtk_signal_connect(GTK_OBJECT (button1a), "clicked",
			       GTK_SIGNAL_FUNC(backwards_callback),
			       NULL);
    gtk_widget_show (button1a);
    gtk_container_add (GTK_CONTAINER (hbox), button1a);
    GTK_WIDGET_SET_FLAGS (button1a, GTK_CAN_DEFAULT);

    button1b = gtk_button_new_with_label ("Mark");
    gtk_widget_set_usize(button1b, 75, -1);
    gtk_signal_connect(GTK_OBJECT (button1b), "clicked",
			       GTK_SIGNAL_FUNC(mark_callback),
			       NULL);
    gtk_widget_show (button1b);
    gtk_container_add (GTK_CONTAINER (hbox), button1b);
    GTK_WIDGET_SET_FLAGS (button1b, GTK_CAN_DEFAULT);

    button1c = gtk_button_new_with_label ("Clear");
    gtk_widget_set_usize(button1c, 75, -1);
    gtk_signal_connect(GTK_OBJECT (button1c), "clicked",
			       GTK_SIGNAL_FUNC(clear_callback),
			       NULL);
    gtk_widget_show (button1c);
    gtk_container_add (GTK_CONTAINER (hbox), button1c);
    GTK_WIDGET_SET_FLAGS (button1c, GTK_CAN_DEFAULT);

    button2 = gtk_button_new_with_label ("Exit");
    gtk_widget_set_usize(button2, 75, -1);
    gtk_signal_connect(GTK_OBJECT (button2), "clicked",
			       GTK_SIGNAL_FUNC(destroy_callback),
			       NULL);
    GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
    gtk_widget_show (button2);
    gtk_container_add (GTK_CONTAINER (hbox), button2);

    gtk_widget_show(GLOBALS.window_strace_c_10);
}


/*
 * strace backward or forward..
 */
void strace_search(int direction)
{
struct strace *s;
TimeType basetime, maxbase, sttim, fintim;
Trptr t;
int totaltraces, passcount;
int whichpass;
TimeType middle=0, width;


if(direction==STRACE_BACKWARD) /* backwards */
{
if(GLOBALS.tims.marker<0)
	{
	basetime=MAX_HISTENT_TIME;
	}
	else
	{	
	basetime=GLOBALS.tims.marker;
	}
}
else /* go forwards */
{
if(GLOBALS.tims.marker<0)
	{
	basetime=GLOBALS.tims.first;
	}
	else
	{	
	basetime=GLOBALS.tims.marker;
	}
} 

sttim=GLOBALS.tims.first;
fintim=GLOBALS.tims.last;

for(whichpass=0;;whichpass++)
{

if(direction==STRACE_BACKWARD) /* backwards */
{
maxbase=-1;
s=GLOBALS.straces;
while(s)
	{
	t=s->trace;
	GLOBALS.shift_timebase=t->shift;
	if(!(t->vector))
		{
		hptr h;
		hptr *hp;
		UTimeType utt;
		TimeType  tt;

		h=bsearch_node(t->n.nd, basetime);
		hp=GLOBALS.max_compare_index;
		if((hp==&(t->n.nd->harray[1]))||(hp==&(t->n.nd->harray[0]))) return;
		hp--;
		h=*hp;
		s->his.h=h;
		utt=adjust(h->time,GLOBALS.shift_timebase); tt=utt;
		if(tt > maxbase) maxbase=tt;
		}
		else
		{
		vptr v;
		vptr *vp;
		UTimeType utt;
		TimeType  tt;

		v=bsearch_vector(t->n.vec, basetime);
		vp=GLOBALS.vmax_compare_index;
		if((vp==&(t->n.vec->vectors[1]))||(vp==&(t->n.vec->vectors[0]))) return;
		vp--;
		v=*vp;
		s->his.v=v;
		utt=adjust(v->time,GLOBALS.shift_timebase); tt=utt;
		if(tt > maxbase) maxbase=tt;
		}

	s=s->next;
	}
}
else /* go forward */
{
maxbase=MAX_HISTENT_TIME;
s=GLOBALS.straces;
while(s)
	{
	t=s->trace;
	GLOBALS.shift_timebase=t->shift;
	if(!(t->vector))
		{
		hptr h;
		UTimeType utt;
		TimeType  tt;

		h=bsearch_node(t->n.nd, basetime);
		while(h->next && h->time==h->next->time) h=h->next;
		if((whichpass)||(GLOBALS.tims.marker>=0)) h=h->next;
		if(!h) return;
		s->his.h=h;
		utt=adjust(h->time,GLOBALS.shift_timebase); tt=utt;		
		if(tt < maxbase) maxbase=tt;
		}
		else
		{
		vptr v;
		UTimeType utt;
		TimeType  tt;

		v=bsearch_vector(t->n.vec, basetime);
		while(v->next && v->time==v->next->time) v=v->next;
		if((whichpass)||(GLOBALS.tims.marker>=0)) v=v->next;
		if(!v) return;
		s->his.v=v;
		utt=adjust(v->time,GLOBALS.shift_timebase); tt=utt;
		if(tt < maxbase) maxbase=tt;
		}

	s=s->next;
	}
}

s=GLOBALS.straces;
totaltraces=0;	/* increment when not don't care */
while(s)
	{
	char str[2];
	t=s->trace;
	s->search_result=0;	/* explicitly must set this */
	GLOBALS.shift_timebase=t->shift;
	
	if((!t->vector)&&(!(t->n.nd->ext)))
		{
		if(adjust(s->his.h->time,GLOBALS.shift_timebase)!=maxbase) 
			{
			s->his.h=bsearch_node(t->n.nd, maxbase);
			while(s->his.h->next && s->his.h->time==s->his.h->next->time) s->his.h=s->his.h->next;
			}
		if(t->flags&TR_INVERT)  
                	{
                        str[0]=AN_STR_INV[s->his.h->v.h_val];
                        }
                        else   
                        {
                        str[0]=AN_STR[s->his.h->v.h_val];
                        }
		str[1]=0x00;

		switch(s->value)
			{
			case ST_DC:
				break;
				
			case ST_HIGH:
				totaltraces++;
				if((str[0]=='1')||(str[0]=='H')) s->search_result=1;	
				break;

			case ST_RISE:
				if((str[0]=='1')||(str[0]=='H')) s->search_result=1;	
				totaltraces++;
				break;

			case ST_LOW:
				totaltraces++;
				if((str[0]=='0')||(str[0]=='L')) s->search_result=1;
				break;

			case ST_FALL:
				totaltraces++;
				if((str[0]=='0')||(str[0]=='L')) s->search_result=1;
				break;

			case ST_MID:
				totaltraces++;
				if(str[0]=='Z')
 					s->search_result=1;
				break;				

			case ST_X:
				totaltraces++;
				if(str[0]=='X') s->search_result=1;
				break;

			case ST_ANY:
				totaltraces++;
				s->search_result=1;
				break;
		
			case ST_STRING:
				totaltraces++;
				if(s->string)
				if(strstr_i(s->string,str)) s->search_result=1;
				break;

			default:
				fprintf(stderr, "Internal error: st_type of %d\n",s->value);
				exit(255);
			}


		}
		else
		{
		char *chval, *chval2;
		char ch;

		if(t->vector)
			{
			if(adjust(s->his.v->time,GLOBALS.shift_timebase)!=maxbase) 
				{
				s->his.v=bsearch_vector(t->n.vec, maxbase);
				while(s->his.v->next && s->his.v->time==s->his.v->next->time) s->his.v=s->his.v->next;
				}
			chval=convert_ascii(t,s->his.v);
			}
			else
			{
			if(adjust(s->his.h->time,GLOBALS.shift_timebase)!=maxbase) 
				{
				s->his.h=bsearch_node(t->n.nd, maxbase);
				while(s->his.h->next && s->his.h->time==s->his.h->next->time) s->his.h=s->his.h->next;
				}
			if(s->his.h->flags&HIST_REAL)
				{
				if(!(s->his.h->flags&HIST_STRING))
					{
					chval=convert_ascii_real((double *)s->his.h->v.h_vector);
					}
					else
					{
					chval=convert_ascii_string((char *)s->his.h->v.h_vector);
					chval2=chval;
					while((ch=*chval2))	/* toupper() the string */
						{
						if((ch>='a')&&(ch<='z')) { *chval2= ch-('a'-'A'); }
						chval2++;
						}
					}
				}
				else
				{
				chval=convert_ascii_vec(t,s->his.h->v.h_vector);
				}
			}

		switch(s->value)
			{
			case ST_DC:
				break;

			case ST_RISE:
			case ST_FALL:
				totaltraces++;
				break;
				
			case ST_HIGH:
				totaltraces++;
				if((chval2=chval))
				while((ch=*(chval2++)))
					{
					if(((ch>='1')&&(ch<='9'))||(ch=='H')||((ch>='A')&&(ch<='F')))
						{
						s->search_result=1;
						break;
						}
					}
				break;

			case ST_LOW:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if((ch!='0')&&(ch!='L'))
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_MID:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if(ch!='Z')
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_X:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if((ch!='X')&&(ch!='W'))
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_ANY:
				totaltraces++;
				s->search_result=1;
				break;
		
			case ST_STRING:
				totaltraces++;
				if(s->string)
				if(strstr_i(chval, s->string)) s->search_result=1;
				break;

			default:
				fprintf(stderr, "Internal error: st_type of %d\n",s->value);
				exit(255);
			}

		free_2(chval);
		}
	s=s->next;
	}

if((maxbase<sttim)||(maxbase>fintim)) return;

DEBUG(printf("Maxbase: "TTFormat", total traces: %d\n",maxbase, totaltraces));
s=GLOBALS.straces;
passcount=0;
while(s)
	{
	DEBUG(printf("\tPass: %d, Name: %s\n",s->search_result, s->trace->name));
	if(s->search_result) passcount++;
	s=s->next;
	}

if(totaltraces)
	{
	if(GLOBALS.logical_mutex[0])	/* and */
		{
		if(totaltraces==passcount) break;		
		}
	else
	if(GLOBALS.logical_mutex[1])	/* or */
		{
		if(passcount) break;
		}
	else
	if(GLOBALS.logical_mutex[2])	/* xor */
		{
		if(passcount&1) break;
		}
	else
	if(GLOBALS.logical_mutex[3])	/* nand */
		{
		if(totaltraces!=passcount) break;
		}
	else
	if(GLOBALS.logical_mutex[4])	/* nor */
		{
		if(!passcount) break;
		}
	else
	if(GLOBALS.logical_mutex[5])	/* xnor */
		{
		if(!(passcount&1)) break;
		}
	}

basetime=maxbase;
}

update_markertime(GLOBALS.tims.marker=maxbase);

width=(TimeType)(((gdouble)GLOBALS.wavewidth)*GLOBALS.nspx);
if((GLOBALS.tims.marker<GLOBALS.tims.start)||(GLOBALS.tims.marker>=GLOBALS.tims.start+width))
	{
	if((GLOBALS.tims.marker<0)||(GLOBALS.tims.marker<GLOBALS.tims.first)||(GLOBALS.tims.marker>GLOBALS.tims.last))
	                {
	                if(GLOBALS.tims.end>GLOBALS.tims.last) GLOBALS.tims.end=GLOBALS.tims.last;
	                middle=(GLOBALS.tims.start/2)+(GLOBALS.tims.end/2);
	                if((GLOBALS.tims.start&1)&&(GLOBALS.tims.end&1)) middle++;
	                }
	                else
	                { 
	                middle=GLOBALS.tims.marker;
	                }
	
	GLOBALS.tims.start=time_trunc(middle-(width/2));
	if(GLOBALS.tims.start+width>GLOBALS.tims.last) GLOBALS.tims.start=GLOBALS.tims.last-width;
	if(GLOBALS.tims.start<GLOBALS.tims.first) GLOBALS.tims.start=GLOBALS.tims.first;  
	GTK_ADJUSTMENT(GLOBALS.wave_hslider)->value=GLOBALS.tims.timecache=GLOBALS.tims.start;
	}

MaxSignalLength();
signalarea_configure_event(GLOBALS.signalarea, NULL);
wavearea_configure_event(GLOBALS.wavearea, NULL);
}


/*********************************************/

/*
 * strace forward to make the timetrace
 */
TimeType strace_timetrace(TimeType basetime, int notfirst)
{
struct strace *s;
TimeType maxbase, fintim;
Trptr t;
int totaltraces, passcount;
int whichpass;

fintim=GLOBALS.tims.last;

for(whichpass=0;;whichpass++)
{
maxbase=MAX_HISTENT_TIME;
s=GLOBALS.straces;
while(s)
	{
	t=s->trace;
	GLOBALS.shift_timebase=t->shift;
	if(!(t->vector))
		{
		hptr h;
		UTimeType utt;
		TimeType  tt;

		h=bsearch_node(t->n.nd, basetime);
		s->his.h=h;
		while(h->time==h->next->time) h=h->next;
		if((whichpass)||(notfirst)) h=h->next;
		if(!h) return(MAX_HISTENT_TIME);
		utt=adjust(h->time,GLOBALS.shift_timebase); tt=utt;		
		if(tt < maxbase) maxbase=tt;
		}
		else
		{
		vptr v;
		UTimeType utt;
		TimeType  tt;

		v=bsearch_vector(t->n.vec, basetime);
		if((whichpass)||(notfirst)) v=v->next;
		if(!v) return(MAX_HISTENT_TIME);
		s->his.v=v;
		utt=adjust(v->time,GLOBALS.shift_timebase); tt=utt;
		if(tt < maxbase) maxbase=tt;
		}

	s=s->next;
	}

s=GLOBALS.straces;
totaltraces=0;	/* increment when not don't care */
while(s)
	{
	char str[2];
	t=s->trace;
	s->search_result=0;	/* explicitly must set this */
	GLOBALS.shift_timebase=t->shift;
	
	if((!t->vector)&&(!(t->n.nd->ext)))
		{
		if(adjust(s->his.h->time,GLOBALS.shift_timebase)!=maxbase) 
			{
			s->his.h=bsearch_node(t->n.nd, maxbase);
			while(s->his.h->next && s->his.h->time==s->his.h->next->time) s->his.h=s->his.h->next;
			}
		if(t->flags&TR_INVERT)  
                	{
                        str[0]=AN_STR_INV[s->his.h->v.h_val];
                        }
                        else   
                        {
                        str[0]=AN_STR[s->his.h->v.h_val];
                        }
		str[1]=0x00;

		switch(s->value)
			{
			case ST_DC:
				break;
				
			case ST_HIGH:
				totaltraces++;
				if((str[0]=='1')||(str[0]=='H')) s->search_result=1;	
				break;

			case ST_RISE:
				totaltraces++;
				if(((str[0]=='1')||(str[0]=='H'))&&(adjust(s->his.h->time,GLOBALS.shift_timebase)==maxbase)) 
					s->search_result=1;	
				break;

			case ST_LOW:
				totaltraces++;
				if((str[0]=='0')||(str[0]=='L')) s->search_result=1;
				break;

			case ST_FALL:
				totaltraces++;
				if(((str[0]=='0')||(str[0]=='L'))&&(adjust(s->his.h->time,GLOBALS.shift_timebase)==maxbase))
 					s->search_result=1;
				break;

			case ST_MID:
				totaltraces++;
				if(str[0]=='Z')
 					s->search_result=1;
				break;				

			case ST_X:
				totaltraces++;
				if(str[0]=='X') s->search_result=1;
				break;

			case ST_ANY:
				totaltraces++;
				if(adjust(s->his.h->time,GLOBALS.shift_timebase)==maxbase)s->search_result=1;
				break;
		
			case ST_STRING:
				totaltraces++;
				if(s->string)
				if(strstr_i(s->string,str)) s->search_result=1;
				break;

			default:
				fprintf(stderr, "Internal error: st_type of %d\n",s->value);
				exit(255);
			}


		}
		else
		{
		char *chval, *chval2;
		char ch;

		if(t->vector)
			{
			if(adjust(s->his.v->time,GLOBALS.shift_timebase)!=maxbase) 
				{
				s->his.v=bsearch_vector(t->n.vec, maxbase);
				while(s->his.v->next && s->his.v->time==s->his.v->next->time) s->his.v=s->his.v->next;
				}
			chval=convert_ascii(t,s->his.v);
			}
			else
			{
			if(adjust(s->his.h->time,GLOBALS.shift_timebase)!=maxbase) 
				{
				s->his.h=bsearch_node(t->n.nd, maxbase);
				while(s->his.h->next && s->his.h->time==s->his.h->next->time) s->his.h=s->his.h->next;
				}
			if(s->his.h->flags&HIST_REAL)
				{
				if(!(s->his.h->flags&HIST_STRING))
					{
					chval=convert_ascii_real((double *)s->his.h->v.h_vector);
					}
					else
					{
					chval=convert_ascii_string((char *)s->his.h->v.h_vector);
					chval2=chval;
					while((ch=*chval2))	/* toupper() the string */
						{
						if((ch>='a')&&(ch<='z')) { *chval2= ch-('a'-'A'); }
						chval2++;
						}
					}
				}
				else
				{
				chval=convert_ascii_vec(t,s->his.h->v.h_vector);
				}
			}

		switch(s->value)
			{
			case ST_DC:
				break;

			case ST_RISE:
			case ST_FALL:
				totaltraces++;
				break;
				
			case ST_HIGH:
				totaltraces++;
				if((chval2=chval))
				while((ch=*(chval2++)))
					{
					if(((ch>='1')&&(ch<='9'))||(ch=='H')||((ch>='A')&&(ch<='F')))
						{
						s->search_result=1;
						break;
						}
					}
				break;

			case ST_LOW:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if((ch!='0')&&(ch!='L'))
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_MID:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if(ch!='Z')
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_X:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if((ch!='X')&&(ch!='W'))
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_ANY:
				totaltraces++;
				if(adjust(s->his.v->time,GLOBALS.shift_timebase)==maxbase)
					s->search_result=1;
				break;
		
			case ST_STRING:
				totaltraces++;
				if(s->string)
				if(strstr_i(chval, s->string)) s->search_result=1;
				break;

			default:
				fprintf(stderr, "Internal error: st_type of %d\n",s->value);
				exit(255);
			}

		free_2(chval);
		}
	s=s->next;
	}

if(maxbase>fintim) return(MAX_HISTENT_TIME);

DEBUG(printf("Maxbase: "TTFormat", total traces: %d\n",maxbase, totaltraces));
s=GLOBALS.straces;
passcount=0;
while(s)
	{
	DEBUG(printf("\tPass: %d, Name: %s\n",s->search_result, s->trace->name));
	if(s->search_result) passcount++;
	s=s->next;
	}

if(totaltraces)
	{
	if(GLOBALS.logical_mutex[0])	/* and */
		{
		if(totaltraces==passcount) break;		
		}
	else
	if(GLOBALS.logical_mutex[1])	/* or */
		{
		if(passcount) break;
		}
	else
	if(GLOBALS.logical_mutex[2])	/* xor */
		{
		if(passcount&1) break;
		}
	else
	if(GLOBALS.logical_mutex[3])	/* nand */
		{
		if(totaltraces!=passcount) break;
		}
	else
	if(GLOBALS.logical_mutex[4])	/* nor */
		{
		if(!passcount) break;
		}
	else
	if(GLOBALS.logical_mutex[5])	/* xnor */
		{
		if(!(passcount&1)) break;
		}
	}

basetime=maxbase;
}


return(maxbase);
}


void strace_maketimetrace(int mode)
{
TimeType basetime=GLOBALS.tims.first;
TimeType endtime =MAX_HISTENT_TIME;
int i, notfirst=0;
struct timechain *tchead=NULL, *tc=NULL, *tcnext;
TimeType *t;

if(GLOBALS.timearray)
	{
	free_2(GLOBALS.timearray);
	GLOBALS.timearray=NULL;
	}

GLOBALS.timearray_size=0;

if((!mode)&&(!GLOBALS.shadow_active))
	{
	update_mark_count_label();
	delete_mprintf();
	return;	/* merely free stuff up */
	}

if(GLOBALS.mark_idx_start>0)
	{
	if(GLOBALS.named_markers[GLOBALS.mark_idx_start-1]>=0)
		basetime=GLOBALS.named_markers[GLOBALS.mark_idx_start-1];
	else
		{
		status_text(GLOBALS.item_mark_start_strings_strace_c_1[GLOBALS.mark_idx_start].str);
		status_text(" not in use.\n");
		}
	}

if(GLOBALS.mark_idx_end>0)
	{
	if(GLOBALS.named_markers[GLOBALS.mark_idx_end-1]>=0)
		endtime=GLOBALS.named_markers[GLOBALS.mark_idx_end-1];
	else
		{
		status_text(GLOBALS.item_mark_end_strings_strace_c_1[GLOBALS.mark_idx_end].str);
		status_text(" not in use.\n");
		}
	}

if(basetime>endtime)
	{
	TimeType tmp=basetime;
	basetime    =endtime;
	endtime     =tmp;
	}

while(1)
	{
	basetime=strace_timetrace(basetime, notfirst);
	notfirst=1;

	if(endtime==MAX_HISTENT_TIME)
		{
		if(basetime==MAX_HISTENT_TIME) break;
		}
		else
	   	{
		if(basetime>=endtime) break;
		}

	GLOBALS.timearray_size++;

	if(!tc)
		{
		tchead=tc=malloc_2(sizeof(struct timechain));	/* formerly wave_alloca but that dies > 4MB on linux x86! */
		}
		else
		{
		tc->next=malloc_2(sizeof(struct timechain));	/* formerly wave_alloca but that dies > 4MB on linux x86! */
		tc=tc->next;
		}

	tc->t=basetime;
	tc->next=NULL;
	}

if(GLOBALS.timearray_size)
	{
	GLOBALS.timearray=t=malloc_2(sizeof(TimeType)*GLOBALS.timearray_size);
	for(i=0;i<GLOBALS.timearray_size;i++)
		{
		*(t++)=tchead->t;
		tcnext=tchead->next;
		free_2(tchead);					/* need to explicitly free this up now */
		tchead=tcnext;
		}
	}
	else
	{
	GLOBALS.timearray = NULL;
	}

if(!GLOBALS.shadow_active) update_mark_count_label();
}


/*
 * swap context for mark during trace load...
 */
void swap_strace_contexts(void)
{
struct strace *stemp;
char logical_mutex_temp[6];
char  mark_idx_start_temp, mark_idx_end_temp;

stemp = GLOBALS.straces;
GLOBALS.straces = GLOBALS.shadow_straces;
GLOBALS.shadow_straces = stemp;

memcpy(logical_mutex_temp, GLOBALS.logical_mutex, 6);
memcpy(GLOBALS.logical_mutex, GLOBALS.shadow_logical_mutex, 6);
memcpy(GLOBALS.shadow_logical_mutex, logical_mutex_temp, 6);

mark_idx_start_temp   = GLOBALS.mark_idx_start;
GLOBALS.mark_idx_start        = GLOBALS.shadow_mark_idx_start;
GLOBALS.shadow_mark_idx_start = mark_idx_start_temp;

mark_idx_end_temp   = GLOBALS.mark_idx_end;
GLOBALS.mark_idx_end        = GLOBALS.shadow_mark_idx_end;
GLOBALS.shadow_mark_idx_end = mark_idx_end_temp;
}


/*
 * delete context
 */
void delete_strace_context(void)
{
int i;
struct strace *stemp;
struct strace *strace_cache;

for(i=0;i<6;i++)
	{
	GLOBALS.shadow_logical_mutex[i] = 0;
	}

GLOBALS.shadow_mark_idx_start = 0;
GLOBALS.shadow_mark_idx_end   = 0;

strace_cache = GLOBALS.straces;	/* so the trace actually deletes */
GLOBALS.straces=NULL;

stemp = GLOBALS.shadow_straces;
while(stemp)
	{
	GLOBALS.shadow_straces = stemp->next;
	if(stemp->string) free_2(stemp->string);

	FreeTrace(stemp->trace);
	free_2(stemp);
	stemp = GLOBALS.shadow_straces;
	}

if(GLOBALS.shadow_string)
	{
	free_2(GLOBALS.shadow_string);
	GLOBALS.shadow_string = NULL;
	}

GLOBALS.straces = strace_cache;
}

/*************************************************************************/

/*
 * printf to memory..
 */
  
int mprintf(const char *fmt, ... )
{
int len;
int rc;
va_list args;
struct mprintf_buff_t *bt = (struct mprintf_buff_t *)calloc_2(1, sizeof(struct mprintf_buff_t));
char buff[65537];
                                
va_start(args, fmt);
rc=vsprintf(buff, fmt, args);
len = strlen(buff);
bt->str = malloc_2(len+1);
strcpy(bt->str, buff);
                 
if(!GLOBALS.mprintf_buff_current)
        {
        GLOBALS.mprintf_buff_head = GLOBALS.mprintf_buff_current = bt;
        }
        else
        {
        GLOBALS.mprintf_buff_current->next = bt;
        GLOBALS.mprintf_buff_current = bt;
        }

va_end(args);                        
return(rc);
}

/*
 * kill mprint buffer
 */
void delete_mprintf(void)
{
if(GLOBALS.mprintf_buff_head)
	{
	struct mprintf_buff_t *mb = GLOBALS.mprintf_buff_head;
	struct mprintf_buff_t *mbt;		
		
	while(mb)
		{
		free_2(mb->str);
		mbt = mb->next;
		free_2(mb);
		mb = mbt;
		}

	GLOBALS.mprintf_buff_head = GLOBALS.mprintf_buff_current = NULL;
	}
}


/*
 * so we can (later) write out the traces which are actually marked...
 */
void cache_actual_pattern_mark_traces(void)
{
Trptr t;
unsigned int def=0;
TimeType prevshift=LLDescriptor(0);
struct strace *st;

delete_mprintf();

if(GLOBALS.timearray)
	{
	mprintf("!%d%d%d%d%d%d%c%c\n", GLOBALS.logical_mutex[0], GLOBALS.logical_mutex[1], GLOBALS.logical_mutex[2], GLOBALS.logical_mutex[3], GLOBALS.logical_mutex[4], GLOBALS.logical_mutex[5], '@'+GLOBALS.mark_idx_start, '@'+GLOBALS.mark_idx_end);
	st=GLOBALS.straces;

	while(st)
		{
		if(st->value==ST_STRING)
			{
			mprintf("?\"%s\n", st->string ? st->string : ""); /* search type for this trace is string.. */
			}
			else
			{
			mprintf("?%02x\n", (unsigned char)st->value);	/* else search type for this trace.. */
			}

		t=st->trace;

		if((t->flags!=def)||(st==GLOBALS.straces))
			{
			mprintf("@%x\n",def=t->flags);
			}

		if((t->shift)||((prevshift)&&(!t->shift)))
			{
			mprintf(">"TTFormat"\n", t->shift);
			}
		prevshift=t->shift;

		if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))	
			{
			if(t->vector)
				{
				int i;
				nptr *nodes;

				mprintf("#%s",t->name);

				nodes=t->n.vec->bits->nodes;
				for(i=0;i<t->n.vec->nbits;i++)
					{
					if(nodes[i]->expansion)
						{
						mprintf(" (%d)%s",nodes[i]->expansion->parentbit, nodes[i]->expansion->parent->nname);
						}
						else
						{
						mprintf(" %s",nodes[i]->nname);
						}
					}
				mprintf("\n");
				}
				else
				{
				if(t->is_alias)
					{
					if(t->n.nd->expansion)
						{
						mprintf("+%s (%d)%s\n",t->name+2,t->n.nd->expansion->parentbit, t->n.nd->expansion->parent->nname);
						}
						else
						{
						mprintf("+%s %s\n",t->name+2,t->n.nd->nname);
						}
					}
					else
					{
					if(t->n.nd->expansion)
						{
						mprintf("(%d)%s\n",t->n.nd->expansion->parentbit, t->n.nd->expansion->parent->nname);
						}
						else
						{
						mprintf("%s\n",t->n.nd->nname);
						}
					}
				}
			}

		st=st->next;
		} /* while(st)... */

	mprintf("!!\n");	/* mark end of strace region */
	}
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
 * Revision 1.1.1.1  2007/05/30 04:27:24  gtkwave
 * Imported sources
 *
 * Revision 1.2  2007/04/20 02:08:17  gtkwave
 * initial release
 *
 */

