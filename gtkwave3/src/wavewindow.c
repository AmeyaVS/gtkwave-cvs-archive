/* 
 * Copyright (c) Tony Bybell 1999-2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include "gtk12compat.h"
#include "currenttime.h"
#include "pixmaps.h"
#include "symbol.h"
#include "bsearch.h"
#include "color.h"
#include "rc.h"
#include "strace.h"
#include "debug.h"
#include "main.h"

#if !defined _ISOC99_SOURCE
#define _ISOC99_SOURCE 1
#endif
#include <math.h>

static void rendertimebar(void);
static void draw_hptr_trace(Trptr t, hptr h, int which, int dodraw, int kill_grid);
static void draw_hptr_trace_vector(Trptr t, hptr h, int which);
static void draw_vptr_trace(Trptr t, vptr v, int which);
static void rendertraces(void);
static void rendertimes(void);

static const GdkModifierType   bmask[4]= {0, GDK_BUTTON1_MASK, 0, GDK_BUTTON3_MASK };                   /* button 1, 3 press/rel encodings */
static const GdkModifierType m_bmask[4]= {0, GDK_BUTTON1_MOTION_MASK, 0, GDK_BUTTON3_MOTION_MASK };     /* button 1, 3 motion encodings */

/******************************************************************/

static void update_dual()
{
if(GLOBALS->dual_ctx && !GLOBALS->dual_race_lock)
        {
        GLOBALS->dual_ctx[GLOBALS->dual_id].zoom = GLOBALS->tims.zoom;
        GLOBALS->dual_ctx[GLOBALS->dual_id].marker = GLOBALS->tims.marker;
        GLOBALS->dual_ctx[GLOBALS->dual_id].baseline = GLOBALS->tims.baseline;
        GLOBALS->dual_ctx[GLOBALS->dual_id].left_margin_time = GLOBALS->tims.start;
        GLOBALS->dual_ctx[GLOBALS->dual_id].use_new_times = 1;
        }
}

/******************************************************************/

/*
 * gtk_draw_line() acceleration for win32 by doing draw combining
 * now always enabled.  if there need to be an exact correlation
 * between this and the printed data, this needs to be disabled.
 */
#if 1

#define WAVE_SEG_BUF_CNT 1024

static int seg_trans_cnt = 0, seg_low_cnt = 0, seg_high_cnt = 0, seg_mid_cnt = 0, seg_x_cnt = 0, seg_vtrans_cnt 
= 0, seg_0_cnt = 0, seg_1_cnt = 0, seg_vbox_cnt = 0;

static GdkSegment seg_trans[WAVE_SEG_BUF_CNT], seg_low[WAVE_SEG_BUF_CNT], seg_high[WAVE_SEG_BUF_CNT], 
seg_mid[WAVE_SEG_BUF_CNT], seg_x[WAVE_SEG_BUF_CNT], seg_vtrans[WAVE_SEG_BUF_CNT], seg_0[WAVE_SEG_BUF_CNT],
seg_1[WAVE_SEG_BUF_CNT], seg_vbox[WAVE_SEG_BUF_CNT];


static void wave_gdk_draw_line(GdkDrawable *drawable, GdkGC *gc, gint _x1, gint _y1, gint _x2, gint _y2)
{
GdkSegment *seg;
int *seg_cnt;

if(gc==GLOBALS->gc_trans_wavewindow_c_1) 	{ seg = seg_trans; seg_cnt = &seg_trans_cnt; }
else if(gc==GLOBALS->gc_low_wavewindow_c_1) 	{ seg = seg_low; seg_cnt = &seg_low_cnt; }
else if(gc==GLOBALS->gc_high_wavewindow_c_1) 	{ seg = seg_high; seg_cnt = &seg_high_cnt; }
else if(gc==GLOBALS->gc_mid_wavewindow_c_1) 	{ seg = seg_mid; seg_cnt = &seg_mid_cnt; }
else if(gc == GLOBALS->gc_x_wavewindow_c_1)	{ seg = seg_x; seg_cnt = &seg_x_cnt; }
else if(gc == GLOBALS->gc_vtrans_wavewindow_c_1){ seg = seg_vtrans; seg_cnt = &seg_vtrans_cnt; }
else if(gc == GLOBALS->gc_0_wavewindow_c_1) 	{ seg = seg_0;  seg_cnt = &seg_0_cnt; }
else if(gc == GLOBALS->gc_1_wavewindow_c_1)	{ seg = seg_1; seg_cnt = &seg_1_cnt; }
else if(gc == GLOBALS->gc_vbox_wavewindow_c_1)	{ seg = seg_vbox; seg_cnt = &seg_vbox_cnt; }
else 						{ gdk_draw_line(drawable, gc, _x1, _y1, _x2, _y2); return; }

seg[*seg_cnt].x1 = _x1;
seg[*seg_cnt].y1 = _y1;
seg[*seg_cnt].x2 = _x2;
seg[*seg_cnt].y2 = _y2;
(*seg_cnt)++;

if(*seg_cnt == WAVE_SEG_BUF_CNT)
	{
	gdk_draw_segments(drawable, gc, seg, *seg_cnt);
	*seg_cnt = 0;
	}
}

static void wave_gdk_draw_line_flush(GdkDrawable *drawable)
{
if(seg_vtrans_cnt)
	{
	gdk_draw_segments (drawable, GLOBALS->gc_vtrans_wavewindow_c_1, seg_vtrans, seg_vtrans_cnt);
	seg_vtrans_cnt = 0;
	}

if(seg_mid_cnt)
	{
	gdk_draw_segments(drawable, GLOBALS->gc_mid_wavewindow_c_1, seg_mid, seg_mid_cnt);
	seg_mid_cnt = 0;
	}

if(seg_high_cnt)
	{
	gdk_draw_segments(drawable, GLOBALS->gc_high_wavewindow_c_1, seg_high, seg_high_cnt);
	seg_high_cnt = 0;
	}

if(seg_low_cnt)
	{
	gdk_draw_segments(drawable, GLOBALS->gc_low_wavewindow_c_1, seg_low, seg_low_cnt);
	seg_low_cnt = 0;
	}

if(seg_trans_cnt)
	{
	gdk_draw_segments(drawable, GLOBALS->gc_trans_wavewindow_c_1, seg_trans, seg_trans_cnt);
	seg_trans_cnt = 0;
	}

if(seg_0_cnt)
	{
	gdk_draw_segments (drawable, GLOBALS->gc_0_wavewindow_c_1, seg_0, seg_0_cnt);
	seg_0_cnt = 0;
	}

if(seg_1_cnt)
	{
	gdk_draw_segments (drawable, GLOBALS->gc_1_wavewindow_c_1, seg_1, seg_1_cnt);
	seg_1_cnt = 0;
	}

/* place x down here to propagate them over low/high transitions */

if(seg_x_cnt)
	{
	gdk_draw_segments (drawable, GLOBALS->gc_x_wavewindow_c_1, seg_x, seg_x_cnt);
	seg_x_cnt = 0;
	}

if(seg_vbox_cnt)
	{
	gdk_draw_segments (drawable, GLOBALS->gc_vbox_wavewindow_c_1, seg_vbox, seg_vbox_cnt);
	seg_vbox_cnt = 0;
	}
}

#else

/* completely unnecessary for linux ... unless there are extremely dense traces */

#define wave_gdk_draw_line(a,b,c,d,e,f) gdk_draw_line(a,b,c,d,e,f)
#define wave_gdk_draw_line_flush(x)

#endif

/******************************************************************/


/*
 * aldec-like "snap" feature...
 */
TimeType cook_markertime(TimeType marker, gint x, gint y)
{
int i, num_traces_displayable;                
Trptr t = NULL;
TimeType lft, rgh;
char lftinv, rghinv;
gdouble xlft, xrgh;
gdouble xlftd, xrghd;
TimeType closest_named = MAX_HISTENT_TIME;
int closest_which = -1;
gint xold = x, yold = y;
TraceEnt t_trans;

if(!GLOBALS->cursor_snap) return(marker);

/* potential snapping to a named marker time */
for(i=0;i<26;i++)
	{
	if(GLOBALS->named_markers[i] != -1)
		{
		TimeType dlt;

		if((GLOBALS->named_markers[i]>=GLOBALS->tims.start)&&(GLOBALS->named_markers[i]<=GLOBALS->tims.end)&&(GLOBALS->named_markers[i]<=GLOBALS->tims.last))
			{
			if(marker < GLOBALS->named_markers[i])
				{
				dlt = GLOBALS->named_markers[i] - marker;			
				}
				else
				{		
				dlt = marker - GLOBALS->named_markers[i];			
				}
			
			if(dlt < closest_named)
				{
				closest_named = dlt;
				closest_which = i;
				}
			}
		}
	}

num_traces_displayable=GLOBALS->wavearea->allocation.height/(GLOBALS->fontheight);
num_traces_displayable--;   /* for the time trace that is always there */

y-=GLOBALS->fontheight;
if(y<0) y=0;
y/=GLOBALS->fontheight;		    /* y now indicates the trace in question */
if(y>num_traces_displayable) y=num_traces_displayable;

t=GLOBALS->topmost_trace;
for(i=0;i<y;i++)
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
			goto process_trace;
			}
		}
        }

if((t->flags&TR_BLANK))
        {
        t = NULL;
        goto bot;
        }

if(t->flags & TR_ANALOG_BLANK_STRETCH)	/* seek to real analog trace if present... */
	{
	while((t) && (t = GivePrevTrace(t)))
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
if(!t) goto bot;

process_trace:
if(t->vector)
	{
	vptr v = bsearch_vector(t->n.vec, marker - t->shift);
	vptr v2 = v ? v->next : NULL;

	if((!v)||(!v2)) goto bot;	/* should never happen */

	lft = v->time;
	rgh = v2->time;
	}
	else
	{
	hptr h = bsearch_node(t->n.nd, marker - t->shift);
	hptr h2 = h ? h->next : NULL;

	if((!h)||(!h2)) goto bot;	/* should never happen */

	lft = h->time;
	rgh = h2->time;
	}


lftinv = (lft < (GLOBALS->tims.start - t->shift))||(lft >= (GLOBALS->tims.end - t->shift))||(lft >= (GLOBALS->tims.last - t->shift));
rghinv = (rgh < (GLOBALS->tims.start - t->shift))||(rgh >= (GLOBALS->tims.end - t->shift))||(rgh >= (GLOBALS->tims.last - t->shift));

xlft = (lft + t->shift - GLOBALS->tims.start) * GLOBALS->pxns;
xrgh = (rgh + t->shift - GLOBALS->tims.start) * GLOBALS->pxns;

xlftd = xlft - x;
if(xlftd<(gdouble)0.0) xlftd = ((gdouble)0.0) - xlftd;

xrghd = xrgh - x;
if(xrghd<(gdouble)0.0) xrghd = ((gdouble)0.0) - xrghd;

if(xlftd<=xrghd)
	{
	if((!lftinv)&&(xlftd<=GLOBALS->cursor_snap)) 
		{
		if(closest_which >= 0)
		        {
        		if((closest_named * GLOBALS->pxns) < xlftd)
                		{
                		marker = GLOBALS->named_markers[closest_which];
				goto xit;
                		}
			}

		marker = lft + t->shift;
		goto xit;
		}
	}
	else
	{
	if((!rghinv)&&(xrghd<=GLOBALS->cursor_snap)) 
		{
		if(closest_which >= 0)
		        {
        		if((closest_named * GLOBALS->pxns) < xrghd)
                		{
                		marker = GLOBALS->named_markers[closest_which];
				goto xit;
                		}
			}

		marker = rgh + t->shift;
		goto xit;
		}
	}

bot:
if(closest_which >= 0)
	{
	if((closest_named * GLOBALS->pxns) <= GLOBALS->cursor_snap)
		{
		marker = GLOBALS->named_markers[closest_which];
		}
	}

xit:
GLOBALS->mouseover_counter = -1;
move_mouseover(t, xold, yold, marker);
return(marker);
}

static void render_individual_named_marker(int i, GdkGC *gc, int blackout)
{
gdouble pixstep;
gint xl, y;
TimeType t;

if((t=GLOBALS->named_markers[i])!=-1)
	{
	if((t>=GLOBALS->tims.start)&&(t<=GLOBALS->tims.last)
		&&(t<=GLOBALS->tims.end))
		{
		/* this needs to be here rather than outside the loop as gcc does some
		   optimizations that cause it to calculate slightly different from the marker if it's not here */
		pixstep=((gdouble)GLOBALS->nsperframe)/((gdouble)GLOBALS->pixelsperframe);

		xl=((gdouble)(t-GLOBALS->tims.start))/pixstep;     /* snap to integer */
		if((xl>=0)&&(xl<GLOBALS->wavewidth))
			{
			char nbuff[2];
			nbuff[0]='A'+i; nbuff[1]=0x00;

			for(y=GLOBALS->fontheight-1;y<=GLOBALS->waveheight-1;y+=8)
				{
				gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1,
					gc,
        	        		xl, y, xl, y+5);
				}

			if((!GLOBALS->marker_names[i])||(!GLOBALS->marker_names[i][0]))
				{
				font_engine_draw_string(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->wavefont_smaller,
					gc,
					xl-(font_engine_string_measure(GLOBALS->wavefont_smaller, nbuff)>>1), 
					GLOBALS->fontheight-2, nbuff);
				}
				else
				{
				int width = font_engine_string_measure(GLOBALS->wavefont_smaller, GLOBALS->marker_names[i]);
				if(blackout) /* blackout background so text is legible if overlaid with other marker labels */
					{
					gdk_draw_rectangle(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_timeb_wavewindow_c_1, TRUE,
						xl-(width>>1), GLOBALS->fontheight-2-GLOBALS->wavefont_smaller->ascent, 
						width, GLOBALS->wavefont_smaller->ascent + GLOBALS->wavefont_smaller->descent);
					}

				font_engine_draw_string(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->wavefont_smaller,
					gc,
					xl-(width>>1), 
					GLOBALS->fontheight-2, GLOBALS->marker_names[i]);
				}
			}
		}
	}
}

static void draw_named_markers(void)
{
int i;

for(i=0;i<26;i++)
	{
	if(i != GLOBALS->named_marker_lock_idx)
		{
		render_individual_named_marker(i, GLOBALS->gc_mark_wavewindow_c_1, 0);
		}
	}

if(GLOBALS->named_marker_lock_idx >= 0)
	{
	render_individual_named_marker(GLOBALS->named_marker_lock_idx, GLOBALS->gc_umark_wavewindow_c_1, 1);
	}
}


static void sync_marker(void)
{
if((GLOBALS->tims.prevmarker==-1)&&(GLOBALS->tims.marker!=-1))
	{
	GLOBALS->signalwindow_width_dirty=1;
	}
	else
if((GLOBALS->tims.marker==-1)&&(GLOBALS->tims.prevmarker!=-1))
	{
	GLOBALS->signalwindow_width_dirty=1;
	}
GLOBALS->tims.prevmarker=GLOBALS->tims.marker;
}


static void draw_marker(void)
{
gdouble pixstep;
gint xl;

if(!GLOBALS->wavearea->window) return;

GLOBALS->m1x_wavewindow_c_1=GLOBALS->m2x_wavewindow_c_1=-1;

if(GLOBALS->tims.baseline>=0)
	{
	if((GLOBALS->tims.baseline>=GLOBALS->tims.start)&&(GLOBALS->tims.baseline<=GLOBALS->tims.last)
		&&(GLOBALS->tims.baseline<=GLOBALS->tims.end))
		{
		pixstep=((gdouble)GLOBALS->nsperframe)/((gdouble)GLOBALS->pixelsperframe);
		xl=((gdouble)(GLOBALS->tims.baseline-GLOBALS->tims.start))/pixstep;     /* snap to integer */
		if((xl>=0)&&(xl<GLOBALS->wavewidth))
			{
			gdk_draw_line(GLOBALS->wavearea->window,GLOBALS->gc_baseline_wavewindow_c_1,xl, GLOBALS->fontheight-1, xl, GLOBALS->waveheight-1);
			}
		}
	}

if(GLOBALS->tims.marker>=0)
	{
	if((GLOBALS->tims.marker>=GLOBALS->tims.start)&&(GLOBALS->tims.marker<=GLOBALS->tims.last)
		&&(GLOBALS->tims.marker<=GLOBALS->tims.end))
		{
		pixstep=((gdouble)GLOBALS->nsperframe)/((gdouble)GLOBALS->pixelsperframe);
		xl=((gdouble)(GLOBALS->tims.marker-GLOBALS->tims.start))/pixstep;     /* snap to integer */
		if((xl>=0)&&(xl<GLOBALS->wavewidth))
			{
			gdk_draw_line(GLOBALS->wavearea->window,GLOBALS->gc_umark_wavewindow_c_1,xl, GLOBALS->fontheight-1, xl, GLOBALS->waveheight-1);
			GLOBALS->m1x_wavewindow_c_1=xl;
			}
		}
	}


if((GLOBALS->enable_ghost_marker)&&(GLOBALS->in_button_press_wavewindow_c_1)&&(GLOBALS->tims.lmbcache>=0))
	{
	if((GLOBALS->tims.lmbcache>=GLOBALS->tims.start)&&(GLOBALS->tims.lmbcache<=GLOBALS->tims.last)
		&&(GLOBALS->tims.lmbcache<=GLOBALS->tims.end))
		{
		pixstep=((gdouble)GLOBALS->nsperframe)/((gdouble)GLOBALS->pixelsperframe);
		xl=((gdouble)(GLOBALS->tims.lmbcache-GLOBALS->tims.start))/pixstep;     /* snap to integer */
		if((xl>=0)&&(xl<GLOBALS->wavewidth))
			{
			gdk_draw_line(GLOBALS->wavearea->window,GLOBALS->gc_umark_wavewindow_c_1,xl, GLOBALS->fontheight-1, xl, GLOBALS->waveheight-1);
			GLOBALS->m2x_wavewindow_c_1=xl;
			}
		}
	}

if(GLOBALS->m1x_wavewindow_c_1>GLOBALS->m2x_wavewindow_c_1)		/* ensure m1x <= m2x for partitioned refresh */
	{
	gint t;

	t=GLOBALS->m1x_wavewindow_c_1;
	GLOBALS->m1x_wavewindow_c_1=GLOBALS->m2x_wavewindow_c_1;
	GLOBALS->m2x_wavewindow_c_1=t;
	}

if(GLOBALS->m1x_wavewindow_c_1==-1) GLOBALS->m1x_wavewindow_c_1=GLOBALS->m2x_wavewindow_c_1;	/* make both markers same if no ghost marker or v.v. */

update_dual();
}


static void draw_marker_partitions(void)
{
draw_marker();

if(GLOBALS->m1x_wavewindow_c_1==GLOBALS->m2x_wavewindow_c_1)
	{
	gdk_draw_pixmap(GLOBALS->wavearea->window, GLOBALS->wavearea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->wavearea)],GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->m1x_wavewindow_c_1, 0, GLOBALS->m1x_wavewindow_c_1, 0, 1, GLOBALS->fontheight-2);

	if(GLOBALS->m1x_wavewindow_c_1<0)
		{
		gdk_draw_pixmap(GLOBALS->wavearea->window, GLOBALS->wavearea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->wavearea)],GLOBALS->wavepixmap_wavewindow_c_1, 0, 0, 0, 0, GLOBALS->wavewidth, GLOBALS->waveheight);
		}
		else
		{
		if(GLOBALS->m1x_wavewindow_c_1==0)
			{
			gdk_draw_pixmap(GLOBALS->wavearea->window, GLOBALS->wavearea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->wavearea)],GLOBALS->wavepixmap_wavewindow_c_1, 1, 0, 1, 0, GLOBALS->wavewidth-1, GLOBALS->waveheight);
			}
		else
		if(GLOBALS->m1x_wavewindow_c_1==GLOBALS->wavewidth-1)
			{

			gdk_draw_pixmap(GLOBALS->wavearea->window, GLOBALS->wavearea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->wavearea)],GLOBALS->wavepixmap_wavewindow_c_1, 0, 0, 0, 0, GLOBALS->wavewidth-1, GLOBALS->waveheight);
			}
		else
			{
			gdk_draw_pixmap(GLOBALS->wavearea->window, GLOBALS->wavearea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->wavearea)],GLOBALS->wavepixmap_wavewindow_c_1, 0, 0, 0, 0, GLOBALS->m1x_wavewindow_c_1, GLOBALS->waveheight);
			gdk_draw_pixmap(GLOBALS->wavearea->window, GLOBALS->wavearea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->wavearea)],GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->m1x_wavewindow_c_1+1, 0, GLOBALS->m1x_wavewindow_c_1+1, 0, GLOBALS->wavewidth-GLOBALS->m1x_wavewindow_c_1-1, GLOBALS->waveheight);
			}
		}
	}
	else
	{
	gdk_draw_pixmap(GLOBALS->wavearea->window, GLOBALS->wavearea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->wavearea)],GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->m1x_wavewindow_c_1, 0, GLOBALS->m1x_wavewindow_c_1, 0, 1, GLOBALS->fontheight-2);
	gdk_draw_pixmap(GLOBALS->wavearea->window, GLOBALS->wavearea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->wavearea)],GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->m2x_wavewindow_c_1, 0, GLOBALS->m2x_wavewindow_c_1, 0, 1, GLOBALS->fontheight-2);

	if(GLOBALS->m1x_wavewindow_c_1>0)
		{
		gdk_draw_pixmap(GLOBALS->wavearea->window, GLOBALS->wavearea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->wavearea)],GLOBALS->wavepixmap_wavewindow_c_1, 0, 0, 0, 0, GLOBALS->m1x_wavewindow_c_1, GLOBALS->waveheight);
		}

	if(GLOBALS->m2x_wavewindow_c_1-GLOBALS->m1x_wavewindow_c_1>1)
		{
		gdk_draw_pixmap(GLOBALS->wavearea->window, GLOBALS->wavearea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->wavearea)],GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->m1x_wavewindow_c_1+1, 0, GLOBALS->m1x_wavewindow_c_1+1, 0, GLOBALS->m2x_wavewindow_c_1-GLOBALS->m1x_wavewindow_c_1-1, GLOBALS->waveheight);
		}

	if(GLOBALS->m2x_wavewindow_c_1!=GLOBALS->wavewidth-1)
		{
		gdk_draw_pixmap(GLOBALS->wavearea->window, GLOBALS->wavearea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->wavearea)],GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->m2x_wavewindow_c_1+1, 0, GLOBALS->m2x_wavewindow_c_1+1, 0, GLOBALS->wavewidth-GLOBALS->m2x_wavewindow_c_1-1, GLOBALS->waveheight);
		}
	}

/* keep baseline from getting obliterated */
#ifndef WAVE_DOUBLE_LINE_WIDTH_MODE
if(GLOBALS->tims.baseline>=0)
#endif
	{
	draw_marker();
	}
}

static void renderblackout(void)
{
gfloat pageinc;
TimeType lhs, rhs, lclip, rclip;
struct blackout_region_t *bt = GLOBALS->blackout_regions;

if(bt)
	{
	pageinc=(gfloat)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
	lhs = GLOBALS->tims.start;
	rhs = pageinc + lhs;

	while(bt)
		{
		if((bt->bend < lhs) || (bt->bstart > rhs))
			{
			/* nothing, out of bounds */
			}
			else
			{
			lclip = bt->bstart; rclip = bt->bend;

			if(lclip < lhs) lclip = lhs;
				else if (lclip > rhs) lclip = rhs;

			if(rclip < lhs) rclip = lhs;

			lclip -= lhs;
			rclip -= lhs;
			if(rclip>((GLOBALS->wavewidth+1)*GLOBALS->nspx)) rclip = (GLOBALS->wavewidth+1)*(GLOBALS->nspx);
			
			gdk_draw_rectangle(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_xfill_wavewindow_c_1, TRUE, (((gdouble)lclip)*GLOBALS->pxns), GLOBALS->fontheight,(((gdouble)(rclip-lclip))*GLOBALS->pxns), GLOBALS->waveheight-GLOBALS->fontheight);
			}

		bt=bt->next;
		}
	}
}

static void     
service_hslider(GtkWidget *text, gpointer data)
{
DEBUG(printf("Wave HSlider Moved\n"));

if((GLOBALS->wavepixmap_wavewindow_c_1)&&(GLOBALS->wavewidth>1))
	{
	GtkAdjustment *hadj;

	hadj=GTK_ADJUSTMENT(GLOBALS->wave_hslider);

	if(!GLOBALS->tims.timecache)
		{
		GLOBALS->tims.start=time_trunc(hadj->value);
		}
		else
		{
		GLOBALS->tims.start=time_trunc(GLOBALS->tims.timecache);
		GLOBALS->tims.timecache=0;	/* reset */
		}

	if(GLOBALS->tims.start<GLOBALS->tims.first) GLOBALS->tims.start=GLOBALS->tims.first;
		else if(GLOBALS->tims.start>GLOBALS->tims.last) GLOBALS->tims.start=GLOBALS->tims.last;

	GLOBALS->tims.laststart=GLOBALS->tims.start;

	gdk_draw_rectangle(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_back_wavewindow_c_1, TRUE, 0, 0,GLOBALS->wavewidth, GLOBALS->waveheight);
	rendertimebar();
	}
}

static void     
service_vslider(GtkWidget *text, gpointer data)
{
GtkAdjustment *sadj, *hadj;
int trtarget;
int xsrc;

if(GLOBALS->signalpixmap)
	{
	hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
	sadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
	xsrc=(gint)hadj->value;

	trtarget=(int)(sadj->value);
	DEBUG(printf("Wave VSlider Moved to %d\n",trtarget));

		gdk_draw_rectangle(GLOBALS->signalpixmap, 
			GLOBALS->gc_ltgray, TRUE, 0, 0,
	            	GLOBALS->signal_fill_width, GLOBALS->signalarea->allocation.height);
	
		sync_marker();
		RenderSigs(trtarget,(GLOBALS->old_wvalue==sadj->value)?0:1);

	GLOBALS->old_wvalue=sadj->value;

		draw_named_markers();

		if(GLOBALS->signalarea_has_focus)
			{
			gdk_draw_pixmap(GLOBALS->signalarea->window, GLOBALS->signalarea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->signalarea)],
		                GLOBALS->signalpixmap,
				xsrc+1, 0+1,
				0+1, 0+1,
		                GLOBALS->signalarea->allocation.width-2, GLOBALS->signalarea->allocation.height-2);
			draw_signalarea_focus();
			}
			else
			{
			gdk_draw_pixmap(GLOBALS->signalarea->window, GLOBALS->signalarea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->signalarea)],
		                GLOBALS->signalpixmap,
				xsrc, 0,
				0, 0,
		                GLOBALS->signalarea->allocation.width, GLOBALS->signalarea->allocation.height);
			}

		draw_marker();
	}
}

void button_press_release_common(void)
{
MaxSignalLength();

gdk_draw_rectangle(GLOBALS->signalpixmap, 
	GLOBALS->gc_ltgray, TRUE, 0, 0,
        GLOBALS->signal_fill_width, GLOBALS->signalarea->allocation.height);
sync_marker();
RenderSigs((int)(GTK_ADJUSTMENT(GLOBALS->wave_vslider)->value),0);

if(GLOBALS->signalarea_has_focus)
	{
	gdk_draw_pixmap(GLOBALS->signalarea->window, GLOBALS->signalarea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->signalarea)],
	      	GLOBALS->signalpixmap,
		(gint)(GTK_ADJUSTMENT(GLOBALS->signal_hslider)->value)+1, 0+1,
		0+1, 0+1,
	        GLOBALS->signalarea->allocation.width-2, GLOBALS->signalarea->allocation.height-2);

	draw_signalarea_focus();
	}
	else
	{
	gdk_draw_pixmap(GLOBALS->signalarea->window, GLOBALS->signalarea->style->fg_gc[GTK_WIDGET_STATE(GLOBALS->signalarea)],
	      	GLOBALS->signalpixmap,
		(gint)(GTK_ADJUSTMENT(GLOBALS->signal_hslider)->value), 0,
		0, 0,
	        GLOBALS->signalarea->allocation.width, GLOBALS->signalarea->allocation.height);
	}
}

static void button_motion_common(gint xin, gint yin, int pressrel, int is_button_2) 
{
gdouble x,offset,pixstep;
TimeType newcurr;

if(xin<0) xin=0;
if(xin>(GLOBALS->wavewidth-1)) xin=(GLOBALS->wavewidth-1);

x=xin;	/* for pix time calc */

pixstep=((gdouble)GLOBALS->nsperframe)/((gdouble)GLOBALS->pixelsperframe);
newcurr=(TimeType)(offset=((gdouble)GLOBALS->tims.start)+(x*pixstep));

if(offset-newcurr>0.5)	/* round to nearest integer ns */
	{
	newcurr++;
	}

if(newcurr>GLOBALS->tims.last) 		/* sanity checking */
	{
	newcurr=GLOBALS->tims.last;
	}
if(newcurr>GLOBALS->tims.end)
	{
	newcurr=GLOBALS->tims.end;
	}
if(newcurr<GLOBALS->tims.start)
	{
	newcurr=GLOBALS->tims.start;
	}

if(!is_button_2)
	{
	update_markertime(GLOBALS->tims.marker=cook_markertime(time_trunc(newcurr), xin, yin));
	if(GLOBALS->tims.lmbcache<0) GLOBALS->tims.lmbcache=time_trunc(newcurr);

	draw_marker_partitions();

	if((pressrel)||(GLOBALS->constant_marker_update)) 
		{
		button_press_release_common();
		}
	}
	else
	{
	GLOBALS->tims.baseline = ((GLOBALS->tims.baseline<0)||(is_button_2<0)) ? cook_markertime(time_trunc(newcurr), xin, yin) : -1;
	update_basetime(GLOBALS->tims.baseline);
	update_markertime(GLOBALS->tims.marker);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}

static gint motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
gdouble x, y, pixstep, offset;
GdkModifierType state;
TimeType newcurr;
int scrolled;

#ifdef WAVE_USE_GTK2
gint xi, yi;
#endif

int dummy_x, dummy_y;
get_window_xypos(&dummy_x, &dummy_y);

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

do
	{
	scrolled=0;
	if(state&bmask[GLOBALS->in_button_press_wavewindow_c_1]) /* needed for retargeting in AIX/X11 */
		{
		if(x<0)
			{ 
			if(GLOBALS->wave_scrolling)
			if(GLOBALS->tims.start>GLOBALS->tims.first)
				{
				if(GLOBALS->nsperframe<10) 
					{
					GLOBALS->tims.start-=GLOBALS->nsperframe;
					}
					else
					{
					GLOBALS->tims.start-=(GLOBALS->nsperframe/10);
					}
				if(GLOBALS->tims.start<GLOBALS->tims.first) GLOBALS->tims.start=GLOBALS->tims.first;
				GTK_ADJUSTMENT(GLOBALS->wave_hslider)->value=GLOBALS->tims.marker=time_trunc(GLOBALS->tims.timecache=GLOBALS->tims.start);
	
				gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed");
				gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed");
				scrolled=1;
				}	
			x=0; 
			}
		else
		if(x>GLOBALS->wavewidth) 
			{ 
			if(GLOBALS->wave_scrolling)
			if(GLOBALS->tims.start!=GLOBALS->tims.last)
				{
				gfloat pageinc;
		
				pageinc=(gfloat)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
	
				if(GLOBALS->nsperframe<10) 
					{
					GLOBALS->tims.start+=GLOBALS->nsperframe;
					}
					else
					{
					GLOBALS->tims.start+=(GLOBALS->nsperframe/10);
					}
	
				if(GLOBALS->tims.start>GLOBALS->tims.last-pageinc+1) GLOBALS->tims.start=time_trunc(GLOBALS->tims.last-pageinc+1);
				if(GLOBALS->tims.start<GLOBALS->tims.first) GLOBALS->tims.start=GLOBALS->tims.first;
	
				GLOBALS->tims.marker=time_trunc(GLOBALS->tims.start+pageinc);
				if(GLOBALS->tims.marker>GLOBALS->tims.last) GLOBALS->tims.marker=GLOBALS->tims.last;
		
				GTK_ADJUSTMENT(GLOBALS->wave_hslider)->value=GLOBALS->tims.timecache=GLOBALS->tims.start;
	
				gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed");
				gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed");
				scrolled=1;
				}
			x=GLOBALS->wavewidth-1; 
			}
		}	
	else if((state&GDK_BUTTON2_MASK)&&(GLOBALS->tims.baseline>=0))
		{
		button_motion_common(x,y,0,-1); /* neg one says don't clear tims.baseline */
		}

	pixstep=((gdouble)GLOBALS->nsperframe)/((gdouble)GLOBALS->pixelsperframe);
	newcurr=GLOBALS->tims.start+(offset=x*pixstep);
	if((offset-((int)offset))>0.5)  /* round to nearest integer ns */
	        {
	        newcurr++;
	        }
	
	if(newcurr>GLOBALS->tims.last) newcurr=GLOBALS->tims.last;
	
	if(newcurr!=GLOBALS->prevtim_wavewindow_c_1)
		{
		update_currenttime(time_trunc(newcurr));
		GLOBALS->prevtim_wavewindow_c_1=newcurr;
		}
	
	if(state&bmask[GLOBALS->in_button_press_wavewindow_c_1])
		{
		button_motion_common(x,y,0,0);
		}

	/* warp selected signals if CTRL is pressed */
        if((event->state & GDK_CONTROL_MASK)&&(state&GDK_BUTTON1_MASK))
		{
	  	int warp = 0;
          	Trptr t = GLOBALS->traces.first;
		TimeType gt, delta;

          	while ( t )
          		{
            		if ( t->flags & TR_HIGHLIGHT )
            			{
	      			warp++;

				if(!t->shift_drag_valid)
					{
					t->shift_drag = t->shift;
					t->shift_drag_valid = 1;
					}

              			gt = t->shift_drag + (GLOBALS->tims.marker - GLOBALS->tims.lmbcache);

		        	if(gt<0)
        		        	{
		                	delta=GLOBALS->tims.first-GLOBALS->tims.last;
		                	if(gt<delta) gt=delta;
		                	}
		       		else 
				if(gt>0)
		                	{
		                	delta=GLOBALS->tims.last-GLOBALS->tims.first;
		                	if(gt>delta) gt=delta;
		                	}
				t->shift = gt;
            			}

            		t = t->t_next;
          		}

	  	if( warp )
	  		{
/* commented out to reduce on visual noise...

            		signalwindow_width_dirty = 1;
            		MaxSignalLength(  );

...commented out to reduce on visual noise */

            		signalarea_configure_event( GLOBALS->signalarea, NULL );
            		wavearea_configure_event( GLOBALS->wavearea, NULL );
          		}
		}

	if(scrolled)	/* make sure counters up top update.. */
		{
		while(gtk_events_pending())
			{
			gtk_main_iteration();
			}
		}

	WAVE_GDK_GET_POINTER(event->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;

	} while((scrolled)&&(state&bmask[GLOBALS->in_button_press_wavewindow_c_1]));

return(TRUE);
}

#ifdef WAVE_USE_GTK2
static void alternate_y_scroll(int delta)
{
GtkAdjustment *wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
int value = (int)wadj->value;
int target = value + delta;
                
int num_traces_displayable=(GLOBALS->signalarea->allocation.height)/(GLOBALS->fontheight);
num_traces_displayable--;   /* for the time trace that is always there */
                
if(target > GLOBALS->traces.visible - num_traces_displayable) target = GLOBALS->traces.visible - num_traces_displayable;

if(target < 0) target = 0;
         
wadj->value = target;
         
gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed"); /* force bar update */
gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /* force text update */
}

static        gint
scroll_event( GtkWidget * widget, GdkEventScroll * event )
{
int num_traces_displayable=(GLOBALS->signalarea->allocation.height)/(GLOBALS->fontheight);
num_traces_displayable--;

  DEBUG(printf("Mouse Scroll Event\n"));
  switch ( event->direction )
  {
    case GDK_SCROLL_UP:
      if (GLOBALS->use_scrollwheel_as_y)
	{
	if(event->state & GDK_SHIFT_MASK)
		{
		alternate_y_scroll(-num_traces_displayable);
		}
		else
		{
		alternate_y_scroll(-1);
		}
	}
	else
	{
      	if ( event->state & GDK_CONTROL_MASK )
        	service_left_shift(NULL, 0);
      	else if ( event->state & GDK_MOD1_MASK )
		service_zoom_out(NULL, 0);
      	else
        	service_left_page(NULL, 0);
	}
      break;
    case GDK_SCROLL_DOWN:
      if (GLOBALS->use_scrollwheel_as_y)
	{
	if(event->state & GDK_SHIFT_MASK)
		{
		alternate_y_scroll(num_traces_displayable);
		}
		else
		{
		alternate_y_scroll(1);
		}
	}
	{
      	if ( event->state & GDK_CONTROL_MASK )
        	service_right_shift(NULL, 0);
      	else if ( event->state & GDK_MOD1_MASK )
		service_zoom_in(NULL, 0);
      	else
        	service_right_page(NULL, 0);
	}
      break;

    default:
      break;
  }
  return(TRUE);
}
#endif

static gint button_press_event(GtkWidget *widget, GdkEventButton *event)
{
if((event->button==1)||((event->button==3)&&(!GLOBALS->in_button_press_wavewindow_c_1)))
	{
	GLOBALS->in_button_press_wavewindow_c_1=event->button;
	
	DEBUG(printf("Button Press Event\n"));
	GLOBALS->prev_markertime = GLOBALS->tims.marker;
	button_motion_common(event->x,event->y,1,0);
	GLOBALS->tims.timecache=GLOBALS->tims.start;

	gdk_pointer_grab(widget->window, FALSE,
		m_bmask[GLOBALS->in_button_press_wavewindow_c_1] | /* key up on motion for button pressed ONLY */
		GDK_POINTER_MOTION_HINT_MASK |
	      	GDK_BUTTON_RELEASE_MASK, NULL, NULL, event->time);
	
	if ((event->state & GDK_CONTROL_MASK) && (event->button==1))
		{
		Trptr t = GLOBALS->traces.first;

		while(t)
			{
			if((t->flags & TR_HIGHLIGHT)&&(!t->shift_drag_valid))			
				{
				t->shift_drag = t->shift; /* cache old value */
				t->shift_drag_valid = 1;
				}
			t=t->t_next;
			}
		}
	}
else
if(event->button==2)
	{
	button_motion_common(event->x,event->y,1,1);
	}

return(TRUE);
}

static gint button_release_event(GtkWidget *widget, GdkEventButton *event)
{
if((event->button)&&(event->button==GLOBALS->in_button_press_wavewindow_c_1))
	{
	GLOBALS->in_button_press_wavewindow_c_1=0;

	DEBUG(printf("Button Release Event\n"));
	button_motion_common(event->x,event->y,1,0);

	/* warp selected signals if CTRL is pressed */
        if((event->button==1))
		{
	  	int warp = 0;
          	Trptr t = GLOBALS->traces.first;

		if(event->state & GDK_CONTROL_MASK)
			{	
			TimeType gt, delta;

	          	while ( t )
	          		{
	            		if ( t->flags & TR_HIGHLIGHT )
	            			{
		      			warp++;
	              			gt = (t->shift_drag_valid ? t-> shift_drag : t->shift) + (GLOBALS->tims.marker - GLOBALS->tims.lmbcache);
	
			        	if(gt<0)
	        		        	{
			                	delta=GLOBALS->tims.first-GLOBALS->tims.last;
			                	if(gt<delta) gt=delta;
			                	}
			       		else 
					if(gt>0)
			                	{
			                	delta=GLOBALS->tims.last-GLOBALS->tims.first;
			                	if(gt>delta) gt=delta;
			                	}
					t->shift = gt;
	
	              			t->flags &= ( ~TR_HIGHLIGHT );
	            			}
	
				t->shift_drag_valid = 0;
	            		t = t->t_next;
	          		}
			}
			else	/* back out warp and keep highlighting */
			{
			while(t)
				{
				if(t->shift_drag_valid)
					{
					t->shift = t->shift_drag;
					t->shift_drag_valid = 0;
					warp++;
					}
				t=t->t_next;
				}
			}

	  	if( warp )
	  		{
            		GLOBALS->signalwindow_width_dirty = 1;
            		MaxSignalLength(  );
            		signalarea_configure_event( GLOBALS->signalarea, NULL );
            		wavearea_configure_event( GLOBALS->wavearea, NULL );
          		}
		}

	GLOBALS->tims.timecache=GLOBALS->tims.start;

	gdk_pointer_ungrab(event->time);

	if(event->button==3)	/* oh yeah, dragzoooooooom! */
		{
		service_dragzoom(GLOBALS->tims.lmbcache, GLOBALS->tims.marker);
		}

	GLOBALS->tims.lmbcache=-1;
	update_markertime(GLOBALS->tims.marker);
	}

GLOBALS->mouseover_counter = 11;
move_mouseover(NULL, 0, 0, LLDescriptor(0));
GLOBALS->tims.timecache=0;

if(GLOBALS->prev_markertime == LLDescriptor(-1))
	{
	signalarea_configure_event( GLOBALS->signalarea, NULL );
	}

return(TRUE);
}


void make_sigarea_gcs(GtkWidget *signalarea)
{
if(!GLOBALS->made_sgc_contexts_wavewindow_c_1)
	{
	GLOBALS->gc_white = alloc_color(signalarea, GLOBALS->color_white, signalarea->style->white_gc);
	GLOBALS->gc_black = alloc_color(signalarea, GLOBALS->color_black, signalarea->style->black_gc);
	GLOBALS->gc_ltgray= alloc_color(signalarea, GLOBALS->color_ltgray, signalarea->style->bg_gc[GTK_STATE_PRELIGHT]);
	GLOBALS->gc_normal= alloc_color(signalarea, GLOBALS->color_normal, signalarea->style->bg_gc[GTK_STATE_NORMAL]);
	GLOBALS->gc_mdgray= alloc_color(signalarea, GLOBALS->color_mdgray, signalarea->style->bg_gc[GTK_STATE_INSENSITIVE]);
	GLOBALS->gc_dkgray= alloc_color(signalarea, GLOBALS->color_dkgray, signalarea->style->bg_gc[GTK_STATE_ACTIVE]);
	GLOBALS->gc_dkblue= alloc_color(signalarea, GLOBALS->color_dkblue, signalarea->style->bg_gc[GTK_STATE_SELECTED]);
	GLOBALS->gc_brkred= alloc_color(signalarea, GLOBALS->color_brkred, signalarea->style->bg_gc[GTK_STATE_SELECTED]);
	GLOBALS->gc_ltblue= alloc_color(signalarea, GLOBALS->color_ltblue, signalarea->style->bg_gc[GTK_STATE_SELECTED]);
	GLOBALS->gc_gmstrd= alloc_color(signalarea, GLOBALS->color_gmstrd, signalarea->style->bg_gc[GTK_STATE_SELECTED]);

	GLOBALS->made_sgc_contexts_wavewindow_c_1=~0;
	}
}

gint wavearea_configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
if((!widget)||(!widget->window)) return(TRUE);

DEBUG(printf("WaveWin Configure Event h: %d, w: %d\n",widget->allocation.height,
		widget->allocation.width));

if(GLOBALS->wavepixmap_wavewindow_c_1)
	{
	if((GLOBALS->wavewidth!=widget->allocation.width)||(GLOBALS->waveheight!=widget->allocation.height))
		{
		gdk_pixmap_unref(GLOBALS->wavepixmap_wavewindow_c_1);
		GLOBALS->wavepixmap_wavewindow_c_1=gdk_pixmap_new(widget->window, GLOBALS->wavewidth=widget->allocation.width,
			GLOBALS->waveheight=widget->allocation.height, -1);
		}
	GLOBALS->old_wvalue=-1.0;
	}
	else
	{
	GLOBALS->wavepixmap_wavewindow_c_1=gdk_pixmap_new(widget->window, GLOBALS->wavewidth=widget->allocation.width,
		GLOBALS->waveheight=widget->allocation.height, -1);
	}

if(!GLOBALS->made_gc_contexts_wavewindow_c_1)
	{
	GLOBALS->gc_back_wavewindow_c_1   = alloc_color(GLOBALS->wavearea, GLOBALS->color_back, GLOBALS->wavearea->style->white_gc);    
	GLOBALS->gc_baseline_wavewindow_c_1 = alloc_color(GLOBALS->wavearea, GLOBALS->color_baseline, GLOBALS->wavearea->style->bg_gc[GTK_STATE_SELECTED]);    
	GLOBALS->gc_grid_wavewindow_c_1   = alloc_color(GLOBALS->wavearea, GLOBALS->color_grid, GLOBALS->wavearea->style->bg_gc[GTK_STATE_PRELIGHT]);
	GLOBALS->gc_grid2_wavewindow_c_1  = alloc_color(GLOBALS->wavearea, GLOBALS->color_grid2, GLOBALS->wavearea->style->bg_gc[GTK_STATE_ACTIVE]);
	GLOBALS->gc_time_wavewindow_c_1   = alloc_color(GLOBALS->wavearea, GLOBALS->color_time, GLOBALS->wavearea->style->black_gc);
	GLOBALS->gc_timeb_wavewindow_c_1  = alloc_color(GLOBALS->wavearea, GLOBALS->color_timeb, GLOBALS->wavearea->style->bg_gc[GTK_STATE_ACTIVE]);
	GLOBALS->gc_value_wavewindow_c_1  = alloc_color(GLOBALS->wavearea, GLOBALS->color_value, GLOBALS->wavearea->style->black_gc);
	GLOBALS->gc_low_wavewindow_c_1    = alloc_color(GLOBALS->wavearea, GLOBALS->color_low, GLOBALS->wavearea->style->black_gc);    
	GLOBALS->gc_high_wavewindow_c_1   = alloc_color(GLOBALS->wavearea, GLOBALS->color_high, GLOBALS->wavearea->style->black_gc);    
	GLOBALS->gc_trans_wavewindow_c_1  = alloc_color(GLOBALS->wavearea, GLOBALS->color_trans, GLOBALS->wavearea->style->black_gc);    
	GLOBALS->gc_mid_wavewindow_c_1    = alloc_color(GLOBALS->wavearea, GLOBALS->color_mid, GLOBALS->wavearea->style->black_gc);    
	GLOBALS->gc_xfill_wavewindow_c_1  = alloc_color(GLOBALS->wavearea, GLOBALS->color_xfill, GLOBALS->wavearea->style->bg_gc[GTK_STATE_PRELIGHT]);
	GLOBALS->gc_x_wavewindow_c_1      = alloc_color(GLOBALS->wavearea, GLOBALS->color_x, GLOBALS->wavearea->style->black_gc);
	GLOBALS->gc_vbox_wavewindow_c_1   = alloc_color(GLOBALS->wavearea, GLOBALS->color_vbox, GLOBALS->wavearea->style->black_gc);
	GLOBALS->gc_vtrans_wavewindow_c_1 = alloc_color(GLOBALS->wavearea, GLOBALS->color_vtrans, GLOBALS->wavearea->style->black_gc);
	GLOBALS->gc_mark_wavewindow_c_1   = alloc_color(GLOBALS->wavearea, GLOBALS->color_mark, GLOBALS->wavearea->style->bg_gc[GTK_STATE_SELECTED]);
	GLOBALS->gc_umark_wavewindow_c_1  = alloc_color(GLOBALS->wavearea, GLOBALS->color_umark, GLOBALS->wavearea->style->bg_gc[GTK_STATE_SELECTED]);

#ifdef WAVE_DOUBLE_LINE_WIDTH_MODE
	gdk_gc_set_line_attributes(GLOBALS->gc_value_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_low_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_high_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_trans_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_mid_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_xfill_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_x_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_vbox_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_vtrans_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_mark_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_umark_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
#endif

        GLOBALS->gc_0_wavewindow_c_1      = alloc_color(GLOBALS->wavearea, GLOBALS->color_0, GLOBALS->wavearea->style->black_gc);
        GLOBALS->gc_1_wavewindow_c_1      = alloc_color(GLOBALS->wavearea, GLOBALS->color_1, GLOBALS->wavearea->style->black_gc);
        GLOBALS->gc_ufill_wavewindow_c_1  = alloc_color(GLOBALS->wavearea, GLOBALS->color_ufill, GLOBALS->wavearea->style->bg_gc[GTK_STATE_PRELIGHT]);
        GLOBALS->gc_u_wavewindow_c_1      = alloc_color(GLOBALS->wavearea, GLOBALS->color_u, GLOBALS->wavearea->style->black_gc);
        GLOBALS->gc_wfill_wavewindow_c_1  = alloc_color(GLOBALS->wavearea, GLOBALS->color_wfill, GLOBALS->wavearea->style->bg_gc[GTK_STATE_PRELIGHT]);
        GLOBALS->gc_w_wavewindow_c_1      = alloc_color(GLOBALS->wavearea, GLOBALS->color_w, GLOBALS->wavearea->style->black_gc);
        GLOBALS->gc_dashfill_wavewindow_c_1= alloc_color(GLOBALS->wavearea, GLOBALS->color_dashfill, GLOBALS->wavearea->style->bg_gc[GTK_STATE_PRELIGHT]);
        GLOBALS->gc_dash_wavewindow_c_1   = alloc_color(GLOBALS->wavearea, GLOBALS->color_dash, GLOBALS->wavearea->style->black_gc);

#ifdef WAVE_DOUBLE_LINE_WIDTH_MODE
	gdk_gc_set_line_attributes(GLOBALS->gc_0_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_1_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_ufill_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_u_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_wfill_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_w_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_dashfill_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_line_attributes(GLOBALS->gc_dash_wavewindow_c_1, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
#endif

	GLOBALS->made_gc_contexts_wavewindow_c_1=~0;

	GLOBALS->gccache_ltgray = GLOBALS->gc_ltgray ; 
	GLOBALS->gccache_normal = GLOBALS->gc_normal ; 
	GLOBALS->gccache_mdgray = GLOBALS->gc_mdgray ; 
	GLOBALS->gccache_dkgray = GLOBALS->gc_dkgray ; 
	GLOBALS->gccache_dkblue = GLOBALS->gc_dkblue ; 
	GLOBALS->gccache_brkred = GLOBALS->gc_brkred ; 
	GLOBALS->gccache_ltblue = GLOBALS->gc_ltblue ; 
	GLOBALS->gccache_gmstrd = GLOBALS->gc_gmstrd ; 
	GLOBALS->gccache_back_wavewindow_c_1 = GLOBALS->gc_back_wavewindow_c_1 ; 
	GLOBALS->gccache_baseline_wavewindow_c_1 = GLOBALS->gc_baseline_wavewindow_c_1 ; 
	GLOBALS->gccache_grid_wavewindow_c_1 = GLOBALS->gc_grid_wavewindow_c_1 ; 
	GLOBALS->gccache_grid2_wavewindow_c_1 = GLOBALS->gc_grid2_wavewindow_c_1 ; 
	GLOBALS->gccache_time_wavewindow_c_1 = GLOBALS->gc_time_wavewindow_c_1 ; 
	GLOBALS->gccache_timeb_wavewindow_c_1 = GLOBALS->gc_timeb_wavewindow_c_1 ; 
	GLOBALS->gccache_value_wavewindow_c_1 = GLOBALS->gc_value_wavewindow_c_1 ; 
	GLOBALS->gccache_low_wavewindow_c_1 = GLOBALS->gc_low_wavewindow_c_1 ; 
	GLOBALS->gccache_high_wavewindow_c_1 = GLOBALS->gc_high_wavewindow_c_1 ; 
	GLOBALS->gccache_trans_wavewindow_c_1 = GLOBALS->gc_trans_wavewindow_c_1 ; 
	GLOBALS->gccache_mid_wavewindow_c_1 = GLOBALS->gc_mid_wavewindow_c_1 ; 
	GLOBALS->gccache_xfill_wavewindow_c_1 = GLOBALS->gc_xfill_wavewindow_c_1 ; 
	GLOBALS->gccache_x_wavewindow_c_1 = GLOBALS->gc_x_wavewindow_c_1 ; 
	GLOBALS->gccache_vbox_wavewindow_c_1 = GLOBALS->gc_vbox_wavewindow_c_1 ; 
	GLOBALS->gccache_vtrans_wavewindow_c_1 = GLOBALS->gc_vtrans_wavewindow_c_1 ; 
	GLOBALS->gccache_mark_wavewindow_c_1 = GLOBALS->gc_mark_wavewindow_c_1 ; 
	GLOBALS->gccache_umark_wavewindow_c_1 = GLOBALS->gc_umark_wavewindow_c_1 ; 
	GLOBALS->gccache_0_wavewindow_c_1 = GLOBALS->gc_0_wavewindow_c_1 ; 
	GLOBALS->gccache_1_wavewindow_c_1 = GLOBALS->gc_1_wavewindow_c_1 ; 
	GLOBALS->gccache_ufill_wavewindow_c_1 = GLOBALS->gc_ufill_wavewindow_c_1 ; 
	GLOBALS->gccache_u_wavewindow_c_1 = GLOBALS->gc_u_wavewindow_c_1 ; 
	GLOBALS->gccache_wfill_wavewindow_c_1 = GLOBALS->gc_wfill_wavewindow_c_1 ; 
	GLOBALS->gccache_w_wavewindow_c_1 = GLOBALS->gc_w_wavewindow_c_1 ; 
	GLOBALS->gccache_dashfill_wavewindow_c_1 = GLOBALS->gc_dashfill_wavewindow_c_1 ; 
	GLOBALS->gccache_dash_wavewindow_c_1 = GLOBALS->gc_dash_wavewindow_c_1 ; 
	}

if(GLOBALS->timestart_from_savefile_valid)
	{
	gfloat pageinc=(gfloat)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);

	if((GLOBALS->timestart_from_savefile >= GLOBALS->tims.first) && (GLOBALS->timestart_from_savefile <= (GLOBALS->tims.last-pageinc)))
		{
		GtkAdjustment *hadj = GTK_ADJUSTMENT(GLOBALS->wave_hslider);
		hadj->value = GLOBALS->timestart_from_savefile;
		fix_wavehadj();
		gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */ 
		gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
		}
	GLOBALS->timestart_from_savefile_valid = 0;
	}

if(GLOBALS->wavewidth>1)
	{
	if(!GLOBALS->do_initial_zoom_fit)
		{
		calczoom(GLOBALS->tims.zoom);
		fix_wavehadj();
		gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */ 
		gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
		}
		else
		{
		GLOBALS->do_initial_zoom_fit=0;
		service_zoom_fit(NULL,NULL);
		}	
	}

/* tims.timecache=tims.laststart; */
return(TRUE);
}

/*
 * screengrab vs normal rendering gcs...
 */ 
void force_screengrab_gcs(void)
{
GLOBALS->black_and_white = 1;

GLOBALS->gc_ltgray= GLOBALS->gc_white ;
GLOBALS->gc_normal= GLOBALS->gc_white ;
GLOBALS->gc_mdgray= GLOBALS->gc_white ;
GLOBALS->gc_dkgray= GLOBALS->gc_white ;
GLOBALS->gc_dkblue= GLOBALS->gc_black ;
GLOBALS->gc_brkred= GLOBALS->gc_black ;
GLOBALS->gc_ltblue= GLOBALS->gc_black ;
GLOBALS->gc_gmstrd= GLOBALS->gc_black ;
GLOBALS->gc_back_wavewindow_c_1   = GLOBALS->gc_white ;
GLOBALS->gc_baseline_wavewindow_c_1 = GLOBALS->gc_black;
GLOBALS->gc_grid_wavewindow_c_1   = GLOBALS->gc_black;
GLOBALS->gc_grid2_wavewindow_c_1  = GLOBALS->gc_black;
GLOBALS->gc_time_wavewindow_c_1   = GLOBALS->gc_black;
GLOBALS->gc_timeb_wavewindow_c_1  = GLOBALS->gc_white;
GLOBALS->gc_value_wavewindow_c_1  = GLOBALS->gc_black;
GLOBALS->gc_low_wavewindow_c_1    = GLOBALS->gc_black;
GLOBALS->gc_high_wavewindow_c_1   = GLOBALS->gc_black;
GLOBALS->gc_trans_wavewindow_c_1  = GLOBALS->gc_black;
GLOBALS->gc_mid_wavewindow_c_1    = GLOBALS->gc_black;
GLOBALS->gc_xfill_wavewindow_c_1  = GLOBALS->gc_black;
GLOBALS->gc_x_wavewindow_c_1      = GLOBALS->gc_black;
GLOBALS->gc_vbox_wavewindow_c_1   = GLOBALS->gc_black;
GLOBALS->gc_vtrans_wavewindow_c_1 = GLOBALS->gc_black;
GLOBALS->gc_mark_wavewindow_c_1   = GLOBALS->gc_black;
GLOBALS->gc_umark_wavewindow_c_1  = GLOBALS->gc_black;
GLOBALS->gc_0_wavewindow_c_1      = GLOBALS->gc_black;
GLOBALS->gc_1_wavewindow_c_1      = GLOBALS->gc_black;
GLOBALS->gc_ufill_wavewindow_c_1  = GLOBALS->gc_black;
GLOBALS->gc_u_wavewindow_c_1      = GLOBALS->gc_black;
GLOBALS->gc_wfill_wavewindow_c_1  = GLOBALS->gc_black;
GLOBALS->gc_w_wavewindow_c_1      = GLOBALS->gc_black;
GLOBALS->gc_dashfill_wavewindow_c_1= GLOBALS->gc_black;
GLOBALS->gc_dash_wavewindow_c_1   = GLOBALS->gc_black;
}

void force_normal_gcs(void)
{
GLOBALS->black_and_white = 0;

GLOBALS->gc_ltgray = GLOBALS->gccache_ltgray ;
GLOBALS->gc_normal = GLOBALS->gccache_normal ;
GLOBALS->gc_mdgray = GLOBALS->gccache_mdgray ;
GLOBALS->gc_dkgray = GLOBALS->gccache_dkgray ;
GLOBALS->gc_dkblue = GLOBALS->gccache_dkblue ;
GLOBALS->gc_brkred = GLOBALS->gccache_brkred ;
GLOBALS->gc_ltblue = GLOBALS->gccache_ltblue ;
GLOBALS->gc_gmstrd = GLOBALS->gccache_gmstrd ;
GLOBALS->gc_back_wavewindow_c_1 = GLOBALS->gccache_back_wavewindow_c_1 ;
GLOBALS->gc_baseline_wavewindow_c_1 = GLOBALS->gccache_baseline_wavewindow_c_1 ;
GLOBALS->gc_grid_wavewindow_c_1 = GLOBALS->gccache_grid_wavewindow_c_1 ;
GLOBALS->gc_grid2_wavewindow_c_1 = GLOBALS->gccache_grid2_wavewindow_c_1 ;
GLOBALS->gc_time_wavewindow_c_1 = GLOBALS->gccache_time_wavewindow_c_1 ;
GLOBALS->gc_timeb_wavewindow_c_1 = GLOBALS->gccache_timeb_wavewindow_c_1 ;
GLOBALS->gc_value_wavewindow_c_1 = GLOBALS->gccache_value_wavewindow_c_1 ;
GLOBALS->gc_low_wavewindow_c_1 = GLOBALS->gccache_low_wavewindow_c_1 ;
GLOBALS->gc_high_wavewindow_c_1 = GLOBALS->gccache_high_wavewindow_c_1 ;
GLOBALS->gc_trans_wavewindow_c_1 = GLOBALS->gccache_trans_wavewindow_c_1 ;
GLOBALS->gc_mid_wavewindow_c_1 = GLOBALS->gccache_mid_wavewindow_c_1 ;
GLOBALS->gc_xfill_wavewindow_c_1 = GLOBALS->gccache_xfill_wavewindow_c_1 ;
GLOBALS->gc_x_wavewindow_c_1 = GLOBALS->gccache_x_wavewindow_c_1 ;
GLOBALS->gc_vbox_wavewindow_c_1 = GLOBALS->gccache_vbox_wavewindow_c_1 ;
GLOBALS->gc_vtrans_wavewindow_c_1 = GLOBALS->gccache_vtrans_wavewindow_c_1 ;
GLOBALS->gc_mark_wavewindow_c_1 = GLOBALS->gccache_mark_wavewindow_c_1 ;
GLOBALS->gc_umark_wavewindow_c_1 = GLOBALS->gccache_umark_wavewindow_c_1 ;
GLOBALS->gc_0_wavewindow_c_1 = GLOBALS->gccache_0_wavewindow_c_1 ;
GLOBALS->gc_1_wavewindow_c_1 = GLOBALS->gccache_1_wavewindow_c_1 ;
GLOBALS->gc_ufill_wavewindow_c_1 = GLOBALS->gccache_ufill_wavewindow_c_1 ;
GLOBALS->gc_u_wavewindow_c_1 = GLOBALS->gccache_u_wavewindow_c_1 ;
GLOBALS->gc_wfill_wavewindow_c_1 = GLOBALS->gccache_wfill_wavewindow_c_1 ;
GLOBALS->gc_w_wavewindow_c_1 = GLOBALS->gccache_w_wavewindow_c_1 ;
GLOBALS->gc_dashfill_wavewindow_c_1 = GLOBALS->gccache_dashfill_wavewindow_c_1 ;
GLOBALS->gc_dash_wavewindow_c_1 = GLOBALS->gccache_dash_wavewindow_c_1 ;
}

static gint wavearea_configure_event_local(GtkWidget *widget, GdkEventConfigure *event)
{
gint rc;
gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(GLOBALS->notebook));
struct Global *g_old = GLOBALS;

set_GLOBALS((*GLOBALS->contexts)[page_num]);

rc = wavearea_configure_event(widget, event);

set_GLOBALS(g_old);

return(rc);
}


static gint expose_event(GtkWidget *widget, GdkEventExpose *event)
{
gdk_draw_pixmap(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)],GLOBALS->wavepixmap_wavewindow_c_1, event->area.x, event->area.y,event->area.x, event->area.y,event->area.width, event->area.height);
draw_marker();

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


GtkWidget *
create_wavewindow(void)
{
GtkWidget *table;
GtkWidget *frame;
GtkAdjustment *hadj, *vadj;

table = gtk_table_new(10, 10, FALSE);
GLOBALS->wavearea=gtk_drawing_area_new();
gtk_widget_show(GLOBALS->wavearea);

gtk_widget_set_events(GLOBALS->wavearea,
#ifdef WAVE_USE_GTK2
		GDK_SCROLL_MASK |
#endif
                GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
                GDK_BUTTON_RELEASE_MASK |
                GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK
                );

gtk_signal_connect(GTK_OBJECT(GLOBALS->wavearea), "configure_event",GTK_SIGNAL_FUNC(wavearea_configure_event_local), NULL);
gtk_signal_connect(GTK_OBJECT(GLOBALS->wavearea), "expose_event",GTK_SIGNAL_FUNC(expose_event_local), NULL);

gtkwave_signal_connect(GTK_OBJECT(GLOBALS->wavearea), "motion_notify_event",GTK_SIGNAL_FUNC(motion_notify_event), NULL);
gtkwave_signal_connect(GTK_OBJECT(GLOBALS->wavearea), "button_press_event",GTK_SIGNAL_FUNC(button_press_event), NULL);
gtkwave_signal_connect(GTK_OBJECT(GLOBALS->wavearea), "button_release_event",GTK_SIGNAL_FUNC(button_release_event), NULL);

#ifdef WAVE_USE_GTK2
gtkwave_signal_connect(GTK_OBJECT(GLOBALS->wavearea), "scroll_event",GTK_SIGNAL_FUNC(scroll_event), NULL);
GTK_WIDGET_SET_FLAGS( GLOBALS->wavearea, GTK_CAN_FOCUS );
#endif

gtk_table_attach (GTK_TABLE (table), GLOBALS->wavearea, 0, 9, 0, 9,GTK_FILL | GTK_EXPAND,GTK_FILL | GTK_EXPAND, 3, 2);

GLOBALS->wave_vslider=gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
vadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
gtkwave_signal_connect(GTK_OBJECT(GLOBALS->wave_vslider), "value_changed",GTK_SIGNAL_FUNC(service_vslider), NULL);
GLOBALS->vscroll_wavewindow_c_1=gtk_vscrollbar_new(vadj);
/* GTK_WIDGET_SET_FLAGS(GLOBALS->vscroll_wavewindow_c_1, GTK_CAN_FOCUS); */
gtk_widget_show(GLOBALS->vscroll_wavewindow_c_1);
gtk_table_attach (GTK_TABLE (table), GLOBALS->vscroll_wavewindow_c_1, 9, 10, 0, 9,
                        GTK_FILL,
                        GTK_FILL | GTK_SHRINK, 3, 3);

GLOBALS->wave_hslider=gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
hadj=GTK_ADJUSTMENT(GLOBALS->wave_hslider);
gtkwave_signal_connect(GTK_OBJECT(GLOBALS->wave_hslider), "value_changed",GTK_SIGNAL_FUNC(service_hslider), NULL);
GLOBALS->hscroll_wavewindow_c_2=gtk_hscrollbar_new(hadj);
/* GTK_WIDGET_SET_FLAGS(GLOBALS->hscroll_wavewindow_c_2, GTK_CAN_FOCUS); */
gtk_widget_show(GLOBALS->hscroll_wavewindow_c_2);

gtk_table_attach (GTK_TABLE (table), GLOBALS->hscroll_wavewindow_c_2, 0, 9, 9, 10,
                        GTK_FILL,
                        GTK_FILL | GTK_SHRINK, 3, 4);
gtk_widget_show(table);

frame=gtk_frame_new("Waves");
gtk_container_border_width(GTK_CONTAINER(frame),2);

gtk_container_add(GTK_CONTAINER(frame),table);
return(frame);
}
   

/**********************************************/

void RenderSigs(int trtarget, int update_waves)
{
Trptr t;
int i, trwhich;
int num_traces_displayable;
GtkAdjustment *hadj;
int xsrc;

hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
xsrc=(gint)hadj->value;

num_traces_displayable=GLOBALS->signalarea->allocation.height/(GLOBALS->fontheight);
num_traces_displayable--;   /* for the time trace that is always there */

gdk_draw_rectangle(GLOBALS->signalpixmap, GLOBALS->gc_mdgray, TRUE, 0, -1, GLOBALS->signal_fill_width, GLOBALS->fontheight); 
gdk_draw_line(GLOBALS->signalpixmap, GLOBALS->gc_white, 0, GLOBALS->fontheight-1, GLOBALS->signal_fill_width-1, GLOBALS->fontheight-1);
font_engine_draw_string(GLOBALS->signalpixmap, GLOBALS->signalfont, GLOBALS->gc_black, 3+xsrc, GLOBALS->fontheight-4, "Time");

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

GLOBALS->topmost_trace=t;
if(t)
        {
        for(i=0;(i<num_traces_displayable)&&(t);i++)
                {
		RenderSig(t, i, 0);
                t=GiveNextTrace(t);
                }
        }

if(GLOBALS->signalarea_has_focus)
	{
	gdk_draw_rectangle(GLOBALS->signalpixmap, GLOBALS->gc_black, FALSE, 0, 0, GLOBALS->signal_fill_width-1, GLOBALS->signalarea->allocation.height-1); 
	}

if((GLOBALS->wavepixmap_wavewindow_c_1)&&(update_waves))
	{
        gdk_draw_rectangle(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_back_wavewindow_c_1, TRUE, 0, 0,GLOBALS->wavewidth, GLOBALS->waveheight);

	if(GLOBALS->display_grid) rendertimes();
	rendertraces();
	}
}


void populateBuffer (Trptr t, char *altname, char* buf)
{
  char* ptr = buf;
  char *tname = altname ? altname : t->name;

  if (HasWave(t))
    {
      if (tname) 
	{
	  strcpy(ptr, tname);
	  ptr = ptr + strlen(ptr);

	  if((tname)&&(t->shift))
	    {
	      ptr[0]='`';
	      reformat_time(ptr+1, t->shift, GLOBALS->time_dimension);
	      ptr = ptr + strlen(ptr+1) + 1;
	      strcpy(ptr,"\'");
	      ptr = ptr + strlen(ptr);
	    }

	  if((!t->vector)&&(t->n.nd)&&(t->n.nd->array_height))
	    {
	      sprintf(ptr, "{%d}", t->n.nd->this_row);
	      ptr = ptr + strlen(ptr);
	    }
	}

      if (IsGroupBegin(t))
	{
	  char * pch;
	  ptr = buf;
	  if (IsClosed(t)) {
	    pch = strstr (ptr,"[-]");
	    if(pch) {strncpy (pch,"[+]", 3); }
	  } else {
	    pch = strstr (ptr,"[+]");
	    if(pch) {strncpy (pch,"[-]", 3); }
	  }
	}
    }
  else
    {
      if (tname) 
	{

	  if (IsGroupEnd(t))
	    {
	      strcpy(ptr, "} ");
	      ptr = ptr + strlen(ptr);
	    }

	  strcpy(ptr, tname);
	  ptr = ptr + strlen(ptr);

	  if (IsGroupBegin(t))
	    {
	      if (IsClosed(t) && IsCollapsed(t->t_match))
		{
		  strcpy(ptr, " {}");
		}
	      else
		{
		  strcpy(ptr, " {");
		}
	      ptr = ptr + strlen(ptr);
	    }
	}
    }
}

/***************************************************************************/

int RenderSig(Trptr t, int i, int dobackground)
{
  int texty, liney;
  int retval;
  char buf[2048];
  GdkGC *clr_comment;
  GdkGC *clr_group;
  GdkGC *clr_shadowed;
  GdkGC *clr_signal;
  GdkGC* bg_color;  
  GdkGC* text_color;
  unsigned left_justify;
  char *subname = NULL; 
  bvptr bv = NULL;

  buf[0] = 0;

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
                bv = tscan->n.vec;
                do
                        {
                        bv = bv->transaction_chain; /* correlate to blank trace */
                        } while(bv && (bcnt--));
                if(bv)
                        {
                        subname = bv->bvname;
                        if(GLOBALS->hier_max_level)
                                subname = hier_extract(subname, GLOBALS->hier_max_level);
                        }
                }
        }

  populateBuffer(t, subname, buf);

  clr_comment  = GLOBALS->gc_brkred;
  clr_group    = GLOBALS->gc_gmstrd;
  clr_shadowed = GLOBALS->gc_ltblue;
  clr_signal   = GLOBALS->gc_dkblue;

  UpdateSigValue(t); /* in case it's stale on nonprop */

  liney=((i+2)*GLOBALS->fontheight)-2;
  texty=liney-(GLOBALS->signalfont->descent);

  retval=liney-GLOBALS->fontheight+1;

  left_justify = ((IsGroupBegin(t) || IsGroupEnd(t)) && !HasWave(t))|| GLOBALS->left_justify_sigs;

  if (IsSelected(t))
    {
      bg_color = (!HasWave(t))
	? ((IsGroupBegin(t) || IsGroupEnd(t)) ? clr_group : clr_comment)
	: ((IsShadowed(t)) ? clr_shadowed : clr_signal);
      text_color = GLOBALS->gc_white;
    }
  else
    {
      bg_color = (dobackground==2) ?  GLOBALS->gc_normal : GLOBALS->gc_ltgray;
      if(HasWave(t))
	{ text_color = GLOBALS->gc_black; }
      else
	{ text_color = (IsGroupBegin(t) || IsGroupEnd(t)) ? clr_group : clr_comment; }
    }

  if (dobackground || IsSelected(t))
    {

      gdk_draw_rectangle(GLOBALS->signalpixmap, bg_color, TRUE, 
			 0, retval,
			 GLOBALS->signal_fill_width, GLOBALS->fontheight-1);

    }

  gdk_draw_line(GLOBALS->signalpixmap, 
		GLOBALS->gc_white,
		0, liney,
		GLOBALS->signal_fill_width-1, liney);

  if((t->name)||(subname))
    {
      font_engine_draw_string(GLOBALS->signalpixmap,
			      GLOBALS->signalfont,
			      text_color,
			      left_justify?3:3+GLOBALS->max_signal_name_pixel_width-
			      font_engine_string_measure(GLOBALS->signalfont, buf), 
			      texty,
			      buf);

      
    }

  if (HasWave(t) || bv)
    {
      if((t->asciivalue)&&(!(t->flags&TR_EXCLUDE)))
	font_engine_draw_string(GLOBALS->signalpixmap,
				GLOBALS->signalfont,
				text_color,
				GLOBALS->max_signal_name_pixel_width+6,
				texty,
				t->asciivalue);
    }

  return(retval);
}


/***************************************************************************/

void MaxSignalLength(void)
{
Trptr t;
int len=0,maxlen=0;
int vlen=0, vmaxlen=0;
char buf[2048];
char dirty_kick;
bvptr bv;
Trptr tscan;

DEBUG(printf("signalwindow_width_dirty: %d\n",signalwindow_width_dirty));

if((!GLOBALS->signalwindow_width_dirty)&&(GLOBALS->use_nonprop_fonts)) return;

dirty_kick = GLOBALS->signalwindow_width_dirty;
GLOBALS->signalwindow_width_dirty=0;

t=GLOBALS->traces.first;


while(t)
  {
  char *subname = NULL;
  bv = NULL;
  tscan = NULL;

  if(t->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH))  /* seek to real xact trace if present... */
        {
        tscan = t;
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
                bv = tscan->n.vec;
                do
                        {
                        bv = bv->transaction_chain; /* correlate to blank trace */
                        } while(bv && (bcnt--));
                if(bv)
                        {
                        subname = bv->bvname;
                        if(GLOBALS->hier_max_level)
                                subname = hier_extract(subname, GLOBALS->hier_max_level);
                        }
                }
        }

    populateBuffer(t, subname, buf);

    if(!bv && (t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))	/* for "comment" style blank traces */
      {
	if(t->name || subname)
	  {
	    len=font_engine_string_measure(GLOBALS->signalfont, buf);
	    if(len>maxlen) maxlen=len;
	  }

	if(t->asciivalue)
		{
		free_2(t->asciivalue); t->asciivalue = NULL;
		}
	t=GiveNextTrace(t);
      }
    else
      if(t->name || subname)
	{
	  len=font_engine_string_measure(GLOBALS->signalfont, buf);
	  if(len>maxlen) maxlen=len;

	  if((GLOBALS->tims.marker!=-1)&&(!(t->flags&TR_EXCLUDE)))
		{
		t->asciitime=GLOBALS->tims.marker;
		if(t->asciivalue) { free_2(t->asciivalue); } t->asciivalue = NULL;

		if(bv || t->vector)
			{
			char *str, *str2;
			vptr v;
			Trptr ts;
			TraceEnt t_temp;

			if(bv)
				{
				ts = &t_temp;
				memcpy(ts, tscan, sizeof(TraceEnt));
				ts->vector = 1;
				ts->n.vec = bv;
				}
				else
				{
				ts = t;
				bv = t->n.vec;
				}


                        v=bsearch_vector(bv, GLOBALS->tims.marker - ts->shift);
                        str=convert_ascii(ts,v);
			if(str)
				{
				str2=(char *)malloc_2(strlen(str)+2);
				*str2='=';
				strcpy(str2+1,str);
				free_2(str);

				vlen=font_engine_string_measure(GLOBALS->signalfont,str2);
				t->asciivalue=str2;
				}
				else
				{
				vlen=0;
				t->asciivalue=NULL;
				}
			}
			else
			{
			char *str;
			hptr h_ptr;
			if((h_ptr=bsearch_node(t->n.nd,GLOBALS->tims.marker - t->shift)))
				{
				if(!t->n.nd->extvals)
					{
					unsigned char h_val = h_ptr->v.h_val;

					str=(char *)calloc_2(1,3*sizeof(char));
					str[0]='=';
					if(t->n.nd->vartype == ND_VCD_EVENT)
						{
						h_val = (h_ptr->time >= GLOBALS->tims.first) && ((GLOBALS->tims.marker-GLOBALS->shift_timebase) == h_ptr->time) ? AN_1 : AN_0; /* generate impulse */
						}

					if(t->flags&TR_INVERT)
						{
						str[1]=AN_STR_INV[h_val];
						}
						else
						{
						str[1]=AN_STR[h_val];
						}
					t->asciivalue=str;
					vlen=font_engine_string_measure(GLOBALS->signalfont,str);
					}
					else
					{
					char *str2;

					if(h_ptr->flags&HIST_REAL)
						{
						if(!(h_ptr->flags&HIST_STRING))
							{
							str=convert_ascii_real(t, (double *)h_ptr->v.h_vector);
							}
							else
							{
							str=convert_ascii_string((char *)h_ptr->v.h_vector);
							}
						}
						else
						{
		                        	str=convert_ascii_vec(t,h_ptr->v.h_vector);
						}

					if(str)
						{
						str2=(char *)malloc_2(strlen(str)+2);
						*str2='=';
						strcpy(str2+1,str);

						free_2(str); 

						vlen=font_engine_string_measure(GLOBALS->signalfont,str2);
						t->asciivalue=str2;
						}
						else
						{
						vlen=0;
						t->asciivalue=NULL;
						}
					}
				}
				else
				{
				vlen=0;
				t->asciivalue=NULL;
				}
			}

		if(vlen>vmaxlen)
			{
			vmaxlen=vlen;
			}
		}

	t=GiveNextTrace(t);
	}
	else
	{
	t=GiveNextTrace(t);
	}
}

GLOBALS->max_signal_name_pixel_width = maxlen;
GLOBALS->signal_pixmap_width=maxlen+6; 		/* 2 * 3 pixel pad */
if(GLOBALS->tims.marker!=-1)
	{
	GLOBALS->signal_pixmap_width+=(vmaxlen+6);
	}

if(GLOBALS->signal_pixmap_width<60) GLOBALS->signal_pixmap_width=60;

if(!GLOBALS->in_button_press_wavewindow_c_1)
	{
	if(!GLOBALS->do_resize_signals)
		{
		int os;
		os=48;
		if(GLOBALS->initial_signal_window_width > os)
			{
			os = GLOBALS->initial_signal_window_width;
			}

		if(GLOBALS->signalwindow)
			{
			gtk_widget_set_usize(GTK_WIDGET(GLOBALS->signalwindow), os+30, -1);
			}
		}
	else
	if((GLOBALS->do_resize_signals)&&(GLOBALS->signalwindow))
		{
		int oldusize;
		int rs;

                if(GLOBALS->initial_signal_window_width > GLOBALS->max_signal_name_pixel_width)
                        {
                        rs=GLOBALS->initial_signal_window_width;
                        }
                        else
                        {
                        rs=GLOBALS->max_signal_name_pixel_width;
                        }


		oldusize=GLOBALS->signalwindow->allocation.width;
		if((oldusize!=rs)||(dirty_kick))
			{ /* keep signalwindow from expanding arbitrarily large */
#ifdef WAVE_USE_GTK2
			int wx, wy;
			get_window_size(&wx, &wy);

			if((3*rs) < (2*wx))	/* 2/3 width max */
#else
			if((3*rs) < (2*(GLOBALS->wavewidth + GLOBALS->signalwindow->allocation.width)))
#endif
				{
				int os;
				os=rs;
				os=(os<48)?48:os;
				gtk_widget_set_usize(GTK_WIDGET(GLOBALS->signalwindow), 
						os+30, -1);
				}
				else
				{
				int os;
				os=48;
		                if(GLOBALS->initial_signal_window_width > os)
                		        {
		                        os = GLOBALS->initial_signal_window_width;
		                        }

				gtk_widget_set_usize(GTK_WIDGET(GLOBALS->signalwindow), 
						os+30, -1);
				}
			}
		}
	}
}
/***************************************************************************/

void UpdateSigValue(Trptr t)
{
bvptr bv = NULL;
Trptr tscan = NULL;

if(!t) return;
if((t->asciivalue)&&(t->asciitime==GLOBALS->tims.marker))return;

if(t->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH))  /* seek to real xact trace if present... */
        {
        tscan = t;   
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
                bv = tscan->n.vec;
                do
                        {
                        bv = bv->transaction_chain; /* correlate to blank trace */
                        } while(bv && (bcnt--));
                if(bv)   
                        {
			/* nothing, we just want to set bv */
                        }
                }
        }


if((t->name || bv)&&(bv || !(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH))))
	{
	GLOBALS->shift_timebase=t->shift;
	DEBUG(printf("UpdateSigValue: %s\n",t->name));

	if((GLOBALS->tims.marker!=-1)&&(!(t->flags&TR_EXCLUDE)))
		{
		t->asciitime=GLOBALS->tims.marker;
		if(t->asciivalue) free_2(t->asciivalue);

		if(bv || t->vector)
			{
			char *str, *str2;
			vptr v;
                        Trptr ts;
                        TraceEnt t_temp;
         
                        if(bv)
                                {
                                ts = &t_temp;
                                memcpy(ts, tscan, sizeof(TraceEnt));
                                ts->vector = 1;
                                ts->n.vec = bv;
                                }
                                else
                                {
                                ts = t;
                                bv = t->n.vec;
				}

                        v=bsearch_vector(bv,GLOBALS->tims.marker - ts->shift);
                        str=convert_ascii(ts,v);
			if(str)
				{
				str2=(char *)malloc_2(strlen(str)+2);
				*str2='=';
				strcpy(str2+1,str);
				free_2(str);

				t->asciivalue=str2;
				}
				else
				{
				t->asciivalue=NULL;
				}
			}
			else
			{
			char *str;
			hptr h_ptr;
			if((h_ptr=bsearch_node(t->n.nd,GLOBALS->tims.marker - t->shift)))
				{
				if(!t->n.nd->extvals)
					{
					unsigned char h_val = h_ptr->v.h_val;
					if(t->n.nd->vartype == ND_VCD_EVENT)
						{
						h_val = (h_ptr->time >= GLOBALS->tims.first) && ((GLOBALS->tims.marker-GLOBALS->shift_timebase) == h_ptr->time) ? AN_1 : AN_0; /* generate impulse */ 
						}

					str=(char *)calloc_2(1,3*sizeof(char));
					str[0]='=';
					if(t->flags&TR_INVERT)
						{
						str[1]=AN_STR_INV[h_val];
						}
						else
						{
						str[1]=AN_STR[h_val];
						}
					t->asciivalue=str;
					}
					else
					{
					char *str2;

					if(h_ptr->flags&HIST_REAL)
						{
						if(!(h_ptr->flags&HIST_STRING))
							{
							str=convert_ascii_real(t, (double *)h_ptr->v.h_vector);
							}
							else
							{
							str=convert_ascii_string((char *)h_ptr->v.h_vector);
							}
						}
						else
						{
		                        	str=convert_ascii_vec(t,h_ptr->v.h_vector);
						}

					if(str)
						{
						str2=(char *)malloc_2(strlen(str)+2);
						*str2='=';
						strcpy(str2+1,str);
						free_2(str);

						t->asciivalue=str2;
						}
						else
						{
						t->asciivalue=NULL;
						}
					}
				}
				else
				{
				t->asciivalue=NULL;
				}
			}
		}
	}
}

/***************************************************************************/

void calczoom(gdouble z0)
{
gdouble ppf, frame;
ppf=((gdouble)(GLOBALS->pixelsperframe=200));
frame=pow(GLOBALS->zoombase,-z0);

if(frame>((gdouble)MAX_HISTENT_TIME/(gdouble)4.0))
	{
	GLOBALS->nsperframe=((gdouble)MAX_HISTENT_TIME/(gdouble)4.0);
	}
	else
	if(frame<(gdouble)1.0)
	{
	GLOBALS->nsperframe=1.0;
	}
	else
	{
	GLOBALS->nsperframe=frame;
	}

GLOBALS->hashstep=10.0;

if(GLOBALS->zoom_pow10_snap)
if(GLOBALS->nsperframe>10.0)
	{
	TimeType nsperframe2;
	gdouble p=10.0;
	gdouble scale;
	int l;
	l=(int)((log(GLOBALS->nsperframe)/log(p))+0.5);	/* nearest power of 10 */
	nsperframe2=pow(p, (gdouble)l);

	scale = (gdouble)nsperframe2 / (gdouble)GLOBALS->nsperframe;
	ppf *= scale;
	GLOBALS->pixelsperframe = ppf;

	GLOBALS->nsperframe = nsperframe2;
	GLOBALS->hashstep = ppf / 10.0;
	}

GLOBALS->nspx=GLOBALS->nsperframe/ppf;
GLOBALS->pxns=ppf/GLOBALS->nsperframe;

time_trunc_set();	/* map nspx to rounding value */

DEBUG(printf("Zoom: %e Pixelsperframe: %lld, nsperframe: %e\n",z0, pixelsperframe,(float)nsperframe));
}

static void renderhash(int x, TimeType tim)
{
TimeType rborder;
int fhminus2;
int rhs;
gdouble dx;
gdouble hashoffset;
int iter = 0;
int s_ctx_iter;
int timearray_encountered = 0;

fhminus2=GLOBALS->fontheight-2;

WAVE_STRACE_ITERATOR(s_ctx_iter)
	{
	GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];
	if(GLOBALS->strace_ctx->timearray)
		{
		timearray_encountered = 1;
		break;
		}
	}

gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_grid_wavewindow_c_1,x, 0,x, 
	((!timearray_encountered)&&(GLOBALS->display_grid)&&(GLOBALS->enable_vert_grid))?GLOBALS->waveheight:fhminus2);

if(tim==GLOBALS->tims.last) return;

rborder=(GLOBALS->tims.last-GLOBALS->tims.start)*GLOBALS->pxns;
DEBUG(printf("Rborder: %lld, Wavewidth: %d\n", rborder, wavewidth));
if(rborder>GLOBALS->wavewidth) rborder=GLOBALS->wavewidth;
if((rhs=x+GLOBALS->pixelsperframe)>rborder) rhs=rborder;

gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_grid_wavewindow_c_1,x, GLOBALS->wavecrosspiece,rhs, GLOBALS->wavecrosspiece);

dx = x + (hashoffset=GLOBALS->hashstep);
x  = dx;

while((hashoffset<GLOBALS->pixelsperframe)&&(x<=rhs)&&(iter<9))
	{
	gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_grid_wavewindow_c_1,x, GLOBALS->wavecrosspiece,x, fhminus2);

	hashoffset+=GLOBALS->hashstep;
	dx=dx+GLOBALS->hashstep;
	if((GLOBALS->pixelsperframe!=200)||(GLOBALS->hashstep!=10.0)) iter++; /* fix any roundoff errors */
	x = dx;
	} 

}

static void rendertimes(void)
{
int lastx = -1000; /* arbitrary */
int x, lenhalf;
TimeType tim, rem;
char timebuff[32];
char prevover=0;
gdouble realx;
int s_ctx_iter;

renderblackout();

tim=GLOBALS->tims.start;
GLOBALS->tims.end=GLOBALS->tims.start+(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);

wave_gdk_draw_line_flush(GLOBALS->wavepixmap_wavewindow_c_1); /* clear out state */

/***********/
WAVE_STRACE_ITERATOR_FWD(s_ctx_iter)
	{
	GdkGC * gc;

	if(!s_ctx_iter)
		{ 
		gc = GLOBALS->gc_grid_wavewindow_c_1;
		}
		else
		{
		gc = GLOBALS->gc_grid2_wavewindow_c_1;
		gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_BEVEL);
		}

	GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];

	if(GLOBALS->strace_ctx->timearray)
		{
		int pos, pos2;
		TimeType *t, tm;
		int y=GLOBALS->fontheight+2;
		int oldx=-1;

		pos=bsearch_timechain(GLOBALS->tims.start);
		top:
		if((pos>=0)&&(pos<GLOBALS->strace_ctx->timearray_size))
			{
			t=GLOBALS->strace_ctx->timearray+pos;
			for(;pos<GLOBALS->strace_ctx->timearray_size;t++, pos++)
				{
				tm=*t;
				if(tm>=GLOBALS->tims.start)
					{
					if(tm<=GLOBALS->tims.end)
						{
						x=(tm-GLOBALS->tims.start)*GLOBALS->pxns;
						if(oldx==x) 
							{
							pos2=bsearch_timechain(GLOBALS->tims.start+(((gdouble)(x+1))*GLOBALS->nspx));
							if(pos2>pos) { pos=pos2; goto top; } else continue;
							}
						oldx=x;
						gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, gc, x, y, x, GLOBALS->waveheight);
						}
						else
						{
						break;
						}
					}
				}
			}
		}

	
	wave_gdk_draw_line_flush(GLOBALS->wavepixmap_wavewindow_c_1); /* clear out state */

	if(s_ctx_iter)
		{
		gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_BEVEL);
		}
	}

GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = 0];
/***********/


DEBUG(printf("Ruler Start time: "TTFormat", Finish time: "TTFormat"\n",tims.start, tims.end));

x=0;
realx=0;
if(tim)
	{
	rem=tim%((TimeType)GLOBALS->nsperframe);
	if(rem)
		{
		tim=tim-GLOBALS->nsperframe-rem;
		x=-GLOBALS->pixelsperframe-((rem*GLOBALS->pixelsperframe)/GLOBALS->nsperframe);
		realx=-GLOBALS->pixelsperframe-((rem*GLOBALS->pixelsperframe)/GLOBALS->nsperframe);
		}
	}

for(;;)
	{
	renderhash(realx, tim);

	if(tim)
		{
		reformat_time(timebuff, time_trunc(tim), GLOBALS->time_dimension);
		}
		else
		{
		strcpy(timebuff, "0");
		}

	lenhalf=font_engine_string_measure(GLOBALS->wavefont, timebuff) >> 1;

	if((x-lenhalf >= lastx) || (GLOBALS->pixelsperframe >= 200))
		{
		font_engine_draw_string(GLOBALS->wavepixmap_wavewindow_c_1,GLOBALS->wavefont,GLOBALS->gc_time_wavewindow_c_1,x-lenhalf, GLOBALS->wavefont->ascent-1,timebuff);

		lastx = x+lenhalf;
		}

	tim+=GLOBALS->nsperframe;
	x+=GLOBALS->pixelsperframe;
	realx+=GLOBALS->pixelsperframe;
	if((prevover)||(tim>GLOBALS->tims.last)) break;
	if(x>=GLOBALS->wavewidth) prevover=1;
	}
}

/***************************************************************************/

static void rendertimebar(void)
{
gdk_draw_rectangle(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_timeb_wavewindow_c_1, TRUE,0, -1, GLOBALS->wavewidth, GLOBALS->fontheight);
rendertimes();
rendertraces();

update_dual();
}

static void rendertraces(void)
{
if(!GLOBALS->topmost_trace)
	{
	GLOBALS->topmost_trace=GLOBALS->traces.first;
	}

if(GLOBALS->topmost_trace)
	{
	Trptr t = GLOBALS->topmost_trace;
	Trptr tback = t;
	hptr h;
	vptr v;
	int i = 0, num_traces_displayable;
	int iback = 0;

	num_traces_displayable=GLOBALS->wavearea->allocation.height/(GLOBALS->fontheight);
	num_traces_displayable--;   /* for the time trace that is always there */

	/* ensure that transaction traces are visible even if the topmost traces are blanks */
	while(tback)
		{
		if(tback->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH))
			{
			tback = GivePrevTrace(tback);			
			iback--;
			}
		else if(tback->flags & TR_TTRANSLATED)
			{
			if(tback != t) 
				{ 
				t = tback;
				i = iback;
				}
			break;
			}
		else
			{
			break;
			}
		}

	for(;((i<num_traces_displayable)&&(t));i++)
		{
		if(!(t->flags&(TR_EXCLUDE|TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
			{
			GLOBALS->shift_timebase=t->shift;
			if(!t->vector)
				{
				h=bsearch_node(t->n.nd, GLOBALS->tims.start - t->shift);
				DEBUG(printf("Start time: "TTFormat", Histent time: "TTFormat"\n", tims.start,(h->time+shift_timebase)));

				if(!t->n.nd->extvals)
					{
					if(i>=0) draw_hptr_trace(t,h,i,1,0);
					}
					else
					{
					if(i>=0) draw_hptr_trace_vector(t,h,i);
					}
				}
				else
				{
				Trptr t_orig, tn;
				bvptr bv = t->n.vec;

				v=bsearch_vector(bv, GLOBALS->tims.start - t->shift);
				DEBUG(printf("Vector Trace: %s, %s\n", t->name, bv->name));
				DEBUG(printf("Start time: "TTFormat", Vectorent time: "TTFormat"\n", tims.start,(v->time+shift_timebase)));
				if(i>=0) draw_vptr_trace(t,v,i);

				if((bv->transaction_chain) && (t->flags & TR_TTRANSLATED))
					{
					t_orig = t;
					for(;;)
						{
						tn = GiveNextTrace(t);
						bv = bv->transaction_chain;

						if(bv && tn && (tn->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
							{
							i++;
							if(i<num_traces_displayable)
								{
								v=bsearch_vector(bv, GLOBALS->tims.start - t->shift);
								if(i>=0) draw_vptr_trace(t_orig,v,i);
								t = tn;
								continue;
								}
							}
						break;
						}
					}
				}
			}
			else
			{
			int kill_dodraw_grid = t->flags & TR_ANALOG_BLANK_STRETCH;

			if(kill_dodraw_grid)
				{
				Trptr tn = GiveNextTrace(t);
				if(!tn) 
					{
					kill_dodraw_grid = 0;
					}
				else
				if(!(tn->flags & TR_ANALOG_BLANK_STRETCH))
					{
					kill_dodraw_grid = 0;
					}
				}
			
			if(i>=0) draw_hptr_trace(NULL,NULL,i,0,kill_dodraw_grid);
			}
		t=GiveNextTrace(t);
/* bot:		1; */
		}
	}


draw_named_markers();
draw_marker_partitions();

if(GLOBALS->traces.dirty)
	{
	char dbuf[32];
	sprintf(dbuf, "%d", GLOBALS->traces.total);
	GLOBALS->traces.dirty = 0;
	gtkwavetcl_setvar(WAVE_TCLCB_TRACES_UPDATED, dbuf, WAVE_TCLCB_TRACES_UPDATED_FLAGS);
	}
}


/*
 * draw single traces and use this for rendering the grid lines
 * for "excluded" traces
 */
static void draw_hptr_trace(Trptr t, hptr h, int which, int dodraw, int kill_grid)
{
TimeType _x0, _x1, newtime;
int _y0, _y1, yu, liney, ytext;
TimeType tim, h2tim;
hptr h2, h3;
char hval, h2val, invert;
GdkGC    *c;
GdkGC    *gcx, *gcxf;
char identifier_str[2];
int is_event = t && t->n.nd && (t->n.nd->vartype == ND_VCD_EVENT);

GLOBALS->tims.start-=GLOBALS->shift_timebase;
GLOBALS->tims.end-=GLOBALS->shift_timebase;

liney=((which+2)*GLOBALS->fontheight)-2;
if(((t)&&(t->flags&TR_INVERT))&&(!is_event))
	{
	_y0=((which+1)*GLOBALS->fontheight)+2;	
	_y1=liney-2;
	invert=1;
	}
	else
	{
	_y1=((which+1)*GLOBALS->fontheight)+2;	
	_y0=liney-2;
	invert=0;
	}
yu=(_y0+_y1)/2;
ytext=yu-(GLOBALS->wavefont->ascent/2)+GLOBALS->wavefont->ascent;


if((GLOBALS->display_grid)&&(GLOBALS->enable_horiz_grid)&&(!kill_grid))
	{
	gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_grid_wavewindow_c_1,(GLOBALS->tims.start<GLOBALS->tims.first)?(GLOBALS->tims.first-GLOBALS->tims.start)*GLOBALS->pxns:0, liney,(GLOBALS->tims.last<=GLOBALS->tims.end)?(GLOBALS->tims.last-GLOBALS->tims.start)*GLOBALS->pxns:GLOBALS->wavewidth-1, liney);
	}

if((h)&&(GLOBALS->tims.start==h->time))
	if (h->v.h_val != AN_Z) {

	switch(h->v.h_val)
		{
		case AN_X:	c = GLOBALS->gc_x_wavewindow_c_1; break;
		case AN_U:	c = GLOBALS->gc_u_wavewindow_c_1; break;
		case AN_W:	c = GLOBALS->gc_w_wavewindow_c_1; break;
		case AN_DASH:	c = GLOBALS->gc_dash_wavewindow_c_1; break;
		default:	c = (h->v.h_val == AN_X) ? GLOBALS->gc_x_wavewindow_c_1: GLOBALS->gc_trans_wavewindow_c_1;
		}
	gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c, 0, _y0, 0, _y1);
	}

if(dodraw)
for(;;)
{
if(!h) break;
tim=(h->time);

if((tim>GLOBALS->tims.end)||(tim>GLOBALS->tims.last)) break;

_x0=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
if(_x0<-1) 
	{ 
	_x0=-1; 
	}
	else
if(_x0>GLOBALS->wavewidth)
	{
	break;
	}

h2=h->next;
if(!h2) break;
h2tim=tim=(h2->time);
if(tim>GLOBALS->tims.last) tim=GLOBALS->tims.last;
	else if(tim>GLOBALS->tims.end+1) tim=GLOBALS->tims.end+1;
_x1=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
if(_x1<-1) 
	{ 
	_x1=-1; 
	}
	else
if(_x1>GLOBALS->wavewidth)
	{
	_x1=GLOBALS->wavewidth;
	}

if(_x0!=_x1)
	{
	if(is_event)
		{
		if(h->time >= GLOBALS->tims.first)
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_w_wavewindow_c_1, _x0, _y0, _x0, _y1); 
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_w_wavewindow_c_1, _x0, _y1, _x0+2, _y1+2); 
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_w_wavewindow_c_1, _x0, _y1, _x0-2, _y1+2); 
			}
		h=h->next;
		continue;
		}

	hval=h->v.h_val;
	h2val=h2->v.h_val;

	switch(h2val)
		{
		case AN_X:	c = GLOBALS->gc_x_wavewindow_c_1; break;
		case AN_U:	c = GLOBALS->gc_u_wavewindow_c_1; break;
		case AN_W:	c = GLOBALS->gc_w_wavewindow_c_1; break;
		case AN_DASH:	c = GLOBALS->gc_dash_wavewindow_c_1; break;
		default:	c = (hval == AN_X) ? GLOBALS->gc_x_wavewindow_c_1: GLOBALS->gc_trans_wavewindow_c_1;
		}

	switch(hval)
		{
		case AN_0:	/* 0 */
		case AN_L:	/* L */
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (hval==AN_0) ? GLOBALS->gc_0_wavewindow_c_1 : GLOBALS->gc_low_wavewindow_c_1,_x0, _y0,_x1, _y0);

		if(h2tim<=GLOBALS->tims.end)
		switch(h2val)
			{
			case AN_0:
			case AN_L:	break;

			case AN_Z:	wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c, _x1, _y0, _x1, yu); break;
			default:	wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c, _x1, _y0, _x1, _y1); break;
			}
		break;
	
		case AN_X: /* X */
		case AN_W: /* W */
		case AN_U: /* U */
		case AN_DASH: /* - */

		identifier_str[1] = 0;
		switch(hval)
			{
			case AN_X:	c = gcx = GLOBALS->gc_x_wavewindow_c_1; gcxf = GLOBALS->gc_xfill_wavewindow_c_1; identifier_str[0] = 0; break;
			case AN_W:	c = gcx = GLOBALS->gc_w_wavewindow_c_1; gcxf = GLOBALS->gc_wfill_wavewindow_c_1; identifier_str[0] = 'W'; break;
			case AN_U:	c = gcx = GLOBALS->gc_u_wavewindow_c_1; gcxf = GLOBALS->gc_ufill_wavewindow_c_1; identifier_str[0] = 'U'; break;
			default:	c = gcx = GLOBALS->gc_dash_wavewindow_c_1; gcxf = GLOBALS->gc_dashfill_wavewindow_c_1; identifier_str[0] = '-'; break;
			}

		if(invert)
			{
			gdk_draw_rectangle(GLOBALS->wavepixmap_wavewindow_c_1, gcx, TRUE,_x0+1, _y0, _x1-_x0, _y1-_y0+1); 
			}
			else
			{
			gdk_draw_rectangle(GLOBALS->wavepixmap_wavewindow_c_1, gcxf, TRUE,_x0+1, _y1, _x1-_x0, _y0-_y1+1); 
			}

		if(identifier_str[0])
			{
			int _x0_new = (_x0>=0) ? _x0 : 0;
			int width;

			if((width=_x1-_x0_new)>GLOBALS->vector_padding)
				{
				if((_x1>=GLOBALS->wavewidth)||(font_engine_string_measure(GLOBALS->wavefont, identifier_str)+GLOBALS->vector_padding<=width))
					{
		                        font_engine_draw_string(GLOBALS->wavepixmap_wavewindow_c_1,GLOBALS->wavefont,  GLOBALS->gc_value_wavewindow_c_1,  _x0+2,ytext,identifier_str);
					}
				}
			}

		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, gcx,_x0, _y0,_x1, _y0);
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, gcx,_x0, _y1,_x1, _y1);
		if(h2tim<=GLOBALS->tims.end) wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c, _x1, _y0, _x1, _y1);
		break;
		
		case AN_Z: /* Z */
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_mid_wavewindow_c_1,_x0, yu,_x1, yu);
		if(h2tim<=GLOBALS->tims.end)
		switch(h2val)
			{
			case AN_0:
			case AN_L: 	
					wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c, _x1, yu, _x1, _y0); break;
			case AN_1:
			case AN_H:
					wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c, _x1, yu, _x1, _y1); break;
			default:	wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c, _x1, _y0, _x1, _y1); break;
			}
		break;
		
		case AN_1: /* 1 */
		case AN_H: /* 1 */
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (hval==AN_1) ? GLOBALS->gc_1_wavewindow_c_1 : GLOBALS->gc_high_wavewindow_c_1,_x0, _y1,_x1, _y1);
		if(h2tim<=GLOBALS->tims.end)
		switch(h2val)
			{
			case AN_1:
			case AN_H:	break;

			case AN_0: 	
			case AN_L:
					wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c, _x1, _y1, _x1, _y0); break;
			case AN_Z:	wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c, _x1, _y1, _x1, yu); break;
			default:	wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c, _x1, _y0, _x1, _y1); break;
			}
		break;

		default:
		break;
		}
	}
	else
	{
	if(!is_event)
		{
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_trans_wavewindow_c_1, _x1, _y0, _x1, _y1);		
		}
		else
		{
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_w_wavewindow_c_1, _x1, _y0, _x1, _y1);		
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_w_wavewindow_c_1, _x0, _y1, _x0+2, _y1+2); 
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_w_wavewindow_c_1, _x0, _y1, _x0-2, _y1+2); 
		}
	newtime=(((gdouble)(_x1+WAVE_OPT_SKIP))*GLOBALS->nspx)+GLOBALS->tims.start+GLOBALS->shift_timebase;	/* skip to next pixel */
	h3=bsearch_node(t->n.nd,newtime);
	if(h3->time>h->time)
		{
		h=h3;
		continue;
		}
	}

h=h->next;
}

wave_gdk_draw_line_flush(GLOBALS->wavepixmap_wavewindow_c_1); /* clear out state */

GLOBALS->tims.start+=GLOBALS->shift_timebase;
GLOBALS->tims.end+=GLOBALS->shift_timebase;
}

/********************************************************************************************************/

static void draw_hptr_trace_vector_analog(Trptr t, hptr h, int which, int num_extension)
{
TimeType _x0, _x1, newtime;
int _y0, _y1, yu, liney, yt0, yt1;
TimeType tim, h2tim;
hptr h2, h3;
int endcnt = 0;
int type;
int lasttype=-1;
GdkGC    *c, *ci;
GdkGC    *cnan = GLOBALS->gc_u_wavewindow_c_1;
GdkGC    *cinf = GLOBALS->gc_w_wavewindow_c_1;
GdkGC    *cfixed;
double mynan = strtod("NaN", NULL);
double tmin = mynan, tmax = mynan, tv, tv2;
gint rmargin;
int is_nan = 0, is_nan2 = 0, is_inf = 0, is_inf2 = 0;
int any_infs = 0, any_infp = 0, any_infm = 0;
int skipcnt = 0;

ci = GLOBALS->gc_baseline_wavewindow_c_1;

liney=((which+2+num_extension)*GLOBALS->fontheight)-2;
_y1=((which+1)*GLOBALS->fontheight)+2;	
_y0=liney-2;
yu=(_y0+_y1)/2;

if(t->flags & TR_ANALOG_FULLSCALE) /* otherwise use dynamic */
	{
	if(!t->minmax_valid)
		{
		h3 = &t->n.nd->head;
		for(;;)
			{
			if(!h3) break;
	
			if((h3->time >= GLOBALS->tims.first) && (h3->time <= GLOBALS->tims.last))
				{
				tv = mynan;
				if(h3->flags&HIST_REAL)
					{
					if(!(h3->flags&HIST_STRING) && h3->v.h_vector)
						tv = *(double *)h3->v.h_vector;
					}
					else
					{
					if(h3->time <= GLOBALS->tims.last) tv=convert_real_vec(t,h3->v.h_vector);
					}


				if (!isnan(tv) && !isinf(tv))
					{
					if (isnan(tmin) || tv < tmin)
						tmin = tv;
					if (isnan(tmax) || tv > tmax)
						tmax = tv;
					}
				else
				if(isinf(tv))
					{
					any_infs = 1;

					if(tv > 0)
						{
						any_infp = 1;
						}
						else
						{
						any_infm = 1;
						}
					}
				}
			h3 = h3->next;
			}
	
		if (isnan(tmin) || isnan(tmax))
			{
			tmin = tmax = 0;
			}

		if(any_infs)
			{
			double tdelta = (tmax - tmin) * WAVE_INF_SCALING;

			if(any_infp) tmax = tmax + tdelta;
			if(any_infm) tmin = tmin - tdelta;
			}

		if ((tmax - tmin) < 1e-20)
			{
			tmax = 1;
			tmin -= 0.5 * (_y1 - _y0);
			}
			else
			{
			tmax = (_y1 - _y0) / (tmax - tmin);
			}

		t->minmax_valid = 1;
		t->d_minval = tmin;
		t->d_maxval = tmax;
		}
		else
		{
		tmin = t->d_minval;
		tmax = t->d_maxval;
		}
	}
	else
	{
	h3 = h;	
	for(;;)
	{
	if(!h3) break;
	tim=(h3->time);
	if(tim>GLOBALS->tims.end) { endcnt++; if(endcnt==2) break; }
	if(tim>GLOBALS->tims.last) break;

	_x0=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
	if((_x0>GLOBALS->wavewidth)&&(endcnt==2))
		{
		break;
		}

	tv = mynan;
	if(h3->flags&HIST_REAL)
		{
		if(!(h3->flags&HIST_STRING) && h3->v.h_vector)
			tv = *(double *)h3->v.h_vector;
		}
		else
		{
		if(h3->time <= GLOBALS->tims.last) tv=convert_real_vec(t,h3->v.h_vector);
		}

	if (!isnan(tv) & !isinf(tv))
		{
		if (isnan(tmin) || tv < tmin)
			tmin = tv;
		if (isnan(tmax) || tv > tmax)
			tmax = tv;
		}
	else
	if(isinf(tv))
		{
		any_infs = 1;
		if(tv > 0)
			{
			any_infp = 1;
			}
			else
			{
			any_infm = 1;
			}
		}

	h3 = h3->next;
	}

	if (isnan(tmin) || isnan(tmax))
		tmin = tmax = 0;

	if(any_infs)
		{
		double tdelta = (tmax - tmin) * WAVE_INF_SCALING;

		if(any_infp) tmax = tmax + tdelta;
		if(any_infm) tmin = tmin - tdelta;
		}

	if ((tmax - tmin) < 1e-20)
		{
		tmax = 1;
		tmin -= 0.5 * (_y1 - _y0);
		}
		else
		{
		tmax = (_y1 - _y0) / (tmax - tmin);
		}
	}

if(GLOBALS->tims.last - GLOBALS->tims.start < GLOBALS->wavewidth)
	{
	rmargin=(GLOBALS->tims.last - GLOBALS->tims.start) * GLOBALS->pxns;
	}
	else
	{
	rmargin = GLOBALS->wavewidth;
	}

/* now do the actual drawing */
h3 = NULL;
for(;;)
{
if(!h) break;
tim=(h->time);
if((tim>GLOBALS->tims.end)||(tim>GLOBALS->tims.last)) break;

_x0=(tim - GLOBALS->tims.start) * GLOBALS->pxns;

/*
if(_x0<-1) 
	{ 
	_x0=-1; 
	}
	else
if(_x0>GLOBALS->wavewidth)
	{
	break;
	}
*/

h2=h->next;
if(!h2) break;
h2tim=tim=(h2->time);
if(tim>GLOBALS->tims.last) tim=GLOBALS->tims.last;
/*	else if(tim>GLOBALS->tims.end+1) tim=GLOBALS->tims.end+1; */
_x1=(tim - GLOBALS->tims.start) * GLOBALS->pxns;

/*
if(_x1<-1) 
	{ 
	_x1=-1; 
	}
	else
if(_x1>GLOBALS->wavewidth)
	{
	_x1=GLOBALS->wavewidth;
	}
*/

/* draw trans */
type = (!(h->flags&(HIST_REAL|HIST_STRING))) ? vtype(t,h->v.h_vector) : AN_0;
tv = tv2 = mynan;

if(h->flags&HIST_REAL)
	{
	if(!(h->flags&HIST_STRING) && h->v.h_vector)
		tv = *(double *)h->v.h_vector;
	}
	else
	{
	if(h->time <= GLOBALS->tims.last) tv=convert_real_vec(t,h->v.h_vector);
	}

if(h2->flags&HIST_REAL)
	{
	if(!(h2->flags&HIST_STRING) && h2->v.h_vector)
		tv2 = *(double *)h2->v.h_vector;
	}
	else
	{
	if(h2->time <= GLOBALS->tims.last) tv2=convert_real_vec(t,h2->v.h_vector);
	}

if((is_inf = isinf(tv)))
	{
	if(tv < 0)
		{
		yt0 = _y0;
		}
		else
		{
		yt0 = _y1;
		}
	}
else
if((is_nan = isnan(tv)))
	{
	yt0 = yu;
	}
	else
	{
	yt0 = _y0 + (tv - tmin) * tmax;
	}

if((is_inf2 = isinf(tv2)))
	{
	if(tv2 < 0)
		{
		yt1 = _y0;
		}
		else
		{
		yt1 = _y1;
		}
	}
else
if((is_nan2 = isnan(tv2)))
	{
	yt1 = yu;
	}
	else
	{
	yt1 = _y0 + (tv2 - tmin) * tmax;
	}

if((_x0!=_x1)||(skipcnt < GLOBALS->analog_redraw_skip_count)) /* lower number = better performance */
	{
	if(_x0==_x1) 
		{
		skipcnt++;
		}
		else
		{
		skipcnt = 0;
		}

	if(type == AN_0) 
		{
		c = GLOBALS->gc_vbox_wavewindow_c_1; 
		} 
		else 
		{
		c = GLOBALS->gc_x_wavewindow_c_1;
		}

	if(h->next)
		{
		if(h->next->time > GLOBALS->max_time)
			{
			yt1 = yt0;
			}
		}

	cfixed = is_inf ? cinf : c;
	if((is_nan2) && (h2tim > GLOBALS->max_time)) is_nan2 = 0;

/* clamp to top/bottom because of integer rounding errors */

if(yt0 < _y1) yt0 = _y1;
else if(yt0 > _y0) yt0 = _y0;

if(yt1 < _y1) yt1 = _y1;
else if(yt1 > _y0) yt1 = _y0;

/* clipping... */
{
int coords[4];
int rect[4];

coords[0] = _x0;
coords[1] = yt0;
coords[2] = _x1;
coords[3] = yt1;

rect[0] = -10;
rect[1] = _y1;
rect[2] = GLOBALS->wavewidth + 10;
rect[3] = _y0;

wave_lineclip(coords, rect);
_x0 = coords[0];
yt0 = coords[1];
_x1 = coords[2];
yt1 = coords[3];
}
/* ...clipping */

	if(is_nan || is_nan2)
		{
		if(is_nan)
			{
			gdk_draw_rectangle(GLOBALS->wavepixmap_wavewindow_c_1, cnan, TRUE, _x0, _y1, _x1-_x0, _y0-_y1);

			if((t->flags & (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP)) == (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP))
				{
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x1-1, yt1,_x1+1, yt1);
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x1, yt1-1,_x1, yt1+1);

				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0-1, _y0,_x0+1, _y0);
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0, _y0-1,_x0, _y0+1);

				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0-1, _y1,_x0+1, _y1);
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0, _y1-1,_x0, _y1+1);
				}
			}
		if(is_nan2)
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, cfixed,_x0, yt0, _x1, yt0);

			if((t->flags & (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP)) == (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP))
				{
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, cnan,_x1, yt1,_x1, yt0);

				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x1-1, _y0,_x1+1, _y0);
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x1, _y0-1,_x1, _y0+1);

				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x1-1, _y1,_x1+1, _y1);
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x1, _y1-1,_x1, _y1+1);
				}
			}
		}
	else
	if((t->flags & TR_ANALOG_INTERPOLATED) && !is_inf && !is_inf2)
		{
		if(t->flags & TR_ANALOG_STEP)
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0-1, yt0,_x0+1, yt0);
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0, yt0-1,_x0, yt0+1);
			}

		if(rmargin != GLOBALS->wavewidth)	/* the window is clipped in postscript */
			{
			if((yt0==yt1)&&((_x0 > _x1)||(_x0 < 0)))
				{
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, cfixed,0, yt0,_x1,yt1);
				}
				else
				{
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, cfixed,_x0, yt0,_x1,yt1);
				}
			}
			else
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, cfixed,_x0, yt0,_x1,yt1);
			}
		}
	else
	/* if(t->flags & TR_ANALOG_STEP) */
		{
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, cfixed,_x0, yt0,_x1, yt0);

		if(is_inf2) cfixed = cinf;
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, cfixed,_x1, yt0,_x1, yt1);

		if ((t->flags & (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP)) == (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP))
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0-1, yt0,_x0+1, yt0);
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0, yt0-1,_x0, yt0+1);
			}
		}
	}
	else
	{
	newtime=(((gdouble)(_x1+WAVE_OPT_SKIP))*GLOBALS->nspx)+GLOBALS->tims.start+GLOBALS->shift_timebase;	/* skip to next pixel */
	h3=bsearch_node(t->n.nd,newtime);
	if(h3->time>h->time)
		{
		h=h3;
		lasttype=type;
		continue;
		}
	}

h=h->next;
lasttype=type;
}

wave_gdk_draw_line_flush(GLOBALS->wavepixmap_wavewindow_c_1);
}

/*
 * draw hptr vectors (integer+real)
 */
static void draw_hptr_trace_vector(Trptr t, hptr h, int which)
{
TimeType _x0, _x1, newtime, width;
int _y0, _y1, yu, liney, ytext;
TimeType tim, h2tim;
hptr h2, h3;
char *ascii=NULL;
int type;
int lasttype=-1;
GdkGC    *c;

GLOBALS->tims.start-=GLOBALS->shift_timebase;
GLOBALS->tims.end-=GLOBALS->shift_timebase;

liney=((which+2)*GLOBALS->fontheight)-2;
_y1=((which+1)*GLOBALS->fontheight)+2;	
_y0=liney-2;
yu=(_y0+_y1)/2;
ytext=yu-(GLOBALS->wavefont->ascent/2)+GLOBALS->wavefont->ascent;

if((GLOBALS->display_grid)&&(GLOBALS->enable_horiz_grid))
	{
	Trptr tn = GiveNextTrace(t);
	if((t->flags & TR_ANALOGMASK) && (tn) && (tn->flags & TR_ANALOG_BLANK_STRETCH))
		{
		}
		else
		{
		gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_grid_wavewindow_c_1,(GLOBALS->tims.start<GLOBALS->tims.first)?(GLOBALS->tims.first-GLOBALS->tims.start)*GLOBALS->pxns:0, liney,(GLOBALS->tims.last<=GLOBALS->tims.end)?(GLOBALS->tims.last-GLOBALS->tims.start)*GLOBALS->pxns:GLOBALS->wavewidth-1, liney);
		}
	}

if((t->flags & TR_ANALOGMASK) && (!(h->flags&HIST_STRING) || !(h->flags&HIST_REAL)))
	{
	Trptr te = GiveNextTrace(t);
	int ext = 0;

	while(te)
		{
		if(te->flags & TR_ANALOG_BLANK_STRETCH)
			{
			ext++;
			te = GiveNextTrace(te);
			}
			else
			{
			break;
			}
		}	

	draw_hptr_trace_vector_analog(t, h, which, ext);
	GLOBALS->tims.start+=GLOBALS->shift_timebase;
	GLOBALS->tims.end+=GLOBALS->shift_timebase;
	return;
	}

GLOBALS->color_active_in_filter = 1;

for(;;)
{
if(!h) break;
tim=(h->time);
if((tim>GLOBALS->tims.end)||(tim>GLOBALS->tims.last)) break;

_x0=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
if(_x0<-1) 
	{ 
	_x0=-1; 
	}
	else
if(_x0>GLOBALS->wavewidth)
	{
	break;
	}

h2=h->next;
if(!h2) break;
h2tim=tim=(h2->time);
if(tim>GLOBALS->tims.last) tim=GLOBALS->tims.last;
	else if(tim>GLOBALS->tims.end+1) tim=GLOBALS->tims.end+1;
_x1=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
if(_x1<-1) 
	{ 
	_x1=-1; 
	}
	else
if(_x1>GLOBALS->wavewidth)
	{
	_x1=GLOBALS->wavewidth;
	}

/* draw trans */
type = (!(h->flags&(HIST_REAL|HIST_STRING))) ? vtype(t,h->v.h_vector) : AN_0;
if(_x0>-1) {
if(GLOBALS->use_roundcaps)
	{
	if (type == AN_Z) {
		if (lasttype != -1) {
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (lasttype==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0-1, _y0,_x0,   yu);
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (lasttype==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0, yu,_x0-1, _y1);
		}
	} else
	if (lasttype==AN_Z) {
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (type==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0+1, _y0,_x0,   yu);
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (type==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0, yu,_x0+1, _y1);
	} else {
		if (lasttype != type) {
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (lasttype==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0-1, _y0,_x0,   yu);
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (lasttype==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0, yu,_x0-1, _y1);
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (type==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0+1, _y0,_x0,   yu);
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (type==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0, yu,_x0+1, _y1);
		} else {
	wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (type==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0-2, _y0,_x0+2, _y1);
	wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (type==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0+2, _y0,_x0-2, _y1);
		}
	}
	}
	else
	{
	wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (type==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0, _y0,_x0, _y1);
	}
}
		
if(_x0!=_x1)
	{
	if (type == AN_Z) 
		{
		if(GLOBALS->use_roundcaps)
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_mid_wavewindow_c_1,_x0+1, yu,_x1-1, yu);
			} 
			else 
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_mid_wavewindow_c_1,_x0, yu,_x1, yu);
			}
		} 
		else 
		{
		if(type == AN_0) 
			{
			c = GLOBALS->gc_vbox_wavewindow_c_1; 
			} 
			else 
			{
			c = GLOBALS->gc_x_wavewindow_c_1;
			}
	
	if(GLOBALS->use_roundcaps)
		{
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c,_x0+2, _y0,_x1-2, _y0);
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c,_x0+2, _y1,_x1-2, _y1);
		}
		else
		{
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c,_x0, _y0,_x1, _y0);
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c,_x0, _y1,_x1, _y1);
		}

if(_x0<0) _x0=0;	/* fixup left margin */

	if((width=_x1-_x0)>GLOBALS->vector_padding)
		{
		char *ascii2;

		if(h->flags&HIST_REAL)
			{
			if(!(h->flags&HIST_STRING))
				{
				ascii=convert_ascii_real(t, (double *)h->v.h_vector);
				}
				else
				{
				ascii=convert_ascii_string((char *)h->v.h_vector);
				}
			}
			else
			{
			ascii=convert_ascii_vec(t,h->v.h_vector);
			}

		ascii2 = ascii;
		if(*ascii == '?')
			{
			GdkGC *cb;
			char *srch_for_color = strchr(ascii+1, '?');
			if(srch_for_color)
				{
				*srch_for_color = 0;
				cb = get_gc_from_name(ascii+1);
				if(cb)
					{	
					ascii2 =  srch_for_color + 1;
					if(GLOBALS->gc_back_wavewindow_c_1 != GLOBALS->gc_white)
						{
						if(!GLOBALS->black_and_white) gdk_draw_rectangle(GLOBALS->wavepixmap_wavewindow_c_1, cb, TRUE, _x0+1, _y1+1, width-1, (_y0-1) - (_y1+1) + 1);
						}
					GLOBALS->fill_in_smaller_rgb_areas_wavewindow_c_1 = 1;
					}
					else
					{
					*srch_for_color = '?'; /* replace name as color is a miss */
					}
				}
			}

		if((_x1>=GLOBALS->wavewidth)||(font_engine_string_measure(GLOBALS->wavefont, ascii2)+GLOBALS->vector_padding<=width))
			{
			font_engine_draw_string(GLOBALS->wavepixmap_wavewindow_c_1,GLOBALS->wavefont,GLOBALS->gc_value_wavewindow_c_1,_x0+2,ytext,ascii2);
			}
		else
			{
			char *mod;

			mod=bsearch_trunc(ascii2,width-GLOBALS->vector_padding);
			if(mod)
				{
				*mod='+';
				*(mod+1)=0;

				font_engine_draw_string(GLOBALS->wavepixmap_wavewindow_c_1,GLOBALS->wavefont,GLOBALS->gc_value_wavewindow_c_1,_x0+2,ytext,ascii2);
				}
			}
		}
		else if(GLOBALS->fill_in_smaller_rgb_areas_wavewindow_c_1)
		{
		char *ascii2;

		if(h->flags&HIST_REAL)
			{
			if(!(h->flags&HIST_STRING))
				{
				ascii=convert_ascii_real(t, (double *)h->v.h_vector);
				}
				else
				{
				ascii=convert_ascii_string((char *)h->v.h_vector);
				}
			}
			else
			{
			ascii=convert_ascii_vec(t,h->v.h_vector);
			}

		ascii2 = ascii;
		if(*ascii == '?')
			{
			GdkGC *cb;
			char *srch_for_color = strchr(ascii+1, '?');
			if(srch_for_color)
				{
				*srch_for_color = 0;
				cb = get_gc_from_name(ascii+1);
				if(cb)
					{	
					ascii2 =  srch_for_color + 1;
					if(GLOBALS->gc_back_wavewindow_c_1 != GLOBALS->gc_white)
						{
						if(!GLOBALS->black_and_white) gdk_draw_rectangle(GLOBALS->wavepixmap_wavewindow_c_1, cb, TRUE, _x0, _y1+1, width, (_y0-1) - (_y1+1) + 1);
						}
					}
					else
					{
					*srch_for_color = '?'; /* replace name as color is a miss */
					}
				}
			}

		}
	    }
	}
	else
	{
	newtime=(((gdouble)(_x1+WAVE_OPT_SKIP))*GLOBALS->nspx)+GLOBALS->tims.start+GLOBALS->shift_timebase;	/* skip to next pixel */
	h3=bsearch_node(t->n.nd,newtime);
	if(h3->time>h->time)
		{
		h=h3;
		lasttype=type;
		continue;
		}
	}

if(ascii) { free_2(ascii); ascii=NULL; }
h=h->next;
lasttype=type;
}

GLOBALS->color_active_in_filter = 0;

wave_gdk_draw_line_flush(GLOBALS->wavepixmap_wavewindow_c_1);

GLOBALS->tims.start+=GLOBALS->shift_timebase;
GLOBALS->tims.end+=GLOBALS->shift_timebase;
}

/********************************************************************************************************/

static void draw_vptr_trace_analog(Trptr t, vptr v, int which, int num_extension)
{
TimeType _x0, _x1, newtime;
int _y0, _y1, yu, liney, yt0, yt1;
TimeType tim, h2tim;
vptr h, h2, h3;
int endcnt = 0;
int type;
int lasttype=-1;
GdkGC    *c, *ci;
GdkGC    *cnan = GLOBALS->gc_u_wavewindow_c_1;
GdkGC    *cinf = GLOBALS->gc_w_wavewindow_c_1;
GdkGC    *cfixed;
double mynan = strtod("NaN", NULL);
double tmin = mynan, tmax = mynan, tv=0.0, tv2;
gint rmargin;
int is_nan = 0, is_nan2 = 0, is_inf = 0, is_inf2 = 0;
int any_infs = 0, any_infp = 0, any_infm = 0;

ci = GLOBALS->gc_baseline_wavewindow_c_1;

h=v;
liney=((which+2+num_extension)*GLOBALS->fontheight)-2;
_y1=((which+1)*GLOBALS->fontheight)+2;	
_y0=liney-2;
yu=(_y0+_y1)/2;

if(t->flags & TR_ANALOG_FULLSCALE) /* otherwise use dynamic */
        {
        if(!t->minmax_valid)
                { 
                h3 = t->n.vec->vectors[0];
                for(;;)
                        {
                        if(!h3) break;

                        if((h3->time >= GLOBALS->tims.first) && (h3->time <= GLOBALS->tims.last))
                                {
                                tv = mynan;

				tv=convert_real(t,h3);
				if (!isnan(tv) && !isinf(tv))
					{
					if (isnan(tmin) || tv < tmin)
						tmin = tv;
					if (isnan(tmax) || tv > tmax)
						tmax = tv;
					}
				}
                                else
                                if(isinf(tv))
                                        {
                                        any_infs = 1;
                                 
                                        if(tv > 0)
                                                {
                                                any_infp = 1;
                                                }
                                                else
                                                {
                                                any_infm = 1;
                                                }
                                        }

			h3 = h3->next;
			}

		if (isnan(tmin) || isnan(tmax))
		tmin = tmax = 0;

                if(any_infs)
                        {
			double tdelta = (tmax - tmin) * WAVE_INF_SCALING;
                                         
                        if(any_infp) tmax = tmax + tdelta;
                        if(any_infm) tmin = tmin - tdelta;
                        }

		if ((tmax - tmin) < 1e-20)
			{
			tmax = 1;
			tmin -= 0.5 * (_y1 - _y0);
			}
			else
			{
			tmax = (_y1 - _y0) / (tmax - tmin);
			}
                               
                t->minmax_valid = 1;
                t->d_minval = tmin;
                t->d_maxval = tmax;
                }
                else
                {
                tmin = t->d_minval;
                tmax = t->d_maxval;
                }
	}
	else
	{
	h3 = h;
	for(;;)
	{
	if(!h3) break;
	tim=(h3->time);
	
	if(tim>GLOBALS->tims.end) { endcnt++; if(endcnt==2) break; }
	if(tim>GLOBALS->tims.last) break;
	
	_x0=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
	if((_x0>GLOBALS->wavewidth)&&(endcnt==2))
	        {
	        break;
	        }

	tv=convert_real(t,h3);
	if (!isnan(tv) && !isinf(tv))
		{
		if (isnan(tmin) || tv < tmin)
			tmin = tv;
		if (isnan(tmax) || tv > tmax)
			tmax = tv;
		}
        else
        if(isinf(tv))    
                {
                any_infs = 1;
                if(tv > 0)
                        {
                        any_infp = 1;
                        }
                        else
                        {
                        any_infm = 1;
                        }                
                }   

	h3 = h3->next;
	}
	if (isnan(tmin) || isnan(tmax))
		tmin = tmax = 0;

        if(any_infs)
                {
		double tdelta = (tmax - tmin) * WAVE_INF_SCALING;
                 
                if(any_infp) tmax = tmax + tdelta;
                if(any_infm) tmin = tmin - tdelta;
                }

	if ((tmax - tmin) < 1e-20)
		{
		tmax = 1;
		tmin -= 0.5 * (_y1 - _y0);
		}
		else
		{
		tmax = (_y1 - _y0) / (tmax - tmin);
		}
	}
	
if(GLOBALS->tims.last - GLOBALS->tims.start < GLOBALS->wavewidth)
	{
	rmargin=(GLOBALS->tims.last - GLOBALS->tims.start) * GLOBALS->pxns;
	}
	else
	{
	rmargin = GLOBALS->wavewidth;
	}

h3 = NULL;
for(;;)
{
if(!h) break;
tim=(h->time);
if((tim>GLOBALS->tims.end)||(tim>GLOBALS->tims.last)) break;

_x0=(tim - GLOBALS->tims.start) * GLOBALS->pxns;

/*
if(_x0<-1) 
	{ 
	_x0=-1; 
	}
	else
if(_x0>GLOBALS->wavewidth)
	{
	break;
	}
*/

h2=h->next;
if(!h2) break;
h2tim=tim=(h2->time);
if(tim>GLOBALS->tims.last) tim=GLOBALS->tims.last;
/*	else if(tim>GLOBALS->tims.end+1) tim=GLOBALS->tims.end+1; */
_x1=(tim - GLOBALS->tims.start) * GLOBALS->pxns;

/*
if(_x1<-1) 
	{ 
	_x1=-1; 
	}
	else
if(_x1>GLOBALS->wavewidth)
	{
	_x1=GLOBALS->wavewidth;
	}
*/
/* draw trans */
type = vtype2(t,h);
tv=convert_real(t,h);
tv2=convert_real(t,h2);

if((is_inf = isinf(tv)))
	{
	if(tv < 0)
		{
		yt0 = _y0;
		}
		else
		{
		yt0 = _y1;
		}
	}
else
if((is_nan = isnan(tv)))
	{
	yt0 = yu;
	}
	else
	{
	yt0 = _y0 + (tv - tmin) * tmax;
	}

if((is_inf2 = isinf(tv2)))
	{
	if(tv2 < 0)
		{
		yt1 = _y0;
		}
		else
		{
		yt1 = _y1;
		}
	}
else
if((is_nan2 = isnan(tv2)))
	{
	yt1 = yu;
	}
	else
	{
	yt1 = _y0 + (tv2 - tmin) * tmax;
	}

if(_x0!=_x1)
	{
	if(type == AN_0) 
		{
		c = GLOBALS->gc_vbox_wavewindow_c_1; 
		} 
		else 
		{
		c = GLOBALS->gc_x_wavewindow_c_1;
		}

	if(h->next)
		{
		if(h->next->time > GLOBALS->max_time)
			{
			yt1 = yt0;
			}
		}

	cfixed = is_inf ? cinf : c;
	if((is_nan2) && (h2tim > GLOBALS->max_time)) is_nan2 = 0;

        if(is_nan || is_nan2)
                {
		if(is_nan)
			{
			gdk_draw_rectangle(GLOBALS->wavepixmap_wavewindow_c_1, cnan, TRUE, _x0, _y1, _x1-_x0, _y0-_y1);

			if((t->flags & (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP)) == (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP))
				{
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x1-1, yt1,_x1+1, yt1);
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x1, yt1-1,_x1, yt1+1);

				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0-1, _y0,_x0+1, _y0);
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0, _y0-1,_x0, _y0+1);

				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0-1, _y1,_x0+1, _y1);
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0, _y1-1,_x0, _y1+1);
				}
			}
		if(is_nan2)
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, cfixed,_x0, yt0, _x1, yt0);

			if((t->flags & (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP)) == (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP))
				{
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, cnan,_x1, yt1,_x1, yt0);

				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x1-1, _y0,_x1+1, _y0);
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x1, _y0-1,_x1, _y0+1);

				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x1-1, _y1,_x1+1, _y1);
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x1, _y1-1,_x1, _y1+1);
				}
			}
                }
        else
	if((t->flags & TR_ANALOG_INTERPOLATED) && !is_inf && !is_inf2)
		{
		if(t->flags & TR_ANALOG_STEP)
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0-1, yt0,_x0+1, yt0);
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0, yt0-1,_x0, yt0+1);
			}

		if(rmargin != GLOBALS->wavewidth)	/* the window is clipped in postscript */
			{
			if((yt0==yt1)&&((_x0 > _x1)||(_x0 < 0)))
				{
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, cfixed,0, yt0,_x1,yt1);
				}
				else
				{
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, cfixed,_x0, yt0,_x1,yt1);
				}
			}
			else
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, cfixed,_x0, yt0,_x1,yt1);
			}
		}
	else
	/* if(t->flags & TR_ANALOG_STEP) */
		{
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, cfixed,_x0, yt0,_x1, yt0);

		if(is_inf2) cfixed = cinf;
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, cfixed,_x1, yt0,_x1, yt1);

		if ((t->flags & (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP)) == (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP))
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0-1, yt0,_x0+1, yt0);
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, ci,_x0, yt0-1,_x0, yt0+1);
			}
		}
	}
	else
	{
	newtime=(((gdouble)(_x1+WAVE_OPT_SKIP))*GLOBALS->nspx)+GLOBALS->tims.start+GLOBALS->shift_timebase;	/* skip to next pixel */
	h3=bsearch_vector(t->n.vec,newtime);
	if(h3->time>h->time)
		{
		h=h3;
		lasttype=type;
		continue;
		}
	}

h=h->next;
lasttype=type;
}

wave_gdk_draw_line_flush(GLOBALS->wavepixmap_wavewindow_c_1);

GLOBALS->tims.start+=GLOBALS->shift_timebase;
GLOBALS->tims.end+=GLOBALS->shift_timebase;
}

/*
 * draw vector traces
 */
static void draw_vptr_trace(Trptr t, vptr v, int which)
{
TimeType _x0, _x1, newtime, width;
int _y0, _y1, yu, liney, ytext;
TimeType tim, h2tim;
vptr h, h2, h3;
char *ascii=NULL;
int type;
int lasttype=-1;
GdkGC    *c;

GLOBALS->tims.start-=GLOBALS->shift_timebase;
GLOBALS->tims.end-=GLOBALS->shift_timebase;

liney=((which+2)*GLOBALS->fontheight)-2;
_y1=((which+1)*GLOBALS->fontheight)+2;	
_y0=liney-2;
yu=(_y0+_y1)/2;
ytext=yu-(GLOBALS->wavefont->ascent/2)+GLOBALS->wavefont->ascent;

if((GLOBALS->display_grid)&&(GLOBALS->enable_horiz_grid))
	{
	Trptr tn = GiveNextTrace(t);
	if((t->flags & TR_ANALOGMASK) && (tn) && (tn->flags & TR_ANALOG_BLANK_STRETCH))
		{
		}
		else
		{
		gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_grid_wavewindow_c_1,(GLOBALS->tims.start<GLOBALS->tims.first)?(GLOBALS->tims.first-GLOBALS->tims.start)*GLOBALS->pxns:0, liney,(GLOBALS->tims.last<=GLOBALS->tims.end)?(GLOBALS->tims.last-GLOBALS->tims.start)*GLOBALS->pxns:GLOBALS->wavewidth-1, liney);
		}
	}

h = v;
/*
if(t->flags & TR_TTRANSLATED)
	{
	traverse_vector_nodes(t);
	h=v=bsearch_vector(t->n.vec, GLOBALS->tims.start);
	}
	else
	{
	h=v;
	}
*/

if(t->flags & TR_ANALOGMASK)
	{
        Trptr te = GiveNextTrace(t);
        int ext = 0;

        while(te)
                {
                if(te->flags & TR_ANALOG_BLANK_STRETCH)
                        {
                        ext++;
                        te = GiveNextTrace(te);
                        }
		else
                        {
                        break;
                        }
                }

	draw_vptr_trace_analog(t, v, which, ext);

	GLOBALS->tims.start+=GLOBALS->shift_timebase;
	GLOBALS->tims.end+=GLOBALS->shift_timebase;
	return;
	}

GLOBALS->color_active_in_filter = 1;

for(;;)
{
if(!h) break;
tim=(h->time);
if((tim>GLOBALS->tims.end)||(tim>GLOBALS->tims.last)) break;

_x0=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
if(_x0<-1) 
	{ 
	_x0=-1; 
	}
	else
if(_x0>GLOBALS->wavewidth)
	{
	break;
	}

h2=h->next;
if(!h2) break;
h2tim=tim=(h2->time);
if(tim>GLOBALS->tims.last) tim=GLOBALS->tims.last;
	else if(tim>GLOBALS->tims.end+1) tim=GLOBALS->tims.end+1;
_x1=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
if(_x1<-1) 
	{ 
	_x1=-1; 
	}
	else
if(_x1>GLOBALS->wavewidth)
	{
	_x1=GLOBALS->wavewidth;
	}

/* draw trans */
type = vtype2(t,h);

if(_x0>-1) {
if(GLOBALS->use_roundcaps)
	{
	if (type == AN_Z) 
		{
		if (lasttype != -1) 
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (lasttype==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0-1, _y0,_x0,   yu);
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (lasttype==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0, yu,_x0-1, _y1);
			}
		} 
		else
		if (lasttype==AN_Z) 
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (type==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0+1, _y0,_x0,   yu);
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (type==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0, yu,_x0+1, _y1);
			} 
			else 
			{
			if (lasttype != type) 
				{
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (lasttype==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0-1, _y0,_x0,   yu);
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (lasttype==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0, yu,_x0-1, _y1);
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (type==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0+1, _y0,_x0,   yu);
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (type==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0, yu,_x0+1, _y1);
				} 
				else 
				{
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (type==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0-2, _y0,_x0+2, _y1);
				wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (type==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0+2, _y0,_x0-2, _y1);
				}
			}
		}
		else
		{
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, (type==AN_X? GLOBALS->gc_x_wavewindow_c_1:GLOBALS->gc_vtrans_wavewindow_c_1),_x0, _y0,_x0, _y1);
		}
}

if(_x0!=_x1)
	{
	if (type == AN_Z) 
		{
		if(GLOBALS->use_roundcaps)
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_mid_wavewindow_c_1,_x0+1, yu,_x1-1, yu);
			} 
			else 
			{
			wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, GLOBALS->gc_mid_wavewindow_c_1,_x0, yu,_x1, yu);
			}
		} 
		else 
		{
		if(type == AN_0) 
			{
			c = GLOBALS->gc_vbox_wavewindow_c_1; 
			} 
			else 
			{
			c = GLOBALS->gc_x_wavewindow_c_1;
			}
	
	if(GLOBALS->use_roundcaps)
		{
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c,_x0+2, _y0,_x1-2, _y0);
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c,_x0+2, _y1,_x1-2, _y1);
		}
		else
		{
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c,_x0, _y0,_x1, _y0);
		wave_gdk_draw_line(GLOBALS->wavepixmap_wavewindow_c_1, c,_x0, _y1,_x1, _y1);
		}


if(_x0<0) _x0=0;	/* fixup left margin */

	if((width=_x1-_x0)>GLOBALS->vector_padding)
		{
		char *ascii2;

		ascii=convert_ascii(t,h);

		ascii2 = ascii;
		if(*ascii == '?')
			{
			GdkGC *cb;
			char *srch_for_color = strchr(ascii+1, '?');
			if(srch_for_color)
				{
				*srch_for_color = 0;
				cb = get_gc_from_name(ascii+1);
				if(cb)
					{	
					ascii2 =  srch_for_color + 1;
					if(!GLOBALS->black_and_white) gdk_draw_rectangle(GLOBALS->wavepixmap_wavewindow_c_1, cb, TRUE, _x0+1, _y1+1, width-1, (_y0-1) - (_y1+1) + 1);
					GLOBALS->fill_in_smaller_rgb_areas_wavewindow_c_1 = 1;
					}
					else
					{
					*srch_for_color = '?'; /* replace name as color is a miss */
					}
				}
			}

		if((_x1>=GLOBALS->wavewidth)||(font_engine_string_measure(GLOBALS->wavefont, ascii2)+GLOBALS->vector_padding<=width))
			{
			font_engine_draw_string(GLOBALS->wavepixmap_wavewindow_c_1,GLOBALS->wavefont,GLOBALS->gc_value_wavewindow_c_1,_x0+2,ytext,ascii2);
			}
		else
			{
			char *mod;

			mod=bsearch_trunc(ascii2,width-GLOBALS->vector_padding);
			if(mod)
				{
				*mod='+';
				*(mod+1)=0;

				font_engine_draw_string(GLOBALS->wavepixmap_wavewindow_c_1,GLOBALS->wavefont,GLOBALS->gc_value_wavewindow_c_1,_x0+2,ytext,ascii2);
				}
			}

		}
		else if(GLOBALS->fill_in_smaller_rgb_areas_wavewindow_c_1)
		{
		char *ascii2;

		ascii=convert_ascii(t,h);

		ascii2 = ascii;
		if(*ascii == '?')
			{
			GdkGC *cb;
			char *srch_for_color = strchr(ascii+1, '?');
			if(srch_for_color)
				{
				*srch_for_color = 0;
				cb = get_gc_from_name(ascii+1);
				if(cb)
					{	
					ascii2 =  srch_for_color + 1;
					if(GLOBALS->gc_back_wavewindow_c_1 != GLOBALS->gc_white)
						{
						if(!GLOBALS->black_and_white) gdk_draw_rectangle(GLOBALS->wavepixmap_wavewindow_c_1, cb, TRUE, _x0, _y1+1, width, (_y0-1) - (_y1+1) + 1);
						}
					}
					else
					{
					*srch_for_color = '?'; /* replace name as color is a miss */
					}
				}
			}
		}
	}
	}
	else
	{
	newtime=(((gdouble)(_x1+WAVE_OPT_SKIP))*GLOBALS->nspx)+GLOBALS->tims.start+GLOBALS->shift_timebase;	/* skip to next pixel */
	h3=bsearch_vector(t->n.vec,newtime);
	if(h3->time>h->time)
		{
		h=h3;
		lasttype=type;
		continue;
		}
	}

if(ascii) { free_2(ascii); ascii=NULL; }
lasttype=type;
h=h->next;
}

GLOBALS->color_active_in_filter = 0;

wave_gdk_draw_line_flush(GLOBALS->wavepixmap_wavewindow_c_1);

GLOBALS->tims.start+=GLOBALS->shift_timebase;
GLOBALS->tims.end+=GLOBALS->shift_timebase;
}

/*
 * $Id$
 * $Log$
 * Revision 1.74  2010/10/26 17:37:35  gtkwave
 * added initial_signal_window_width rc variable
 *
 * Revision 1.73  2010/08/02 20:44:35  gtkwave
 * added gtkwave::cbTracesUpdated
 *
 * Revision 1.72  2010/06/23 05:45:34  gtkwave
 * warnings fixes
 *
 * Revision 1.71  2010/04/15 20:38:51  gtkwave
 * color usage fixes for black and white mode
 *
 * Revision 1.70  2010/04/14 18:45:38  gtkwave
 * allow data values for secondary transaction traces
 *
 * Revision 1.69  2010/04/14 07:49:02  gtkwave
 * updated mouseover handling
 *
 * Revision 1.68  2010/04/14 04:21:46  gtkwave
 * update populateBuffer() usage
 *
 * Revision 1.67  2010/04/10 19:07:38  gtkwave
 * support for mouseover with extension traces
 *
 * Revision 1.66  2010/04/10 03:32:00  gtkwave
 * allow transaction traces to scroll off top of screen yet still be visible
 *
 * Revision 1.65  2010/04/09 20:52:33  gtkwave
 * add extension traces for extension transactions
 *
 * Revision 1.64  2010/04/01 03:10:58  gtkwave
 * time warp fixes
 *
 * Revision 1.63  2010/03/31 15:42:48  gtkwave
 * added preliminary transaction filter support
 *
 * Revision 1.62  2010/03/24 23:05:10  gtkwave
 * added RealToBits menu option
 *
 * Revision 1.61  2010/03/14 07:09:49  gtkwave
 * removed ExtNode and merged with Node
 *
 * Revision 1.60  2010/03/02 18:30:07  gtkwave
 * fix for analog stretch traces when expanded then collapsed
 *
 * Revision 1.59  2010/02/22 17:15:11  gtkwave
 * add y-coord clamping to avoid integer round off errors
 *
 * Revision 1.58  2010/02/21 05:15:16  gtkwave
 * analog drawing fix for clipping + overdraw
 *
 * Revision 1.57  2010/02/07 20:16:34  gtkwave
 * experiment with adding line clipping to analog rendering
 *
 * Revision 1.56  2010/01/22 02:10:49  gtkwave
 * added second pattern search capability
 *
 * Revision 1.55  2010/01/18 20:47:02  gtkwave
 * added named locker marking
 *
 * Revision 1.54  2009/12/24 20:55:28  gtkwave
 * warnings cleanups
 *
 * Revision 1.53  2009/12/24 03:02:14  gtkwave
 * move cached vtran up earlier to make more visually pleasing
 *
 * Revision 1.52  2009/11/20 20:48:19  gtkwave
 * enabled line drawing caching for linux
 *
 * Revision 1.51  2009/09/14 03:00:09  gtkwave
 * bluespec code integration
 *
 * Revision 1.50  2009/07/24 03:21:31  gtkwave
 * added fisher price mode to GCs as a ./configure option
 *
 * Revision 1.49  2009/06/26 19:56:18  gtkwave
 * add arrowheads to all impulse arrows
 *
 * Revision 1.48  2009/06/25 22:12:30  gtkwave
 * convert event impulses to strict 1/0 activity values regardless of val
 *
 * Revision 1.47  2009/06/25 18:31:19  gtkwave
 * added event types for VCD+FST and impulse arrows on event types
 *
 * Revision 1.46  2009/03/24 21:31:34  gtkwave
 * changed dkgray to mdgray for "Time" background as intensity changed
 *
 * Revision 1.45  2009/01/09 16:18:53  gtkwave
 * mingw compile fix for earlier -Wshadow patch
 *
 * Revision 1.44  2008/12/31 22:20:12  gtkwave
 * adding more tcl commands
 *
 * Revision 1.43  2008/12/25 04:14:11  gtkwave
 * -Wshadow warning fixes
 *
 * Revision 1.42  2008/12/11 21:08:15  gtkwave
 * allow support for marker names which are strings
 *
 * Revision 1.41  2008/12/09 00:36:42  gtkwave
 * added mouseover support for signal window
 *
 * Revision 1.40  2008/11/25 20:00:54  gtkwave
 * added use_scrollwheel_as_y rc variable
 *
 * Revision 1.39  2008/11/14 18:32:23  gtkwave
 * added check on GLOBALS->signalwindow pointer validity for set_usize op
 *
 * Revision 1.38  2008/10/12 02:56:20  gtkwave
 * fix for blackout regions
 *
 * Revision 1.37  2008/08/24 21:55:27  gtkwave
 * update for timestart line in savefiles
 *
 * Revision 1.36  2008/07/01 18:51:07  gtkwave
 * compiler warning fixes for amd64
 *
 * Revision 1.35  2008/06/11 08:01:57  gtkwave
 * gcc 4.3.x compiler warning fixes
 *
 * Revision 1.34  2008/04/03 16:56:11  gtkwave
 * more time_trunc() removals
 *
 * Revision 1.33  2008/03/20 01:15:13  gtkwave
 * black and white mode color bug fixed when process filter color escapes used
 *
 * Revision 1.32  2008/02/14 04:15:43  gtkwave
 * fixed recently introduced wavewindow clearing bug
 *
 * Revision 1.31  2008/02/08 02:26:36  gtkwave
 * anti-aliased font support add
 *
 * Revision 1.30  2008/01/28 23:08:50  gtkwave
 * added infinite scaling define in currenttime.h
 *
 * Revision 1.29  2008/01/27 23:54:05  gtkwave
 * right side margin NaN fix
 *
 * Revision 1.28  2008/01/27 01:21:05  gtkwave
 * make transition to/from inf/-inf a square wave when interpolated
 *
 * Revision 1.27  2008/01/26 19:13:34  gtkwave
 * replace NaN trapezoids with NaN boxes
 *
 * Revision 1.26  2008/01/26 05:02:14  gtkwave
 * added parallelogram rendering for nans
 *
 * Revision 1.25  2008/01/25 23:29:23  gtkwave
 * modify analog slightly for nan and inf handling
 *
 * Revision 1.24  2008/01/25 21:50:35  gtkwave
 * added clipping on analog waveforms for horiz lines > tims.last xpos
 *
 * Revision 1.23  2008/01/25 04:10:15  gtkwave
 * added new resizing options to menu
 *
 * Revision 1.22  2008/01/24 20:19:39  gtkwave
 * analog rendering fixes
 *
 * Revision 1.21  2008/01/23 04:49:32  gtkwave
 * more tweaking of interpolated+step mode (use snap dots)
 *
 * Revision 1.20  2008/01/23 02:05:44  gtkwave
 * added interpolated + step mode
 *
 * Revision 1.19  2008/01/22 20:11:47  gtkwave
 * track and hold experimentation
 *
 * Revision 1.18  2008/01/21 16:54:05  gtkwave
 * some analog fixes for end of screen
 *
 * Revision 1.17  2008/01/20 08:56:35  gtkwave
 * added dirty_kick to MaxSignalLength
 *
 * Revision 1.16  2008/01/12 21:36:44  gtkwave
 * added black and white vs color rendering menu options
 *
 * Revision 1.15  2008/01/09 19:20:53  gtkwave
 * more updating to globals management (expose events cause wrong swap)
 *
 * Revision 1.14  2008/01/08 18:21:23  gtkwave
 * focus in/out rendering improvements
 *
 * Revision 1.13  2008/01/08 07:13:08  gtkwave
 * more limiting of ctrl-a focus (away from tree and filter entry)
 *
 * Revision 1.12  2008/01/07 16:34:51  gtkwave
 * force hscrollbar update when button release after nailing down marker
 *
 * Revision 1.11  2008/01/04 22:47:56  gtkwave
 * prelim input focus support for singalwindow
 *
 * Revision 1.10  2008/01/03 21:55:45  gtkwave
 * various cleanups
 *
 * Revision 1.9  2007/09/17 16:00:51  gtkwave
 * yet more stability updates for tabbed viewing
 *
 * Revision 1.8  2007/09/14 16:23:17  gtkwave
 * remove expose events from ctx management
 *
 * Revision 1.7  2007/09/13 21:24:45  gtkwave
 * configure_events must be beyond watchdog monitoring due to how gtk generates one per tab
 *
 * Revision 1.6  2007/09/12 17:26:45  gtkwave
 * experimental ctx_swap_watchdog added...still tracking down mouse thrash crashes
 *
 * Revision 1.5  2007/09/10 19:43:20  gtkwave
 * gtk1.2 compile fixes
 *
 * Revision 1.4  2007/08/26 21:35:50  gtkwave
 * integrated global context management from SystemOfCode2007 branch
 *
 * Revision 1.1.1.1.2.8  2007/08/25 19:43:46  gtkwave
 * header cleanups
 *
 * Revision 1.1.1.1.2.7  2007/08/07 03:18:56  kermin
 * Changed to pointer based GLOBAL structure and added initialization function
 *
 * Revision 1.1.1.1.2.6  2007/08/06 03:50:50  gtkwave
 * globals support for ae2, gtk1, cygwin, mingw.  also cleaned up some machine
 * generated structs, etc.
 *
 * Revision 1.1.1.1.2.5  2007/08/05 02:27:28  kermin
 * Semi working global struct
 *
 * Revision 1.1.1.1.2.4  2007/07/31 03:18:02  kermin
 * Merge Complete - I hope
 *
 * Revision 1.1.1.1.2.3  2007/07/28 19:50:40  kermin
 * Merged in the main line
 *
 * Revision 1.3  2007/07/23 23:21:23  gtkwave
 * was missing rc.h include
 *
 * Revision 1.2  2007/07/23 23:13:09  gtkwave
 * adds for color tags in filtered trace data
 *
 * Revision 1.1.1.1  2007/05/30 04:28:00  gtkwave
 * Imported sources
 *
 * Revision 1.3  2007/05/28 00:55:06  gtkwave
 * added support for arrays as a first class dumpfile datatype
 *
 * Revision 1.2  2007/04/20 02:08:18  gtkwave
 * initial release
 *
 */

