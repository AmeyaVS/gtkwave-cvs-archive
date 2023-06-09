/* 
 * Copyright (c) Tony Bybell 1999-2010.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gdk/gdkkeysyms.h>
#include "gtk12compat.h"
#include "currenttime.h"
#include "pixmaps.h"
#include "symbol.h"
#include "debug.h"
#include "rpc.h"

#undef FOCUS_DEBUG_MSGS

/*
 * complain about certain ops conflict with dnd...
 */
void dnd_error(void)
{ 
status_text("Can't perform that operation when waveform drag and drop is in progress!\n");
}  


static void     
service_hslider(GtkWidget *text, gpointer data)
{
GtkAdjustment *hadj;
gint xsrc;

if(GLOBALS->signalpixmap)
	{
	hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
	xsrc=(gint)hadj->value;
	DEBUG(printf("Signal HSlider Moved to %d\n",xsrc));

	gdk_draw_rectangle(GLOBALS->signalpixmap, GLOBALS->gc_mdgray, TRUE,
	        0, -1, GLOBALS->signal_fill_width, GLOBALS->fontheight);
	gdk_draw_line(GLOBALS->signalpixmap, GLOBALS->gc_white,  
	        0, GLOBALS->fontheight-1, GLOBALS->signal_fill_width-1, GLOBALS->fontheight-1);
	font_engine_draw_string(GLOBALS->signalpixmap, GLOBALS->signalfont,
	        GLOBALS->gc_black, 3+xsrc, GLOBALS->fontheight-4, "Time");

	if(GLOBALS->signalarea_has_focus)
		{
		gdk_draw_pixmap(GLOBALS->signalarea->window, GLOBALS->signalarea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->signalarea)],GLOBALS->signalpixmap,
			xsrc+1, 0+1,
			0+1, 0+1,
			GLOBALS->signalarea->allocation.width-2, GLOBALS->signalarea->allocation.height-2);
		draw_signalarea_focus();
		}
		else
		{
		gdk_draw_pixmap(GLOBALS->signalarea->window, GLOBALS->signalarea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->signalarea)],GLOBALS->signalpixmap,xsrc, 0,0, 0,GLOBALS->signalarea->allocation.width, GLOBALS->signalarea->allocation.height);
		}
	}
}


void draw_signalarea_focus(void)
{
if(GLOBALS->signalarea_has_focus)
        {
        gdk_draw_rectangle(GLOBALS->signalarea->window, GLOBALS->gc_black, FALSE, 0, 0, 
		GLOBALS->signalarea->allocation.width-1, GLOBALS->signalarea->allocation.height-1);
	}
}

/**************************************************************************/
/***  standard click routines turned on with "use_standard_clicking"=1  ***/

/*
 *      DND "drag_begin" handler, this is called whenever a drag starts.
 */
static void DNDBeginCB(
        GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{

GLOBALS->dnd_state = 1;
}
 
/*
 *      DND "drag_end" handler, this is called when a drag and drop has
 *      completed. So this function is the last one to be called in
 *      any given DND operation.
 */
static void DNDEndCB(
        GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
GtkWidget *ddest;
int which;
gdouble x,y;
GdkModifierType state;
Trptr t;
int trwhich, trtarget;
int must_update_screen = 0;
        
#ifdef WAVE_USE_GTK2    
gint xi, yi;
#else
GdkEventMotion event[1];
event[0].deviceid = GDK_CORE_POINTER;
#endif

if(GLOBALS->std_dnd_tgt_on_signalarea || GLOBALS->std_dnd_tgt_on_wavearea)
	{
	GtkAdjustment *wadj;
        wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);

	WAVE_GDK_GET_POINTER(GLOBALS->std_dnd_tgt_on_signalarea ? GLOBALS->signalarea->window : GLOBALS->wavearea->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;

        which=(int)(y);
        which=(which/GLOBALS->fontheight)-2;
	if(which < -1) which = -1;

	trtarget=((int)wadj->value)+which;

	ddest = (GLOBALS->std_dnd_tgt_on_signalarea) ? GTK_WIDGET(GLOBALS->signalarea) : GTK_WIDGET(GLOBALS->wavearea);
	if((x<0)||(x>=ddest->allocation.width)||(y<0)||(y>=ddest->allocation.height))
		{
		goto bot;
		}

	GLOBALS->cachedtrace=t=GLOBALS->traces.first;
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

	while(t && t->t_next && IsGroupEnd(t->t_next) && IsCollapsed(t->t_next)) { /* added missing "t &&" because of possible while termination above */
	  t = t->t_next;
	}
        
        GLOBALS->cachedtrace=t;
        if(GLOBALS->cachedtrace)
		{
		while(t)
			{
			if(!(t->flags&TR_HIGHLIGHT))
				{
				GLOBALS->cachedtrace = t;
			        if(CutBuffer())
			        	{
			                /* char buf[32];
			                sprintf(buf,"Dragging %d trace%s.\n",GLOBALS->traces.buffercount,GLOBALS->traces.buffercount!=1?"s":"");
			                status_text(buf); */
					must_update_screen = 1;
			                }

		                GLOBALS->cachedtrace->flags|=TR_HIGHLIGHT;
				goto success;
				}

			t=GivePrevTrace(t);
			}
		goto bot;
                }

success:
	if( ((which<0) && (GLOBALS->topmost_trace==GLOBALS->traces.first) && PrependBuffer()) || (PasteBuffer()) ) /* short circuit on special which<0 case */
        	{
                /* status_text("Drop completed.\n"); */

	        if(GLOBALS->cachedtrace)
	        	{
	                GLOBALS->cachedtrace->flags&=~TR_HIGHLIGHT;
	                }
   
		GLOBALS->signalwindow_width_dirty=1;
                MaxSignalLength();
                signalarea_configure_event(GLOBALS->signalarea, NULL);
                wavearea_configure_event(GLOBALS->wavearea, NULL);
		must_update_screen = 0;
                }
        }

bot:

if(must_update_screen)
	{
        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);
	}

GLOBALS->dnd_cursor_timer = 0;
GLOBALS->dnd_state = 0;
}

/*
 *	DND "drag_motion" handler, this is called whenever the 
 *	pointer is dragging over the target widget.
 */
static gboolean DNDDragMotionCB(
        GtkWidget *widget, GdkDragContext *dc,
        gint xx, gint yy, guint tt,
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

	GLOBALS->std_dnd_tgt_on_signalarea = (tar_widget == GLOBALS->signalarea);
	GLOBALS->std_dnd_tgt_on_wavearea = (tar_widget == GLOBALS->wavearea);

#ifdef WAVE_USE_GTK2
	/* If this is the same widget, our suggested action should be
	 * move.  For all other case we assume copy.
	 */
	suggested_action = GDK_ACTION_MOVE;

	/* Respond with default drag action (status). First we check
	 * the dc's list of actions. If the list only contains
	 * move, copy, or link then we select just that, otherwise we
	 * return with our default suggested action.
	 * If no valid actions are listed then we respond with 0.
	 */

        /* Only move? */
        if(dc->actions == GDK_ACTION_MOVE)
            gdk_drag_status(dc, GDK_ACTION_MOVE, tt);
        /* Only copy? */
        else if(dc->actions == GDK_ACTION_COPY)
            gdk_drag_status(dc, GDK_ACTION_COPY, tt);
        /* Only link? */
        else if(dc->actions == GDK_ACTION_LINK)
            gdk_drag_status(dc, GDK_ACTION_LINK, tt);
        /* Other action, check if listed in our actions list? */
        else if(dc->actions & suggested_action)
            gdk_drag_status(dc, suggested_action, tt);
        /* All else respond with 0. */
        else
            gdk_drag_status(dc, 0, tt);
#endif

if(GLOBALS->std_dnd_tgt_on_signalarea || GLOBALS->std_dnd_tgt_on_wavearea)
	{
	GtkAdjustment *wadj;
	GtkWidget *ddest;
	int which;
	gdouble x,y;
	GdkModifierType state;
	Trptr t;
	int trwhich, trtarget;
        
	#ifdef WAVE_USE_GTK2    
	gint xi, yi;
	#else
	GdkEventMotion event[1];
	event[0].deviceid = GDK_CORE_POINTER;
	#endif

        wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);

	WAVE_GDK_GET_POINTER(GLOBALS->std_dnd_tgt_on_signalarea ? GLOBALS->signalarea->window : GLOBALS->wavearea->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;

        which=(int)(y);
        which=(which/GLOBALS->fontheight)-2;
	if(which < -1) which = -1;

	trtarget=((int)wadj->value)+which;

	ddest = (GLOBALS->std_dnd_tgt_on_signalarea) ? GTK_WIDGET(GLOBALS->signalarea) : GTK_WIDGET(GLOBALS->wavearea);
	if((x<0)||(x>=ddest->allocation.width)||(y<0)||(y>=ddest->allocation.height))
		{
		goto bot;
		}

	t=GLOBALS->traces.first;
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

	while(t && t->t_next && IsGroupEnd(t->t_next) && IsCollapsed(t->t_next)) {
	  t = t->t_next;
	}

/* 	if(t) */
/* 		{ */
/* 		while(t) */
/* 			{ */
/* 			if(t->flags & TR_HIGHLIGHT) */
/* 				{ */
/* 				t=GivePrevTrace(t); */
/* 				which--; */
/* 				} */
/* 				else */
/* 				{ */
/* 				break; */
/* 				} */
/* 			} */
/* 		} */

	if(1)
		{
	        GtkAdjustment *hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
	        GtkAdjustment *sadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
	        int rsig_trtarget=(int)(sadj->value);
	        gint xsrc=(gint)hadj->value;
		gint ylin;

                gdk_draw_rectangle(GLOBALS->signalpixmap,
                        GLOBALS->gc_ltgray, TRUE, 0, 0,
                        GLOBALS->signal_fill_width, GLOBALS->signalarea->allocation.height);

		RenderSigs(rsig_trtarget, 0);

		GLOBALS->dnd_cursor_timer = 1;
		if((t)&&(which >= -1))
			{
			if(which >= GLOBALS->traces.total) { which = GLOBALS->traces.total-1; } 
			ylin = ((which + 2) * GLOBALS->fontheight) - 2;

		        gdk_draw_line(GLOBALS->signalpixmap, GLOBALS->gc_black,
	                	0, ylin, GLOBALS->signal_fill_width-1, ylin);
			}
			else
			{
			int i;

			which = -1;
			ylin = ((which + 2) * GLOBALS->fontheight) - 2;

			for(i=0;i<GLOBALS->signal_fill_width-1; i+=16)
				{
			        gdk_draw_line(GLOBALS->signalpixmap, GLOBALS->gc_black,
		                	i, ylin, i+7, ylin);
			        gdk_draw_line(GLOBALS->signalpixmap, GLOBALS->gc_white,
		                	i+8, ylin, i+15, ylin);
				}
			}

                gdk_draw_pixmap(GLOBALS->signalarea->window, GLOBALS->signalarea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->signalarea)],
                        GLOBALS->signalpixmap,
                        xsrc, 0,
                        0, 0,
                        GLOBALS->signalarea->allocation.width, GLOBALS->signalarea->allocation.height);

		/* printf("drop to %d of %d: '%s'\n", which, GLOBALS->traces.total, t ? t->name : "undef"); */
		}
	bot: return(FALSE);
	}

	return(FALSE);
}

static gboolean ignoreAccelerators(GdkEventKey *event)
{
if(!GLOBALS || !GLOBALS->filter_entry || !event)
	{
	return(FALSE);
	}
	else
	{
	return (GTK_WIDGET_HAS_FOCUS(GLOBALS->filter_entry) && 
	  !(event->state & GDK_CONTROL_MASK) &&
	  !(event->state & GDK_MOD1_MASK));
	}
}

/*
 * keypress processing, return TRUE to block the event from gtk
 */
static gint keypress_local(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
GtkAdjustment *wadj, *hadj;
int num_traces_displayable;
int target;
int which;
gint rc = FALSE;
int yscroll;

#ifdef FOCUS_DEBUG_MSGS
printf("focus: %d %08x %08x %08x\n", GTK_WIDGET_HAS_FOCUS(GLOBALS->signalarea_event_box), 
	GLOBALS->signalarea_event_box, widget, data);
#endif

if(GTK_WIDGET_HAS_FOCUS(GLOBALS->signalarea_event_box))
	{
	switch(event->keyval)
		{
		case GDK_Page_Up:
		case GDK_KP_Page_Up:
		case GDK_Page_Down:
		case GDK_KP_Page_Down:
		case GDK_Up:
		case GDK_KP_Up:
		case GDK_Down:
		case GDK_KP_Down:
			wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
			num_traces_displayable=(GLOBALS->signalarea->allocation.height)/(GLOBALS->fontheight);
			num_traces_displayable--;   /* for the time trace that is always there */

			if(num_traces_displayable<GLOBALS->traces.visible)
				{
				switch(event->keyval)
					{
					case GDK_Down:
					case GDK_KP_Down:
					case GDK_Page_Down:
					case GDK_KP_Page_Down:
						yscroll = ((event->keyval == GDK_Page_Down) || (event->keyval == GDK_KP_Page_Down)) ? num_traces_displayable : 1;
			                        target=((int)wadj->value)+yscroll;  
			                        which=num_traces_displayable-1;

			                        if(target+which>=(GLOBALS->traces.visible-1)) target=GLOBALS->traces.visible-which-1;
                        			wadj->value=target;

                        			if(GLOBALS->cachedwhich_signalwindow_c_1==which) GLOBALS->cachedwhich_signalwindow_c_1=which-1; /* force update */

                        			gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed"); /* force bar update */
                        			gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /* force text update */
						break;
				
					case GDK_Up:
					case GDK_KP_Up:
					case GDK_Page_Up:
					case GDK_KP_Page_Up:
						yscroll = ((event->keyval == GDK_Page_Up) || (event->keyval == GDK_KP_Page_Up)) ? num_traces_displayable : 1;
                        			target=((int)wadj->value)-yscroll;
                        			if(target<0) target=0;
                        			wadj->value=target;
                         
						which=0;
                        			if(GLOBALS->cachedwhich_signalwindow_c_1==which) GLOBALS->cachedwhich_signalwindow_c_1=-1; /* force update */

                        			gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed"); /* force bar update */
                        			gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /* force text update */
						break;
					}
				}
			rc = TRUE;
			break;

		case GDK_Left:
		case GDK_KP_Left:

			hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
  
			if(hadj->value < hadj->page_increment)
			        {
			        hadj->value = (gfloat)0.0;
			        }
				else
				{
				hadj->value = hadj->value - hadj->page_increment;
				}

			gtk_signal_emit_by_name (GTK_OBJECT (hadj), "changed");	/* force bar update */
			gtk_signal_emit_by_name (GTK_OBJECT (hadj), "value_changed"); /* force text update */
			signalarea_configure_event(GLOBALS->signalarea, NULL);

			rc = TRUE;
			break;

		case GDK_Right:
		case GDK_KP_Right:

			/* fill in left/right hscroll here */
			hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);

			if( ((int) hadj->value + hadj->page_increment) >= hadj->upper)
			        {
			        hadj->value = (gfloat)(hadj->upper)-hadj->page_increment;
			        }
				else
				{
				hadj->value = hadj->value + hadj->page_increment;
				}

			gtk_signal_emit_by_name (GTK_OBJECT (hadj), "changed");	/* force bar update */
			gtk_signal_emit_by_name (GTK_OBJECT (hadj), "value_changed"); /* force text update */
			signalarea_configure_event(GLOBALS->signalarea, NULL);

			rc = TRUE;
			break;
	
		default:
#ifdef FOCUS_DEBUG_MSGS
			printf("key %x, widget: %08x\n", event->keyval, widget);
#endif
			break;
		}
	}
else
if(GLOBALS->dnd_sigview)
	{
	  if(GTK_WIDGET_HAS_FOCUS(GLOBALS->dnd_sigview) || GTK_WIDGET_HAS_FOCUS(GLOBALS->filter_entry))
	    {
	      switch(event->keyval)
		{
		case GDK_a:
		  if(event->state & GDK_CONTROL_MASK)
		    {
		      treeview_select_all_callback();
		      rc = TRUE;
		    }
		  break;

		case GDK_A:
		  if(event->state & GDK_CONTROL_MASK)
		    {
		      treeview_unselect_all_callback();
		      rc = TRUE;
		    }
		default:
		  break;
		}
	    }
	else
	  if(GTK_WIDGET_HAS_FOCUS(GLOBALS->tree_treesearch_gtk2_c_1))
	    {
	      switch(event->keyval)
		{
		case GDK_a:
		  if(event->state & GDK_CONTROL_MASK)
		    {
		      /* eat keystroke */
		      rc = TRUE;
		    }
		  break;

		case GDK_A:
		  if(event->state & GDK_CONTROL_MASK)
		    {
		      /* eat keystroke */
		      rc = TRUE;
		    }
		default:
		  break;
		}
	    }
	}
 if (ignoreAccelerators(event)) {
   gtk_widget_event(GLOBALS->filter_entry, (GdkEvent *)event);
   /* eat keystroke */
   rc = TRUE; 
 }

return(rc);
}

#ifdef WAVE_USE_GTK2
static        gint  
scroll_event( GtkWidget * widget, GdkEventScroll * event )
{
  GdkEventKey ev_fake;

  DEBUG(printf("Mouse Scroll Event\n"));
  switch ( event->direction )
  {
    case GDK_SCROLL_UP:
      ev_fake.keyval = GDK_Up;
      keypress_local(widget, &ev_fake, GLOBALS->signalarea_event_box);
      break;
    case GDK_SCROLL_DOWN:
      ev_fake.keyval = GDK_Down;
      keypress_local(widget, &ev_fake, GLOBALS->signalarea_event_box);
      
    default:
      break;
  }
  return(TRUE);
}
#endif


static gboolean mouseover_timer(gpointer dummy)
{
static gboolean run_once = FALSE;
gdouble x,y;
GdkModifierType state;
TraceEnt t_trans;
                 
#ifdef WAVE_USE_GTK2
gint xi, yi;
#else
GdkEventMotion event[1];
event[0].deviceid = GDK_CORE_POINTER;  
#endif

execute_rpc();

if(GLOBALS->loaded_file_type == MISSING_FILE)
	{
	return(TRUE);
	}

if(run_once == FALSE) /* avoid any race conditions with the toolkit for uninitialized data */
	{
	run_once = TRUE;
	return(TRUE);
	}

if((!GLOBALS) || (!GLOBALS->signalarea) || (!GLOBALS->signalarea->window))                 
	{
	return(TRUE);
	}

if(GLOBALS->dnd_cursor_timer)
	{
	GLOBALS->dnd_cursor_timer++;
	if(GLOBALS->dnd_cursor_timer == 50)
		{
		GLOBALS->dnd_cursor_timer = 0;
	        signalarea_configure_event(GLOBALS->signalarea, NULL);
		}
	}

if(GLOBALS->mouseover_counter < 0) return(TRUE); /* mouseover is up in wave window so don't bother */

WAVE_GDK_GET_POINTER(GLOBALS->signalarea->window, &x, &y, &xi, &yi, &state);
WAVE_GDK_GET_POINTER_COPY;

GLOBALS->mouseover_counter++;

if(!((x>=0)&&(x<GLOBALS->signalarea->allocation.width)&&(y>=0)&&(y<GLOBALS->signalarea->allocation.height)))
	{
	move_mouseover_sigs(NULL, 0, 0, LLDescriptor(0));
	}
else
if(GLOBALS->mouseover_counter == 10)
	{
	int num_traces_displayable=GLOBALS->wavearea->allocation.height/(GLOBALS->fontheight);
	int yr = GLOBALS->cached_mouseover_y;
	int i;
	Trptr t=NULL;

	num_traces_displayable--;   /* for the time trace that is always there */

	yr-=GLOBALS->fontheight;
	if(yr<0) goto bot;
	yr/=GLOBALS->fontheight;             /* y now indicates the trace in question */
	if(yr>num_traces_displayable) goto bot;

	t=GLOBALS->topmost_trace;

	for(i=0;i<yr;i++)
	        {
	        if(!t) goto bot;
	        t=GiveNextTrace(t);
	        }
         
	if(!t) goto bot;
	if((t->flags&(/*TR_BLANK|*/TR_EXCLUDE))) /* TR_BLANK removed because of transaction handling below... */
	        {
	        t = NULL;
	        goto bot; 
	        }

if(t->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH))  /* seek to real xact trace if present... */
        {
        Trptr tscan = t;
        int bcnt = 0;
        while((tscan) && (tscan = GivePrevTrace(tscan)))
                {
                if(!(tscan->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
                        {
                        if(tscan->flags & TR_TTRANSLATED)
                                {
                                break; /* found it */
                                }
                                else
                                {
                                tscan = NULL;
                                }
                        }
                        else
                        {
                        bcnt++; /* bcnt is number of blank traces */
                        }
                }

        if((tscan)&&(tscan->vector))
                {
                bvptr bv = tscan->n.vec;
                do
                        {
                        bv = bv->transaction_chain; /* correlate to blank trace */
                        } while(bv && (bcnt--));
                if(bv)
                        {
                        memcpy(&t_trans, tscan, sizeof(TraceEnt)); /* substitute into a synthetic trace */
                        t_trans.n.vec = bv;
			t_trans.vector = 1;

                        t_trans.name = bv->bvname;
                        if(GLOBALS->hier_max_level)
                                t_trans.name = hier_extract(t_trans.name, GLOBALS->hier_max_level);

                        t = &t_trans;
                        goto bot; /* is goto process_trace; in wavewindow.c */
                        }
                }
        }        
                
if((t->flags&TR_BLANK))
        {                
        t = NULL;
        goto bot;       
        }
	         
	if(t->flags & TR_ANALOG_BLANK_STRETCH)  /* seek to real analog trace is present... */
	        {
	        while((t) && (t = t->t_prev))
	                {
	                if(!(t->flags & TR_ANALOG_BLANK_STRETCH))
	                        {
	                        if(t->flags & TR_ANALOGMASK)
	                                {
	                                break; /* found it */
	                                }
	                                else
	                                {   
	                                t = NULL;
	                                }
	                        }
	                }
	        }

bot:
	if(t)
		{
		move_mouseover_sigs(t, GLOBALS->cached_mouseover_x, GLOBALS->cached_mouseover_y, GLOBALS->tims.marker);
		}
		else
		{
		move_mouseover_sigs(NULL, 0, 0, LLDescriptor(0));
		}
	}

return(TRUE);
}

static gint motion_notify_event_std(GtkWidget *widget, GdkEventMotion *event)
{
gdouble x,y;
GdkModifierType state;

#ifdef WAVE_USE_GTK2
gint xi, yi;
#endif

if(event->is_hint)
	{
	WAVE_GDK_GET_POINTER(event->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;
	}
	else
	{
	x = event->x;
	y = event->y;
	state = event->state;
	}

GLOBALS->cached_mouseover_x = x;
GLOBALS->cached_mouseover_y = y;
GLOBALS->mouseover_counter = 0;

move_mouseover_sigs(NULL, 0, 0, LLDescriptor(0));

return(TRUE);
}


static gint button_release_event_std(GtkWidget *widget, GdkEventButton *event)
{
if(GLOBALS->std_collapse_pressed)
	{
	GLOBALS->std_collapse_pressed = 0;
	}

return(TRUE);
}


static gint button_press_event_std(GtkWidget *widget, GdkEventButton *event)
{
int num_traces_displayable;
int which;
int trwhich, trtarget;
GtkAdjustment *wadj;
Trptr t, t2;

if(GLOBALS->signalarea_event_box)
	{

	  /* Don't mess with highlights with button 2 (save for dnd) */
	  if((event->button == 2) && (event->type == GDK_BUTTON_PRESS))
	    {
	      return(TRUE);
	    }

	  /* Don't mess with highlights with button 3 (save for menu_check) */
	  if((event->button == 3) && (event->type == GDK_BUTTON_PRESS))
	    {
	      goto menu_chk;
	    }
	if((event->x<0)||(event->x>=widget->allocation.width)||(event->y<0)||(event->y>=widget->allocation.height))
		{
		/* let gtk take focus from us with focus out event */
		}
		else
		{
		if(!GLOBALS->signalarea_has_focus)
			{
			GLOBALS->signalarea_has_focus = TRUE;
			gtk_widget_grab_focus(GTK_WIDGET(GLOBALS->signalarea_event_box));
			}
		}
	}

if((GLOBALS->traces.visible)&&(GLOBALS->signalpixmap))
	{
	num_traces_displayable=widget->allocation.height/(GLOBALS->fontheight);
	num_traces_displayable--;   /* for the time trace that is always there */

	which=(int)(event->y);
	which=(which/GLOBALS->fontheight)-1;

	if(which>=GLOBALS->traces.visible)
		{
		if((event->state&(GDK_CONTROL_MASK|GDK_SHIFT_MASK)) == (GDK_SHIFT_MASK))
			{
			/* ok for plain-vanilla shift click only */
			which = GLOBALS->traces.visible-1;
			}
			else
			{
			  ClearTraces();
			  goto redraw; /* off in no man's land */
			}
		}

	if((which>=num_traces_displayable)||(which<0))
		{
		  ClearTraces();
		  goto redraw; /* off in no man's land */
		}

	wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
	trtarget=((int)wadj->value)+which;

	t=GLOBALS->traces.first;
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

	if(event->state&GDK_CONTROL_MASK)
		{
		  if(IsGroupBegin(t) && IsSelected(t))
		    {
		      ClearGroupTraces(t);
		    }
		  else if(IsGroupEnd(t) && IsSelected(t))
		    {
		      ClearGroupTraces(t->t_match);
		    }
		  else
		    {
		      t->flags ^= TR_HIGHLIGHT;
		    }
		}
	else
	if((event->state&GDK_SHIFT_MASK)&&(GLOBALS->starting_unshifted_trace))
		{
		int src = -1, dst = -1;
		int cnt = 0;

		t2=GLOBALS->traces.first;
		while(t2)
			{
			if(t2 == t) { dst = cnt; }
			if(t2 == GLOBALS->starting_unshifted_trace) { src = cnt; }

			cnt++;

			/*			t2->flags &= ~TR_HIGHLIGHT; */
			t2 = t2->t_next;
			}

		if(src != -1)
			{
			  cnt = 0;
			  t2=GLOBALS->traces.first;
			  while(t2)
				{
				  if ((cnt == src) && (cnt == dst) && IsSelected(t2))
				    {
				      GLOBALS->starting_unshifted_trace = NULL;
				    }
				  t2->flags &= ~TR_HIGHLIGHT;
				  t2=t2->t_next;
				  cnt++;
				}

			if(src > dst) { int cpy; cpy = src; src = dst; dst = cpy; }
			cnt = 0;
			t2=GLOBALS->traces.first;
			while(t2 && GLOBALS->starting_unshifted_trace)
				{
				if((cnt >= src) && (cnt <= dst))
					{
					  t2->flags |= TR_HIGHLIGHT;
					}

				cnt++;
				t2=t2->t_next;
				}
			}
			else
			{
			GLOBALS->starting_unshifted_trace = t;
			t->flags |= TR_HIGHLIGHT;
			}
		}
	/*	else if(!(t->flags & TR_HIGHLIGHT)) Ben Sferrazza suggested fix rather than a regular "else" */
else
		{
		GLOBALS->starting_unshifted_trace = t;

		t2=GLOBALS->traces.first;
		while(t2)
			{
			t2->flags &= ~TR_HIGHLIGHT;
			t2 = t2->t_next;
			}

		t->flags |= TR_HIGHLIGHT;
		}

	if(event->type == GDK_2BUTTON_PRESS)
	  {
	    menu_toggle_group(widget, NULL);
	    goto menu_chk;
	  }

	redraw:

	GLOBALS->signalwindow_width_dirty=1;
        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);
	}


menu_chk:
if((event->button == 3) && (event->type == GDK_BUTTON_PRESS))
	{
      	do_popup_menu (widget, event);
    	}

return(TRUE);
}

/***  standard click routines turned on with "use_standard_clicking"=1  ***/
/**************************************************************************/


/**************************************************************************/
/***  standard click routines turned on with "use_standard_clicking"=0  ***/
/***                                                                    ***/
/***                        no longer supported                         ***/
/***                                                                    ***/
/***  gtkwave click routines turned on with "use_standard_clicking"=0   ***/
/**************************************************************************/


gint signalarea_configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
GtkAdjustment *wadj, *hadj;
int num_traces_displayable;
int width;

if((!widget)||(!widget->window)) return(TRUE);

make_sigarea_gcs(widget);
UpdateTracesVisible();

num_traces_displayable=widget->allocation.height/(GLOBALS->fontheight);
num_traces_displayable--;   /* for the time trace that is always there */


DEBUG(printf("SigWin Configure Event h: %d, w: %d\n",
		widget->allocation.height,
		widget->allocation.width));

GLOBALS->old_signal_fill_width=GLOBALS->signal_fill_width;
GLOBALS->signal_fill_width = ((width=widget->allocation.width) > GLOBALS->signal_pixmap_width)
        ? widget->allocation.width : GLOBALS->signal_pixmap_width;

if(GLOBALS->signalpixmap)
	{
	if((GLOBALS->old_signal_fill_width!=GLOBALS->signal_fill_width)||(GLOBALS->old_signal_fill_height!=widget->allocation.height))
		{
		gdk_pixmap_unref(GLOBALS->signalpixmap);
		GLOBALS->signalpixmap=gdk_pixmap_new(widget->window, 
			GLOBALS->signal_fill_width, widget->allocation.height, -1);
		}
	}
	else
	{
	GLOBALS->signalpixmap=gdk_pixmap_new(widget->window, 
		GLOBALS->signal_fill_width, widget->allocation.height, -1);
	}

GLOBALS->old_signal_fill_height= widget->allocation.height;
gdk_draw_rectangle(GLOBALS->signalpixmap, widget->style->bg_gc[GTK_STATE_PRELIGHT], TRUE, 0, 0,
			GLOBALS->signal_fill_width, widget->allocation.height);

hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
hadj->page_size=hadj->page_increment=(gfloat)width;
hadj->step_increment=(gfloat)10.0;  /* approx 1ch at a time */
hadj->lower=(gfloat)0.0;
hadj->upper=(gfloat)GLOBALS->signal_pixmap_width;

if( ((int)hadj->value)+width > GLOBALS->signal_fill_width)
	{
	hadj->value = (gfloat)(GLOBALS->signal_fill_width-width);
	}


wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
wadj->page_size=wadj->page_increment=(gfloat) num_traces_displayable;
wadj->step_increment=(gfloat)1.0;
wadj->lower=(gfloat)0.0;
wadj->upper=(gfloat)(GLOBALS->traces.visible ? GLOBALS->traces.visible : 1);

if(GLOBALS->traces.scroll_bottom)
	{
	Trptr t = GLOBALS->traces.first;
	int which = 0;
	int scroll_top = -1, scroll_bottom = -1;
	int cur_top = wadj->value;
	int cur_bottom = cur_top + num_traces_displayable - 1;

	while(t)
		{
		if(t == GLOBALS->traces.scroll_top)
			{
			scroll_top = which;
			}

		if(t == GLOBALS->traces.scroll_bottom)
			{
			scroll_bottom = which;
			break;
			}

		t = GiveNextTrace(t);
		which++;
		}

	GLOBALS->traces.scroll_top = GLOBALS->traces.scroll_bottom = NULL;

	if((scroll_top >= 0) && (scroll_bottom >= 0))
		{
		if((scroll_top > cur_top) && (scroll_bottom <= cur_bottom))
			{
			/* nothing */
			}	
			else
			{
			if((scroll_bottom - scroll_top + 1) >= num_traces_displayable)			
				{
				wadj->value=(gfloat)(scroll_bottom - num_traces_displayable + 1);
				}
				else
				{
				int midpoint = (cur_top + cur_bottom) / 2;

				if(scroll_top <= cur_top)
					{
					wadj->value=(gfloat)scroll_top-1;
					}
				else if(scroll_top >= cur_bottom)
					{
					wadj->value=(gfloat)(scroll_bottom - num_traces_displayable + 1);
					}
				else
				if(scroll_top < midpoint)
					{
					wadj->value=(gfloat)scroll_top-1;
					}
				else
					{
					wadj->value=(gfloat)(scroll_bottom - num_traces_displayable + 1);
					}
				}

			if(wadj->value < 0.0) wadj->value = 0.0;
			}
		}
	}

if(num_traces_displayable>GLOBALS->traces.visible)
	{
	wadj->value=(gfloat)(GLOBALS->trtarget_signalwindow_c_1=0);
	}
	else
	if (wadj->value + num_traces_displayable > GLOBALS->traces.visible)
	{
	wadj->value=(gfloat)(GLOBALS->trtarget_signalwindow_c_1=GLOBALS->traces.visible-num_traces_displayable);
	}

gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed");	/* force bar update */
gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /* force text update */

gtk_signal_emit_by_name (GTK_OBJECT (hadj), "changed");	/* force bar update */

return(TRUE);
}

static gint signalarea_configure_event_local(GtkWidget *widget, GdkEventConfigure *event)
{
gint rc;
gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(GLOBALS->notebook));
struct Global *g_old = GLOBALS;

set_GLOBALS((*GLOBALS->contexts)[page_num]);

rc = signalarea_configure_event(widget, event);

set_GLOBALS(g_old);

return(rc);
}


static gint expose_event(GtkWidget *widget, GdkEventExpose *event)
{
GtkAdjustment *hadj;
int xsrc;

hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
xsrc=(gint)hadj->value;

gdk_draw_pixmap(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
		GLOBALS->signalpixmap, 
		xsrc+event->area.x, event->area.y,
		event->area.x, event->area.y,
		event->area.width, event->area.height);

draw_signalarea_focus();

return(FALSE);
}

static gint expose_event_local(GtkWidget *widget, GdkEventExpose *event)
{
gint rc;
gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(GLOBALS->notebook));
/* struct Global *g_old = GLOBALS; */

set_GLOBALS((*GLOBALS->contexts)[page_num]);

rc = expose_event(widget, event);

/* seems to cause a conflict flipping back so don't! */
/* set_GLOBALS(g_old); */

return(rc);
}


static int focus_in_local(GtkWidget *widget, GdkEventFocus *event)
{
#ifdef FOCUS_DEBUG_MSGS
printf("Focus in: %08x %08x\n", widget, GLOBALS->signalarea_event_box);
#endif

GLOBALS->signalarea_has_focus = TRUE;

signalarea_configure_event(GLOBALS->signalarea, NULL);

return(FALSE);
}

static int focus_out_local(GtkWidget *widget, GdkEventFocus *event)
{
#ifdef FOCUS_DEBUG_MSGS
printf("Focus out: %08x\n", widget);
#endif

GLOBALS->signalarea_has_focus = FALSE;

signalarea_configure_event(GLOBALS->signalarea, NULL);

return(FALSE);
}

GtkWidget *
create_signalwindow(void)
{
GtkWidget *table;
GtkWidget *frame;
char do_focusing = 0;

table = gtk_table_new(10, 10, FALSE);

GLOBALS->signalarea=gtk_drawing_area_new();

gtk_widget_show(GLOBALS->signalarea);
MaxSignalLength();

gtk_widget_set_events(GLOBALS->signalarea, 
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
		GDK_BUTTON_RELEASE_MASK | 
		GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK
		);

gtk_signal_connect(GTK_OBJECT(GLOBALS->signalarea), "configure_event", GTK_SIGNAL_FUNC(signalarea_configure_event_local), NULL);
gtk_signal_connect(GTK_OBJECT(GLOBALS->signalarea), "expose_event",GTK_SIGNAL_FUNC(expose_event_local), NULL);

sclick:
if(GLOBALS->use_standard_clicking)
	{
	GtkTargetEntry target_entry[3];

        target_entry[0].target = WAVE_DRAG_TAR_NAME_0;
        target_entry[0].flags = 0;
        target_entry[0].info = WAVE_DRAG_TAR_INFO_0;
        target_entry[1].target = WAVE_DRAG_TAR_NAME_1;
        target_entry[1].flags = 0;
        target_entry[1].info = WAVE_DRAG_TAR_INFO_1;
        target_entry[2].target = WAVE_DRAG_TAR_NAME_2;
        target_entry[2].flags = 0;
        target_entry[2].info = WAVE_DRAG_TAR_INFO_2;

        gtk_drag_dest_set(
        	GTK_WIDGET(GLOBALS->signalarea),
                GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT |
                GTK_DEST_DEFAULT_DROP,
                target_entry,
                sizeof(target_entry) / sizeof(GtkTargetEntry),
		GDK_ACTION_MOVE
                );

        gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signalarea), "drag_motion", GTK_SIGNAL_FUNC(DNDDragMotionCB), GTK_WIDGET(GLOBALS->signalarea));
        gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signalarea), "drag_begin", GTK_SIGNAL_FUNC(DNDBeginCB), GTK_WIDGET(GLOBALS->signalarea));
        gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signalarea), "drag_end", GTK_SIGNAL_FUNC(DNDEndCB), GTK_WIDGET(GLOBALS->signalarea));

        gtk_drag_dest_set(
        	GTK_WIDGET(GLOBALS->wavearea),
                GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT |
                GTK_DEST_DEFAULT_DROP,
                target_entry,
                sizeof(target_entry) / sizeof(GtkTargetEntry),
		GDK_ACTION_MOVE
                );

        gtkwave_signal_connect(GTK_OBJECT(GLOBALS->wavearea), "drag_motion", GTK_SIGNAL_FUNC(DNDDragMotionCB), GTK_WIDGET(GLOBALS->wavearea));
        gtkwave_signal_connect(GTK_OBJECT(GLOBALS->wavearea), "drag_begin", GTK_SIGNAL_FUNC(DNDBeginCB), GTK_WIDGET(GLOBALS->wavearea));
        gtkwave_signal_connect(GTK_OBJECT(GLOBALS->wavearea), "drag_end", GTK_SIGNAL_FUNC(DNDEndCB), GTK_WIDGET(GLOBALS->wavearea));

	gtk_drag_source_set(GTK_WIDGET(GLOBALS->signalarea),
        	GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                target_entry,
                sizeof(target_entry) / sizeof(GtkTargetEntry),
                GDK_ACTION_PRIVATE);

	gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signalarea), "button_press_event",GTK_SIGNAL_FUNC(button_press_event_std), NULL);
	gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signalarea), "button_release_event", GTK_SIGNAL_FUNC(button_release_event_std), NULL);
	gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signalarea), "motion_notify_event",GTK_SIGNAL_FUNC(motion_notify_event_std), NULL);
	g_timeout_add(100, mouseover_timer, NULL);

#ifdef WAVE_USE_GTK2
	gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signalarea), "scroll_event",GTK_SIGNAL_FUNC(scroll_event), NULL);
#endif
	do_focusing = 1;
	}
	else
	{
	fprintf(stderr, "GTKWAVE | \"use_standard_clicking off\" has been removed.\n");
	fprintf(stderr, "GTKWAVE | Please update your rc files accordingly.\n");
	GLOBALS->use_standard_clicking = 1;
	goto sclick;
	}

gtk_table_attach (GTK_TABLE (table), GLOBALS->signalarea, 0, 10, 0, 9,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 3, 2);

GLOBALS->signal_hslider=gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signal_hslider), "value_changed",GTK_SIGNAL_FUNC(service_hslider), NULL);
GLOBALS->hscroll_signalwindow_c_1=gtk_hscrollbar_new(GTK_ADJUSTMENT(GLOBALS->signal_hslider));
gtk_widget_show(GLOBALS->hscroll_signalwindow_c_1);
gtk_table_attach (GTK_TABLE (table), GLOBALS->hscroll_signalwindow_c_1, 0, 10, 9, 10,
                        GTK_FILL,
                        GTK_FILL | GTK_SHRINK, 3, 4);
gtk_widget_show(table);

frame=gtk_frame_new("Signals");
gtk_container_border_width(GTK_CONTAINER(frame),2);

gtk_container_add(GTK_CONTAINER(frame),table);

if(do_focusing)
	{
	GLOBALS->signalarea_event_box = gtk_event_box_new();
	gtk_container_add (GTK_CONTAINER (GLOBALS->signalarea_event_box), frame);
	gtk_widget_show(frame);
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET(GLOBALS->signalarea_event_box), GTK_CAN_FOCUS | GTK_RECEIVES_DEFAULT);
	gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signalarea_event_box), "focus_in_event", GTK_SIGNAL_FUNC(focus_in_local), NULL);
	gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signalarea_event_box), "focus_out_event", GTK_SIGNAL_FUNC(focus_out_local), NULL);

	/* not necessary for now... */
	/* gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signalarea_event_box), "popup_menu",GTK_SIGNAL_FUNC(popup_event), NULL); */

	if(!GLOBALS->second_page_created)
		{
	        if(!GLOBALS->keypress_handler_id)
	                {
			GLOBALS->keypress_handler_id = install_keypress_handler();
	                }
		}

	return(GLOBALS->signalarea_event_box);
	}
	else
	{
	return(frame);
	}
}


gint install_keypress_handler(void)
{
gint rc = 
	gtk_signal_connect(GTK_OBJECT(GLOBALS->mainwindow), 
	"key_press_event",GTK_SIGNAL_FUNC(keypress_local), NULL);

return(rc);
}


void remove_keypress_handler(gint id)
{
gtk_signal_disconnect(GTK_OBJECT(GLOBALS->mainwindow), id);
}


/*
 * $Id$
 * $Log$
 * Revision 1.50  2010/04/14 07:49:02  gtkwave
 * updated mouseover handling
 *
 * Revision 1.49  2010/02/28 19:05:15  gtkwave
 * missing null pointer guard added
 *
 * Revision 1.48  2009/12/24 20:55:27  gtkwave
 * warnings cleanups
 *
 * Revision 1.47  2009/10/30 18:05:51  gtkwave
 * removed use_standard_clicking off click semantics
 *
 * Revision 1.46  2009/10/24 01:51:41  gtkwave
 * added dynamic loading for tcl/tk via --enable-stubify
 *
 * Revision 1.45  2009/09/27 23:46:53  gtkwave
 * crash fix on DnD drop
 *
 * Revision 1.44  2009/09/14 03:00:08  gtkwave
 * bluespec code integration
 *
 * Revision 1.43  2009/04/09 22:17:21  gtkwave
 * install_keypress_handler() fixes
 *
 * Revision 1.42  2009/03/27 04:58:14  gtkwave
 * enable splash screen during empty gui handling / load dumpfile
 *
 * Revision 1.41  2009/03/24 21:31:34  gtkwave
 * changed dkgray to mdgray for "Time" background as intensity changed
 *
 * Revision 1.40  2008/12/25 03:21:57  gtkwave
 * -Wshadow warning fixes
 *
 * Revision 1.39  2008/12/20 19:42:11  gtkwave
 * add static initializer to timer functions to avoid non-initialized viewer
 * state when first starting (if timer interrupt fires)
 *
 * Revision 1.38  2008/12/20 05:45:03  gtkwave
 * gtk1 compatibility and -Wshadow warning fixes
 *
 * Revision 1.37  2008/12/18 01:31:30  gtkwave
 * integrated experimental autoscroll code on signal adds
 *
 * Revision 1.36  2008/12/17 16:22:31  gtkwave
 * removed clearing of dnd timer in tree drag code
 *
 * Revision 1.35  2008/12/16 18:09:49  gtkwave
 * add countdown timer to remove dnd cursor in signalwindow
 *
 * Revision 1.34  2008/12/09 00:36:42  gtkwave
 * added mouseover support for signal window
 *
 * Revision 1.33  2008/08/18 16:10:54  gtkwave
 * adding sticky click semantics on already selected entries
 *
 * Revision 1.32  2008/07/01 18:51:07  gtkwave
 * compiler warning fixes for amd64
 *
 * Revision 1.31  2008/06/11 08:01:55  gtkwave
 * gcc 4.3.x compiler warning fixes
 *
 * Revision 1.30  2008/03/26 01:23:52  gtkwave
 * reduce drag and drop visual noise
 *
 * Revision 1.29  2008/03/25 17:42:55  gtkwave
 * added d&d visual feedback
 *
 * Revision 1.28  2008/02/24 02:47:33  gtkwave
 * minor adjustment to shift-clicking when signal window not full
 *
 * Revision 1.27  2008/02/08 02:26:36  gtkwave
 * anti-aliased font support add
 *
 * Revision 1.26  2008/01/30 21:12:22  gtkwave
 * dnd for gtk1
 *
 * Revision 1.25  2008/01/23 16:37:56  gtkwave
 * created separate smaller popup instead of using normal itemfactory list
 *
 * Revision 1.24  2008/01/23 11:07:19  gtkwave
 * integration of preliminary signalwindow popup menu code
 *
 * Revision 1.23  2008/01/12 05:04:37  gtkwave
 * scrollwheel support
 *
 * Revision 1.22  2008/01/09 19:20:53  gtkwave
 * more updating to globals management (expose events cause wrong swap)
 *
 * Revision 1.21  2008/01/09 04:09:11  gtkwave
 * fix keyboard focus sighandler when multi-tabs are being used
 *
 * Revision 1.20  2008/01/08 18:21:23  gtkwave
 * focus in/out rendering improvements
 *
 * Revision 1.19  2008/01/08 07:13:08  gtkwave
 * more limiting of ctrl-a focus (away from tree and filter entry)
 *
 * Revision 1.18  2008/01/08 04:01:12  gtkwave
 * more accelerator key ergonomic updates
 *
 * Revision 1.17  2008/01/05 22:25:46  gtkwave
 * degate busy during treeview dnd as it disrupts focus; dnd cleanups
 *
 * Revision 1.16  2008/01/04 22:47:56  gtkwave
 * prelim input focus support for singalwindow
 *
 * Revision 1.15  2008/01/04 04:03:14  gtkwave
 * disable dnd for 1.3.
 *
 * Revision 1.14  2008/01/04 03:23:33  gtkwave
 * have dormant key_press_event handler code.
 *
 * Revision 1.13  2008/01/03 21:55:45  gtkwave
 * various cleanups
 *
 * Revision 1.12  2008/01/03 05:02:14  gtkwave
 * added dnd into wavewindow for both click modes
 *
 * Revision 1.11  2008/01/03 02:04:52  gtkwave
 * more dnd ergonomics
 *
 * Revision 1.10  2008/01/03 00:09:17  gtkwave
 * preliminary dnd support for use_standard_clicking mode
 *
 * Revision 1.9  2008/01/02 22:12:25  gtkwave
 * added collapsible groups to standard click semantics via shift+ctrl
 *
 * Revision 1.8  2008/01/02 18:17:26  gtkwave
 * added standard click semantics with user_standard_clicking rc variable
 *
 * Revision 1.7  2007/09/17 16:00:51  gtkwave
 * yet more stability updates for tabbed viewing
 *
 * Revision 1.6  2007/09/14 16:23:17  gtkwave
 * remove expose events from ctx management
 *
 * Revision 1.5  2007/09/13 21:24:45  gtkwave
 * configure_events must be beyond watchdog monitoring due to how gtk generates one per tab
 *
 * Revision 1.4  2007/09/12 17:26:45  gtkwave
 * experimental ctx_swap_watchdog added...still tracking down mouse thrash crashes
 *
 * Revision 1.3  2007/09/10 19:43:20  gtkwave
 * gtk1.2 compile fixes
 *
 * Revision 1.2  2007/08/26 21:35:44  gtkwave
 * integrated global context management from SystemOfCode2007 branch
 *
 * Revision 1.1.1.1.2.6  2007/08/07 03:18:55  kermin
 * Changed to pointer based GLOBAL structure and added initialization function
 *
 * Revision 1.1.1.1.2.5  2007/08/06 03:50:48  gtkwave
 * globals support for ae2, gtk1, cygwin, mingw.  also cleaned up some machine
 * generated structs, etc.
 *
 * Revision 1.1.1.1.2.4  2007/08/05 02:27:23  kermin
 * Semi working global struct
 *
 * Revision 1.1.1.1.2.3  2007/07/31 03:18:01  kermin
 * Merge Complete - I hope
 *
 * Revision 1.1.1.1.2.2  2007/07/28 19:50:40  kermin
 * Merged in the main line
 *
 * Revision 1.1.1.1  2007/05/30 04:27:50  gtkwave
 * Imported sources
 *
 * Revision 1.2  2007/04/20 02:08:17  gtkwave
 * initial release
 *
 */

