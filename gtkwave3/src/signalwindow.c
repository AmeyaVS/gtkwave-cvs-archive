/* 
 * Copyright (c) Tony Bybell 1999-2008.
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

	gdk_draw_rectangle(GLOBALS->signalpixmap, GLOBALS->gc_dkgray, TRUE,
	        0, -1, GLOBALS->signal_fill_width, GLOBALS->fontheight);
	gdk_draw_line(GLOBALS->signalpixmap, GLOBALS->gc_white,  
	        0, GLOBALS->fontheight-1, GLOBALS->signal_fill_width-1, GLOBALS->fontheight-1);
	gdk_draw_string(GLOBALS->signalpixmap, GLOBALS->signalfont,
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
if((widget == NULL) || (dc == NULL))
	return;

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

			                MaxSignalLength();
			                signalarea_configure_event(GLOBALS->signalarea, NULL);
			                wavearea_configure_event(GLOBALS->wavearea, NULL);
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
                }
        }

bot:
GLOBALS->dnd_state = 0;
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

	GLOBALS->std_dnd_tgt_on_signalarea = (tar_widget == GLOBALS->signalarea);
	GLOBALS->std_dnd_tgt_on_wavearea = (tar_widget == GLOBALS->wavearea);

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

if(GLOBALS->signalarea_event_box != data)
	{
#ifdef FOCUS_DEBUG_MSGS
	printf("wrong session\n");
#endif
	}
else
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

if((event->button == 3) && (event->type == GDK_BUTTON_PRESS))
	{
      	do_popup_menu (widget, event);
      	return TRUE;
    	}

if(GLOBALS->signalarea_event_box)
	{
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

	if((which>=GLOBALS->traces.visible)||(which>=num_traces_displayable)||(which<0))
		{
		return(TRUE); /* off in no man's land */
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

	if((event->state&(GDK_CONTROL_MASK|GDK_SHIFT_MASK)) == (GDK_CONTROL_MASK|GDK_SHIFT_MASK))
		{
		/* frankel add */
		if(t)
			{
			if((!GLOBALS->std_collapse_pressed) && CollapseTrace(t))
				{
				GLOBALS->signalwindow_width_dirty=1;
	        		MaxSignalLength();
	        		signalarea_configure_event(GLOBALS->signalarea, NULL);
	        		wavearea_configure_event(GLOBALS->wavearea, NULL);
				GLOBALS->std_collapse_pressed = 1;
				}
			return(TRUE);
			}
		}

	if(event->state&GDK_CONTROL_MASK)
		{
		t->flags ^= TR_HIGHLIGHT;
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

			t2->flags &= ~TR_HIGHLIGHT;
			t2 = t2->t_next;
			}

		if(src != -1)
			{
			int cpy;

			if(src > dst) { cpy = src; src = dst; dst = cpy; }
			cnt = 0;
			t2=GLOBALS->traces.first;
			while(t2)
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


	GLOBALS->signalwindow_width_dirty=1;
        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);
	}

}

/***  standard click routines turned on with "use_standard_clicking"=1  ***/
/**************************************************************************/


/**************************************************************************/
/***  gtkwave click routines turned on with "use_standard_clicking"=0   ***/

static gint motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
int num_traces_displayable;
int which;
int trwhich, trtarget;
int new_cachedwhich;
GtkAdjustment *wadj;
Trptr t;
gdouble x,y;
GdkModifierType state;
int doloop;

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

top: doloop=0;

/********************* button 1 *********************/

if((GLOBALS->traces.visible)&&(state&(GDK_BUTTON1_MASK|GDK_BUTTON3_MASK))&&(GLOBALS->signalpixmap)&&(GLOBALS->cachedwhich_signalwindow_c_1!=-1))
	{
	num_traces_displayable=widget->allocation.height/(GLOBALS->fontheight);
	num_traces_displayable--;   /* for the time trace that is always there */

	which=(int)(y);
	which=(which/GLOBALS->fontheight)-1;

	if(which>=GLOBALS->traces.visible)
		{
		which=GLOBALS->traces.visible-1;
		}


	wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);

	if(num_traces_displayable<GLOBALS->traces.visible)
		{
		if(which>=num_traces_displayable)
			{
			int target;

			target=((int)wadj->value)+1;
			which=num_traces_displayable-1;

			if(target+which>=(GLOBALS->traces.visible-1)) target=GLOBALS->traces.visible-which-1;
			wadj->value=target;

			if(GLOBALS->cachedwhich_signalwindow_c_1==which) GLOBALS->cachedwhich_signalwindow_c_1=which-1; /* force update */

			gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed");	/* force bar update */
			gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /* force text update */
	
			doloop=1;
			}
			else
			if (which<0)
			{
			int target;
	
			target=((int)wadj->value)-1;
			if(target<0) target=0;
			wadj->value=target;
	
			which=0;
			if(GLOBALS->cachedwhich_signalwindow_c_1==which) GLOBALS->cachedwhich_signalwindow_c_1=-1; /* force update */
	
			gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed");	/* force bar update */
			gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /* force text update */
	
			doloop=1;
			}
		}
		else
		{
		if(which<0) which=0;
		}

	trtarget=((int)wadj->value)+which;

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

	GLOBALS->cachedtrace=t;

	if((GLOBALS->dnd_state==1)&&(state&GDK_BUTTON3_MASK))
		{
		GtkAdjustment *hadj;
		gint xsrc;
		int yval;

		hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
		wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
		gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed");	/* force bar update */
		gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /* force text update */

		xsrc=(gint)hadj->value;

		yval=RenderSig(t, which, 2);
        	gdk_draw_pixmap(GLOBALS->signalarea->window, GLOBALS->signalarea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->signalarea)],GLOBALS->signalpixmap,xsrc, yval,0, yval,GLOBALS->signalarea->allocation.width, GLOBALS->fontheight-1);
		}
	else
	if((state&GDK_BUTTON1_MASK)&&(GLOBALS->dnd_state==0))
	if((t)&&(which!=GLOBALS->cachedwhich_signalwindow_c_1))
		{
		GtkAdjustment *hadj;
		gint xsrc;

		hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
		xsrc=(gint)hadj->value;

		new_cachedwhich=which;	/* save so fill ins go from deltas in the future */
		
		do {
		int oldflags;

		oldflags = t->flags;

		t->flags = (t->flags & (~TR_HIGHLIGHT)) | GLOBALS->cachedhiflag_signalwindow_c_1;

		if(oldflags!=t->flags)
			{
			int yval;
			DEBUG(printf("Motion highlight swap in signalarea at x: %d, y: %d row: %d\n",
				(int)x, (int)y, which));

			yval=RenderSig(t, which, 1);
        		gdk_draw_pixmap(GLOBALS->signalarea->window, GLOBALS->signalarea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->signalarea)],GLOBALS->signalpixmap,xsrc, yval,0, yval,GLOBALS->signalarea->allocation.width, GLOBALS->fontheight-1);
			}

		if(which>GLOBALS->cachedwhich_signalwindow_c_1)
			{
			which--;
			t=GivePrevTrace(t);
			}
		else if(which<GLOBALS->cachedwhich_signalwindow_c_1)
			{
			which++;
			t=GiveNextTrace(t);
			}

		} while((which!=GLOBALS->cachedwhich_signalwindow_c_1)&&(t));

		GLOBALS->cachedwhich_signalwindow_c_1=new_cachedwhich;	/* for next time around */
		}
	}

if(doloop)
	{
	WAVE_GDK_GET_POINTER(event->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;
	goto top;
	}

return(TRUE);
}


static gint button_release_event(GtkWidget *widget, GdkEventButton *event)
{
int which;

if(event->button==1)
	{
	if(GLOBALS->dnd_state==0) GLOBALS->cachedwhich_signalwindow_c_1=-1;
	gdk_pointer_ungrab(event->time);
	DEBUG(printf("Button 1 released\n"));
	}

/********************* button 3 *********************/

if((event->button==3)&&(GLOBALS->signalpixmap))
	{
	GLOBALS->cachedwhich_signalwindow_c_1=-1;
	gdk_pointer_ungrab(event->time);
	DEBUG(printf("Button 3 released\n"));

	if(GLOBALS->dnd_state==1)
		{
		if(GLOBALS->cachedtrace)
			{
			GLOBALS->cachedtrace->flags|=TR_HIGHLIGHT;
			}

		which=(int)(event->y);
		which=(which/GLOBALS->fontheight)-1;
	
		if( ((which<0) && (GLOBALS->topmost_trace==GLOBALS->traces.first) && PrependBuffer()) || (PasteBuffer()) ) /* short circuit on special which<0 case */
	       		{
			/* status_text("Drop completed.\n"); */

			GLOBALS->signalwindow_width_dirty=1;
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
        		wavearea_configure_event(GLOBALS->wavearea, NULL);
        		}
		GLOBALS->dnd_state=0;
		}
	}

/********************* button 3 *********************/

return(TRUE);
}

static gint button_press_event(GtkWidget *widget, GdkEventButton *event)
{
int num_traces_displayable;
int which;
int trwhich, trtarget;
GtkAdjustment *wadj;
Trptr t;

if((GLOBALS->traces.visible)&&(GLOBALS->signalpixmap))
	{
	gdk_pointer_grab(widget->window, FALSE,
		GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON3_MOTION_MASK | 
		GDK_BUTTON_RELEASE_MASK, NULL, NULL, event->time);

	num_traces_displayable=widget->allocation.height/(GLOBALS->fontheight);
	num_traces_displayable--;   /* for the time trace that is always there */

	which=(int)(event->y);
	which=(which/GLOBALS->fontheight)-1;

	if((which>=GLOBALS->traces.visible)||(which>=num_traces_displayable)||(which<0))
		{
		if(GLOBALS->dnd_state==0)GLOBALS->cachedwhich_signalwindow_c_1=-1;
		goto check_button_3;	/* off in no man's land, but check 3rd anyways.. */
		}

	GLOBALS->cachedwhich_signalwindow_c_1=which;	/* cache for later fill in */
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

	GLOBALS->cachedtrace=t;

	/* frankel add */
	if((event->state&GDK_CONTROL_MASK)&&(GLOBALS->dnd_state==0)&&(event->button==1))
	if(t)
		{
		if(CollapseTrace(t))
			{
			GLOBALS->signalwindow_width_dirty=1;
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
        		wavearea_configure_event(GLOBALS->wavearea, NULL);
			return(TRUE);
			}
		}

	if((GLOBALS->dnd_state==0)&&(event->button==1))
	if(t)
		{
		int yval;
		GtkAdjustment *hadj;
		gint xsrc;

		if((GLOBALS->shift_click_trace)&&(event->state&GDK_SHIFT_MASK))
			{
			Trptr t2;
			unsigned int f;

			t2=GLOBALS->shift_click_trace;
			while(t2)
				{
				if(t2==t)
					{
					t2=GLOBALS->shift_click_trace;
					f=t2->flags&TR_HIGHLIGHT;
					while(t2)
						{
						t2->flags = (t2->flags & (~TR_HIGHLIGHT)) | f;
						if(t2==t) break;
						t2=GivePrevTrace(t2);
						}					
					goto resync_signalarea;
					}
				t2=GivePrevTrace(t2);
				}

			t2=GLOBALS->shift_click_trace;
			while(t2)
				{
				if(t2==t)
					{
					t2=GLOBALS->shift_click_trace;
					f=t2->flags&TR_HIGHLIGHT;
					while(t2)
						{
						t2->flags = (t2->flags & (~TR_HIGHLIGHT)) | f;
						if(t2==t) break;
						t2=GiveNextTrace(t2);
						}					
					goto resync_signalarea;
					}
				t2=GiveNextTrace(t2);
				}

			goto normal_button1_press;	/* couldn't find original so make this original... */
			
			resync_signalarea:
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
			DEBUG(printf("Shift-Click in signalarea!\n"));
			return(TRUE);
			}
			else
			{
			normal_button1_press:
			hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
			xsrc=(gint)hadj->value;

			GLOBALS->shift_click_trace=t;
			t->flags ^= TR_HIGHLIGHT;
			GLOBALS->cachedhiflag_signalwindow_c_1 = t->flags & TR_HIGHLIGHT;

			DEBUG(printf("Button pressed in signalarea at x: %d, y: %d row: %d\n",
				(int)event->x, (int)event->y, which));

			yval=RenderSig(t, which, 1);
	        	gdk_draw_pixmap(GLOBALS->signalarea->window, GLOBALS->signalarea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->signalarea)],GLOBALS->signalpixmap,xsrc, yval,0, yval,GLOBALS->signalarea->allocation.width, GLOBALS->fontheight-1);
			}
		}

check_button_3:
	if(event->button==3)
		{
		if(GLOBALS->dnd_state==0)
			{
			if(CutBuffer())
	        		{
				/* char buf[32]; */
				/* sprintf(buf,"Dragging %d trace%s.\n",traces.buffercount,traces.buffercount!=1?"s":"");
				status_text(buf); */
	        		MaxSignalLength();
	        		signalarea_configure_event(GLOBALS->signalarea, NULL);
	        		wavearea_configure_event(GLOBALS->wavearea, NULL);
				GLOBALS->dnd_state=1;
				}
			}
		}
	}

return(TRUE);
}


static gint popup_event (GtkWidget *widget)
{
do_popup_menu(widget, NULL);
return(TRUE);
}

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
struct Global *g_old = GLOBALS;

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

#ifdef WAVE_USE_GTK2
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

	gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signalarea), "scroll_event",GTK_SIGNAL_FUNC(scroll_event), NULL);

	do_focusing = 1;
	}
	else
#endif
	{
	gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signalarea), "button_press_event",GTK_SIGNAL_FUNC(button_press_event), NULL);
	gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signalarea), "button_release_event", GTK_SIGNAL_FUNC(button_release_event), NULL);
	gtkwave_signal_connect(GTK_OBJECT(GLOBALS->signalarea), "motion_notify_event",GTK_SIGNAL_FUNC(motion_notify_event), NULL);
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
		GLOBALS->keypress_handler_id = install_keypress_handler();
		}
		else
		{
		GLOBALS->keypress_handler_id = 0;
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
	"key_press_event",GTK_SIGNAL_FUNC(keypress_local), GLOBALS->signalarea_event_box);

return(rc);
}


void remove_keypress_handler(gint id)
{
gtk_signal_disconnect(GTK_OBJECT(GLOBALS->mainwindow), id);
}


/*
 * $Id$
 * $Log$
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

