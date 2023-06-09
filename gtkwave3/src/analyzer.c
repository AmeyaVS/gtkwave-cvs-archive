/* 
 * Copyright (c) Tony Bybell 1999-2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/* AIX may need this for alloca to work */ 
#if defined _AIX
  #pragma alloca
#endif

#include "globals.h"
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol.h"
#include "lxt.h"
#include "debug.h"
#include "bsearch.h"
#include "strace.h"
#include "translate.h"
#include "ptranslate.h"
#include "ttranslate.h"
#include "hierpack.h"
#include "analyzer.h"

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

void UpdateTraceSelection(Trptr t);
int traverse_vector_nodes(Trptr t);

/*
 * extract last n levels of hierarchy
 */
char *hier_extract(char *pnt, int levels)
{
int i, len;
char ch, *pnt2, *esc;
char only_nums_so_far=1;

if(!pnt) return(NULL);

len=strlen(pnt);
if(!len) return(pnt);

if(levels<1) levels=1;
if((esc=strchr(pnt, '\\'))) 
	{
	return((levels==1) ? esc : pnt); /* shortcut out on escape IDs: level=1, esc char else all */
	}

pnt2=pnt+len-1;
ch=*pnt2;

for(i=0;i<len;i++)
	{
	ch=*(pnt2--);
	if((only_nums_so_far)&&(ch>='0')&&(ch<='9'))		/* skip 1st set of signal.number hier from right if it exists */
		{
		continue;
		/* nothing */
		}
	else
		{
		if(ch==GLOBALS->hier_delimeter) 
			{
			if(!only_nums_so_far) levels--;
			if(!levels)
				{
				pnt2+=2;
				return(pnt2);
				}
			}
		only_nums_so_far=0;
		}
	}

return(pnt); /* not as many levels as max, so give the full name.. */
}

void updateTraceGroup(Trptr t)
{
  /*  t->t_match = NULL; */

  if (t->t_prev)
    {
      if (IsGroupBegin(t->t_prev))
	{
	  if (IsGroupEnd(t))
	    { /* empty group */
	      Trptr g_begin = t->t_prev;
	      t->t_grp = g_begin->t_grp;
	      t->t_match = g_begin;
	      g_begin->t_match = t;
	    }
	  else
	    { /* first trace in group */
	      t->t_grp = t->t_prev;
	    }
	}
      else
	{
	  if (IsGroupEnd(t))
	    {
	      Trptr g_begin = t->t_prev->t_grp;
	      t->t_grp = g_begin->t_grp;
	      t->t_match = g_begin;
	      g_begin->t_match = t;
	    }
	  else
	    { 
	      t->t_grp = t->t_prev->t_grp;
	    }


	}
    }
  else 
    { /* very first trace */
      t->t_grp = NULL;
    }

  if ((t->t_grp) && IsSelected(t->t_grp))
    {
      t->flags |= TR_HIGHLIGHT;
    }
}

void CloseTrace(Trptr t)
{
GLOBALS->traces.dirty = 1;

  if (IsGroupBegin(t))
    {
      t->flags |= TR_CLOSED;
      if (t->t_match) { t->t_match->flags |= TR_CLOSED; };

      if (!HasWave(t))
	{
	  /* Group End */
	  if (t->t_match) { t->t_match->flags |= TR_COLLAPSED; };
	}
      else
	{
	  /* Composite  End */
	  if (t->t_match) { t->t_match->flags |= TR_COLLAPSED; };
	}
    }

  if (IsGroupEnd(t))
    {
      t->flags |= TR_CLOSED;
      if (t->t_match) { t->t_match->flags |= TR_CLOSED; };

      if ((t->t_match) && !HasWave(t->t_match))
	{
	  /* Group End */
	  t->flags |= TR_COLLAPSED;
	}
      else
	{
	  /* Composite End */
	  t->flags |= TR_COLLAPSED;
	}
    }
}


void OpenTrace(Trptr t)
{
GLOBALS->traces.dirty = 1;

  if (IsGroupBegin(t) || IsGroupEnd(t))
  {
    t->flags &= ~TR_CLOSED;
    if (t->t_match) { t->t_match->flags &= ~TR_CLOSED; };

    if (!HasWave(t))
      {
	t->flags &= ~TR_COLLAPSED;
	if(t->t_match) { t->t_match->flags &= ~TR_COLLAPSED; };
      }
  }
}

void ClearTraces(void)
{
  Trptr t = GLOBALS->traces.first;
  while(t)
	{
	  t->flags &= ~TR_HIGHLIGHT;
	  t=t->t_next;
	}
  GLOBALS->traces.dirty = 1;
}

void ClearGroupTraces(Trptr t_grp)
{
  if (IsGroupBegin(t_grp))
    {
      Trptr t = t_grp;
      while(t)
	{
	  t->flags &= ~TR_HIGHLIGHT;
	  if(t->t_match == t_grp) break;
	  t=t->t_next;
	}
    GLOBALS->traces.dirty = 1;
    }
  else
    {
      fprintf(stderr, "INTERNAL ERROR: ClearGroupTrace applied to non-group!  Exiting.\n");
      exit(255);
    }
}

/* 
 * Add a trace to the display...
 */
static void AddTrace( Trptr t )
{
GLOBALS->traces.dirty = 1;

if(GLOBALS->default_flags&TR_NUMMASK) t->flags=GLOBALS->default_flags;
	else t->flags=(t->flags&TR_NUMMASK)|GLOBALS->default_flags;

if(GLOBALS->default_flags & TR_FTRANSLATED)
	{
	t->f_filter = GLOBALS->current_translate_file;
	}
else
if(GLOBALS->default_flags & TR_PTRANSLATED)
	{
	t->p_filter = GLOBALS->current_translate_proc;
	}

/* NOT an else! */
if(GLOBALS->default_flags & TR_TTRANSLATED)
	{
	t->t_filter = GLOBALS->current_translate_ttrans;
	traverse_vector_nodes(t);
	}

 if (IsGroupBegin(t)) {
   GLOBALS->group_depth = GLOBALS->group_depth + 1;
 }

 if (IsGroupEnd(t)) {
   if (GLOBALS->group_depth == 0) {
     fprintf(stderr, "ERROR: Group End encountered with no matching start. Ignoring.\n");
     t->flags &= ~TR_GRP_END;
   } else {
     GLOBALS->group_depth = GLOBALS->group_depth - 1;
   }
 }

if(GLOBALS->shift_timebase_default_for_add)
	t->shift=GLOBALS->shift_timebase_default_for_add;

if(!GLOBALS->strace_ctx->shadow_active)
	{
	if( GLOBALS->traces.first == NULL )
		{
		t->t_next = t->t_prev = NULL;
		GLOBALS->traces.first = GLOBALS->traces.last = t;
	      	}
	    	else
	      	{
		t->t_next = NULL;
		t->t_prev = GLOBALS->traces.last;
		GLOBALS->traces.last->t_next = t;
		GLOBALS->traces.last = t;
	      	}
	GLOBALS->traces.total++;
	updateTraceGroup(GLOBALS->traces.last);
	}
	else	/* hide offscreen */
	{
	struct strace *st = calloc_2(1, sizeof(struct strace));
	st->next = GLOBALS->strace_ctx->shadow_straces;
	st->value = GLOBALS->strace_ctx->shadow_type;
	st->trace = t;

	st->string = GLOBALS->strace_ctx->shadow_string; /* copy string over */
	GLOBALS->strace_ctx->shadow_string = NULL;

	GLOBALS->strace_ctx->shadow_straces = st;
	}
}


/*
 * Add a blank trace to the display...
 */
static char *precondition_string(char *s)
{
int len=0;
char *s2;

if(!s) return(NULL);
s2=s;
while((*s2)&&((*s2)!='\n')&&((*s2)!='\r'))	/* strip off ending CR/LF */
	{
	len++;
	s2++;
	}
if(!len) return(NULL);
s2=(char *)calloc_2(1,len+1);
memcpy(s2,s,len);
return(s2);
}

int AddBlankTrace(char *commentname)
{
  Trptr  t;
  char *comment;
  unsigned int flags_filtered;

  if( (t = (Trptr) calloc_2( 1, sizeof( TraceEnt ))) == NULL )
    {
      fprintf( stderr, "Out of memory, can't add blank trace to analyzer\n");
      return( 0 );
    }
  AddTrace(t);
  /* Keep only flags that make sense for a blank trace. */
  flags_filtered = TR_BLANK | (GLOBALS->default_flags & (TR_CLOSED|
							 TR_GRP_BEGIN|
							 TR_GRP_END|
							 TR_COLLAPSED|
							 TR_ANALOG_BLANK_STRETCH));
  t->flags = flags_filtered;
  if(t->flags & TR_ANALOG_BLANK_STRETCH)

    {
      t->flags &= ~TR_BLANK;
    }

  if((comment=precondition_string(commentname)))
    {
      t->name      = comment;
    }

  return(1);
}


/*
 * Insert a blank [or comment] trace into the display...
 */
int InsertBlankTrace(char *comment, int different_flags)
{
TempBuffer tb;
char *comm;
Trptr  t;

if( (t = (Trptr) calloc_2( 1, sizeof( TraceEnt ))) == NULL )
	{
	fprintf( stderr, "Out of memory, can't insert blank trace to analyzer\n");
	return( 0 );
      	}

GLOBALS->traces.dirty = 1;

if(!different_flags)
	{
	t->flags=TR_BLANK;
	}
	else
	{
	t->flags = different_flags;
	}

if((comm=precondition_string(comment)))
	{
	t->name = comm;
	}

if(!GLOBALS->traces.first)
	{
	GLOBALS->traces.first=GLOBALS->traces.last=t;
	GLOBALS->traces.total=1;
	return(1);
	}
	else
	{
	tb.buffer=GLOBALS->traces.buffer;
	tb.bufferlast=GLOBALS->traces.bufferlast;
	tb.buffercount=GLOBALS->traces.buffercount;
	
	GLOBALS->traces.buffer=GLOBALS->traces.bufferlast=t;
	GLOBALS->traces.buffercount=1;
	PasteBuffer();

	GLOBALS->traces.buffer=tb.buffer;
	GLOBALS->traces.bufferlast=tb.bufferlast;
	GLOBALS->traces.buffercount=tb.buffercount;

	return(1);
	}
}


/*
 * Adds a single bit signal to the display...
 */
int AddNodeTraceReturn(nptr nd, char *aliasname, Trptr *tret)
{
  Trptr  t;
  hptr histpnt;
  hptr *harray;
  int histcount;
  int i;

  if(!nd) return(0); /* passed it a null node ptr by mistake */
  if(nd->mv.mvlfac) import_trace(nd);

  GLOBALS->signalwindow_width_dirty=1;
  GLOBALS->traces.dirty = 1;
    
  if( (t = (Trptr) calloc_2( 1, sizeof( TraceEnt ))) == NULL )
    {
      fprintf( stderr, "Out of memory, can't add to analyzer\n" );
      return( 0 );
    }

  if(!nd->harray)		/* make quick array lookup for aet display */
    {
      histpnt=&(nd->head);
      histcount=0;

      while(histpnt)
	{
	  histcount++;
	  histpnt=histpnt->next;
	}

      nd->numhist=histcount;
	
      if(!(nd->harray=harray=(hptr *)malloc_2(histcount*sizeof(hptr))))
	{
	  fprintf( stderr, "Out of memory, can't add to analyzer\n" );
	  free_2(t);
	  return(0);
	}

      histpnt=&(nd->head);
      for(i=0;i<histcount;i++)
	{
	  *harray=histpnt;
	  harray++;
	  histpnt=histpnt->next;
	}
    }

  if(aliasname)
   {	
     char *alias;

     t->name_full = alias =(char *)malloc_2(strlen(aliasname)+1);
     strcpy(alias,aliasname);
     t->name = t->name_full;
     if(GLOBALS->hier_max_level) 
       t->name = hier_extract(t->name_full, GLOBALS->hier_max_level);
   }
  else
    {
      if(!GLOBALS->hier_max_level) 
	{
	  int flagged = 0;

	  t->name = hier_decompress_flagged(nd->nname, &flagged);
	  t->is_depacked = (flagged != 0);
	}
      else
	{
	  int flagged = 0;
	  char *tbuff = hier_decompress_flagged(nd->nname, &flagged);
	  if(!flagged)
	    {
	      t->name = hier_extract(nd->nname, GLOBALS->hier_max_level);
	    }
	  else
	    {
	      t->name = strdup_2(hier_extract(tbuff, GLOBALS->hier_max_level));
	      free_2(tbuff);
	      t->is_depacked = 1;
	    }
	}
    }

  if(nd->extvals) /* expansion vectors */
    {	
      int n;

      n = nd->msi - nd->lsi;
      if(n<0)n=-n;
      n++;

      t->flags = (( n > 3 )||( n < -3 )) ? TR_HEX|TR_RJUSTIFY : TR_BIN|TR_RJUSTIFY;
    }
  else
    {
      t->flags |= TR_BIN;	/* binary */
    }
  t->vector = FALSE;
  t->n.nd = nd;
  if(tret) *tret = t;		/* for expand */
  AddTrace( t );
  return( 1 );
}


/* single node */
int AddNode(nptr nd, char *aliasname)
{
return(AddNodeTraceReturn(nd, aliasname, NULL));
}


/* add multiple nodes (if array) */
int AddNodeUnroll(nptr nd, char *aliasname)
{
if(nd->array_height <= 1)
	{
	return(AddNodeTraceReturn(nd, aliasname, NULL));
	}
	else
	{
	int i;
	int rc = 1;

	for(i=0;i<nd->array_height;i++)
		{
		rc |= AddNodeTraceReturn(nd+i, aliasname, NULL);
		}
	return(rc);
	}
}


/*
 * Adds a vector to the display...
 */
int AddVector(bvptr vec, char *aliasname)
  {
    Trptr  t;
    int    n;

    if(!vec) return(0); /* must've passed it a null pointer by mistake */

    GLOBALS->signalwindow_width_dirty=1;
    GLOBALS->traces.dirty = 1;

    n = vec->nbits;
    t = (Trptr) calloc_2(1, sizeof( TraceEnt ) );
    if( t == NULL )
      {
	fprintf( stderr, "Out of memory, can't add %s to analyzer\n",
	  vec->bvname );
	return( 0 );
      }

    if (aliasname)
      {
	t->name_full = strdup_2(aliasname);
	t->name = t->name_full;
      }
    else
      {
	t->name = vec->bvname;
      }

    if(GLOBALS->hier_max_level)
      t->name = hier_extract(t->name, GLOBALS->hier_max_level);

    t->flags = ( n > 3 ) ? TR_HEX|TR_RJUSTIFY : TR_BIN|TR_RJUSTIFY;
    t->vector = TRUE;
    t->n.vec = vec;
    AddTrace( t );
    return( 1 );
  }


/*
 * Free up a trace's mallocs...
 */
void FreeTrace(Trptr t)
{
GLOBALS->traces.dirty = 1;

if(GLOBALS->starting_unshifted_trace == t)
	{
	GLOBALS->starting_unshifted_trace = NULL; /* for new "standard" clicking routines */
	}

if(GLOBALS->strace_ctx->straces)
	{
	struct strace_defer_free *sd = calloc_2(1, sizeof(struct strace_defer_free));
	sd->next = GLOBALS->strace_ctx->strace_defer_free_head;
	sd->defer = t;

	GLOBALS->strace_ctx->strace_defer_free_head = sd;
	return;
	}

if(t->vector)
      	{
      	bvptr bv, bv2;
	int i;

	bv=t->n.vec;
	/* back out allocation to revert (if any) */
        if(bv->transaction_cache)
		{ 
		t->n.vec = bv->transaction_cache;

		while(bv)
			{
			bv2 = bv->transaction_chain;
			if(bv->bvname) { free_2(bv->bvname); }

	                for(i=0;i<bv->numregions;i++)
				{
	                        free_2(bv->vectors[i]);
	                        }

	                free_2(bv);
			bv = bv2;
			}			

		bv=t->n.vec;
                }

	/* normal vector deallocation */
	for(i=0;i<bv->numregions;i++)
		{
		if(bv->vectors[i]) free_2(bv->vectors[i]);
		}
	
	if(bv->bits)
		{
		if(bv->bits->name) free_2(bv->bits->name);
		if(bv->bits->attribs) free_2(bv->bits->attribs);
		for(i=0;i<bv->nbits;i++)
			{
			DeleteNode(bv->bits->nodes[i]);
			}
		free_2(bv->bits);
		}

	if(bv->bvname) free_2(bv->bvname);
      	if(t->n.vec) free_2(t->n.vec);
      	}
	else
	{
	if(t->n.nd && t->n.nd->expansion)
		{
		DeleteNode(t->n.nd);
		}
	}

 if(t->is_depacked) free_2(t->name);
 if(t->asciivalue) free_2(t->asciivalue);
 if(t->name_full)  free_2(t->name_full);
 free_2( t );
}


/*
 * Remove a trace from the display and optionally 
 * deallocate its memory usage...
 */ 
void RemoveTrace( Trptr t, int dofree )
  {
    GLOBALS->traces.dirty = 1;
    GLOBALS->traces.total--;
    if( t == GLOBALS->traces.first )
      {
	GLOBALS->traces.first = t->t_next;
	if( t->t_next )
            t->t_next->t_prev = NULL;
        else
            GLOBALS->traces.last = NULL;
      }
    else
      {
        if(t->t_prev)
		{
	        t->t_prev->t_next = t->t_next;
		}
		else
		{
		/* this code should likely *never* execute as if( t == GLOBALS->traces.first ) above should catch this */
		/* there is likely a problem elsewhere in the code! */

		Trptr t2 = GLOBALS->traces.first = t->t_next;
		GLOBALS->traces.total = 0;
		while(t2)
			{
			t2 = t2->t_next;
			GLOBALS->traces.total++;
			}
		}


        if( t->t_next )
            t->t_next->t_prev = t->t_prev;
        else
            GLOBALS->traces.last = t->t_prev;
      }
    
    if(dofree)
	{
        FreeTrace(t);
	}
  }


/*
 * Deallocate the cut/paste buffer...
 */
void FreeCutBuffer(void)
{
Trptr t, t2;

t=GLOBALS->traces.buffer;

while(t)
	{
	t2=t->t_next;
	FreeTrace(t);
	t=t2;	
	}

GLOBALS->traces.buffer=GLOBALS->traces.bufferlast=NULL;
GLOBALS->traces.buffercount=0;
}


/*
 * Cut highlighted traces from the main screen
 * and throw them in the cut buffer.  If anything's
 * in the cut buffer, deallocate it first...
 */
Trptr CutBuffer(void)
{
Trptr t, tnext;
Trptr first=NULL, current=NULL;

GLOBALS->shift_click_trace=NULL;		/* so shift-clicking doesn't explode */

t=GLOBALS->traces.first;
while(t)
	{
	if((t->flags)&(TR_HIGHLIGHT)) break;
	t=t->t_next;
	}
if(!t) return(NULL);	/* keeps a double cut from blowing out the buffer */

GLOBALS->signalwindow_width_dirty=1;
GLOBALS->traces.dirty = 1;

FreeCutBuffer();

t=GLOBALS->traces.first;
while(t)
	{
	tnext=t->t_next;
	if(IsSelected(t) || (t->t_grp && IsSelected(t->t_grp)))
	  {
	    /* members of closed groups may not be highlighted */
	    /* so propogate highlighting here */
	    t->flags |= TR_HIGHLIGHT;
	    GLOBALS->traces.bufferlast=t;
	    GLOBALS->traces.buffercount++;

	    /* t->flags&=(~TR_HIGHLIGHT); */
	    RemoveTrace(t, 0);
	    if(!current)
	      {
		first=current=t;
		t->t_prev=NULL;
		t->t_next=NULL;
	      }
	    else
	      {
		current->t_next=t;
		t->t_prev=current;
		current=t;
		t->t_next=NULL;
	      }
	  }
	t=tnext;
	}

return(GLOBALS->traces.buffer=first);
}


/*
 * Paste the cut buffer into the main display one and
 * mark the cut buffer empty...
 */
Trptr PasteBuffer(void)
{
  Trptr t, tinsert=NULL, tinsertnext;
  int count;
  Trptr prev;

  if(!GLOBALS->traces.buffer) return(NULL);

  GLOBALS->signalwindow_width_dirty=1;
  GLOBALS->traces.dirty = 1;

  if(!(t=GLOBALS->traces.first))
    {
      t=GLOBALS->traces.last=GLOBALS->traces.first=GLOBALS->traces.buffer;
      prev = NULL;
      while(t)
	{
          t->t_prev = prev; /* defensive re-link move */
	  prev = t;
	  GLOBALS->traces.last=t;
	  GLOBALS->traces.total++;
	  t=t->t_next;
	}	

      GLOBALS->traces.buffer=GLOBALS->traces.bufferlast=NULL;
      GLOBALS->traces.buffercount=0;

      return(GLOBALS->traces.first);
    }

  while(t)
    {
      if(t->flags&TR_HIGHLIGHT) 
	{
	  tinsert=t;
	}
      t=t->t_next;
    }


  if(!tinsert) tinsert=GLOBALS->traces.last;

  if(IsGroupBegin(tinsert) && IsClosed(tinsert) && IsCollapsed(tinsert->t_match))
    tinsert=tinsert->t_match;

  tinsertnext=tinsert->t_next;
  tinsert->t_next=GLOBALS->traces.buffer;
  GLOBALS->traces.buffer->t_prev=tinsert;
  GLOBALS->traces.bufferlast->t_next=tinsertnext;
  GLOBALS->traces.total+=GLOBALS->traces.buffercount;

  if(!tinsertnext)
    {
      GLOBALS->traces.last=GLOBALS->traces.bufferlast;
    }
  else
    {
      tinsertnext->t_prev=GLOBALS->traces.bufferlast;
    }

  GLOBALS->traces.scroll_top = GLOBALS->traces.buffer;
  GLOBALS->traces.scroll_bottom = GLOBALS->traces.bufferlast;

  if(GLOBALS->traces.first)
    {
      t = GLOBALS->traces.first;
      t->t_grp = NULL;
      while(t)
	{
	  updateTraceGroup(t);
	  t->flags &= ~TR_HIGHLIGHT;
	  t=t->t_next;
	}
    }

  count = 0;

  if (GLOBALS->traces.buffer)
    {
      t = GLOBALS->traces.buffer;
      while(t)
	{
	  t->flags |= TR_HIGHLIGHT;
	  t=t->t_next;
	  count++;
	  if (count == GLOBALS->traces.buffercount) break;
	}
    }

  /* clean out the buffer */
  GLOBALS->traces.buffer=GLOBALS->traces.bufferlast=NULL;
  GLOBALS->traces.buffercount=0;

  /* defensive re-link */
  t=GLOBALS->traces.first;
  prev = NULL;
  while(t)
	{
        t->t_prev = prev;
	prev = t;
	t=t->t_next;
	}	

  return(GLOBALS->traces.first);
}


/*
 * Prepend the cut buffer into the main display one and
 * mark the cut buffer empty...
 */
Trptr PrependBuffer(void)
{
Trptr t, prev = NULL;
int count;

if(!GLOBALS->traces.buffer) return(NULL);

GLOBALS->signalwindow_width_dirty=1;
GLOBALS->traces.dirty = 1;

t=GLOBALS->traces.buffer;

while(t)
	{
	t->t_prev = prev; /* defensive re-link move */
	prev=t;
	t->flags&=(~TR_HIGHLIGHT);
	GLOBALS->traces.total++;
	t=t->t_next;
	}

if((prev->t_next=GLOBALS->traces.first))
	{
	/* traces.last current value is ok as it stays the same */
	GLOBALS->traces.first->t_prev=prev; /* but we need the reverse link back up */
	}
	else
	{
	GLOBALS->traces.last=prev;
	}

GLOBALS->traces.first=GLOBALS->traces.buffer;

if(GLOBALS->traces.first)
  {
    t = GLOBALS->traces.first;
    t->t_grp = NULL;
    while(t)
      {
	updateTraceGroup(t);
	t->flags &= ~TR_HIGHLIGHT;
	t=t->t_next;
      }
  }

 count = 0;

 if (GLOBALS->traces.buffer)
   {
     t = GLOBALS->traces.buffer;
     while(t)
       {
	 t->flags |= TR_HIGHLIGHT;
	 t=t->t_next;
	 count++;
	 if (count == GLOBALS->traces.buffercount) break;
      }
   }

/* clean out the buffer */
GLOBALS->traces.buffer=GLOBALS->traces.bufferlast=NULL;
GLOBALS->traces.buffercount=0;

/* defensive re-link */
t=GLOBALS->traces.first;
prev = NULL;
while(t)
	{
        t->t_prev = prev;
	prev = t;
	t=t->t_next;
	}	

return(GLOBALS->traces.first);
}


/*
 * avoid sort/rvs manipulations if there are group traces (for now)
 */
static int groupsArePresent(void)
{
Trptr t;
int i, rc = 0;

t=GLOBALS->traces.first;
for(i=0;i<GLOBALS->traces.total;i++)
        {
        if(!t)
                {
                fprintf(stderr, "INTERNAL ERROR: traces.total vs traversal mismatch!  Exiting.\n");
                exit(255);
                }

	if((t->t_grp)||(t->t_match)||(t->flags & TR_GRP_MASK))
		{
		rc = 1; break;
		}

        t=t->t_next;
        }

return(rc);
}


/*************************************************************/


/*
 * sort on tracename pointers (alpha/caseins alpha/sig sort full_reverse)
 */
static int tracenamecompare(const void *s1, const void *s2)
{
char *str1, *str2;

str1=(*((Trptr *)s1))->name;
str2=(*((Trptr *)s2))->name;

if((!str1) || (!*str1))	/* force blank lines to go to bottom */
	{
	if((!str2) || (!*str2))
		{
		return(0);
		}
		else
		{
		return(1);
		}
	}
else
if((!str2) || (!*str2))
	{
	return(-1);		/* str1==str2==zero case is covered above */
	}
  
return(strcmp(str1, str2));
}


static int traceinamecompare(const void *s1, const void *s2)
{
char *str1, *str2;

str1=(*((Trptr *)s1))->name;
str2=(*((Trptr *)s2))->name;

if((!str1) || (!*str1))	/* force blank lines to go to bottom */
	{
	if((!str2) || (!*str2))
		{
		return(0);
		}
		else
		{
		return(1);
		}
	}
else
if((!str2) || (!*str2))
	{
	return(-1);		/* str1==str2==zero case is covered above */
	}
  
return(strcasecmp(str1, str2));
}

static int tracesignamecompare(const void *s1, const void *s2)
{
char *str1, *str2;

str1=(*((Trptr *)s1))->name;
str2=(*((Trptr *)s2))->name;

if((!str1) || (!*str1))	/* force blank lines to go to bottom */
	{
	if((!str2) || (!*str2))
		{
		return(0);
		}
		else
		{
		return(1);
		}
	}
else
if((!str2) || (!*str2))
	{
	return(-1);		/* str1==str2==zero case is covered above */
	}
  
return(sigcmp(str1, str2));
}


/*
 * alphabetization/reordering of traces
 */
int TracesReorder(int mode)
{
Trptr t, prev = NULL;
Trptr *tsort, *tsort_pnt;
#ifdef WAVE_HIERFIX
char *subst, ch;
#endif
int i;
int (*cptr)(const void*, const void*);
   
if(!GLOBALS->traces.total) return(0);
GLOBALS->traces.dirty = 1;

t=GLOBALS->traces.first;
tsort=tsort_pnt=wave_alloca(sizeof(Trptr)*GLOBALS->traces.total);   
for(i=0;i<GLOBALS->traces.total;i++)
        {
        if(!t)
                {
                fprintf(stderr, "INTERNAL ERROR: traces.total vs traversal mismatch!  Exiting.\n");
                exit(255);
                }
        *(tsort_pnt++)=t;

#ifdef WAVE_HIERFIX
	if((subst=t->name))
	        while((ch=(*subst)))
        	        {
        	        if(ch==GLOBALS->hier_delimeter) { *subst=VCDNAM_HIERSORT; } /* forces sort at hier boundaries */
        	        subst++;
        	        }
#endif

        t=t->t_next;
        }

switch(mode)
	{
	case TR_SORT_INS:  	cptr=traceinamecompare;   break;
	case TR_SORT_NORM: 	cptr=tracenamecompare;	  break;
	case TR_SORT_LEX:  	cptr=tracesignamecompare; break;
	default: 		cptr=NULL; break;
	}

if((cptr) && (!groupsArePresent()))
	{
	qsort(tsort, GLOBALS->traces.total, sizeof(Trptr), cptr);
	}
	else /* keep groups segregated off on the side and sort names + (indirect pointer to) top-level groups */
	{
	Trptr *tsort_reduced = wave_alloca(sizeof(Trptr)*GLOBALS->traces.total);
	int num_reduced = 0;
	int j;

	for(i=0;i<GLOBALS->traces.total;i++)
	        {
	        if(tsort[i]->flags & TR_GRP_BEGIN)
	                {
	                int cnt = 0;
	                
	                for(j=i;j<GLOBALS->traces.total;j++)
	                        {
	                        if(tsort[j]->flags & TR_GRP_BEGIN) { cnt++; }   
	                        else if(tsort[j]->flags & TR_GRP_END) { cnt--; }
	                        
	                        if(!cnt) 
	                                {
	                                tsort_reduced[num_reduced] = calloc_2(1, sizeof(struct TraceEnt));
	                                tsort_reduced[num_reduced]->name = tsort[i]->name;
	                                tsort_reduced[num_reduced]->is_sort_group = 1;
	                                tsort_reduced[num_reduced]->t_grp = tsort[i];
	                                
	                                tsort[j]->t_next = NULL;
	                                num_reduced++;        
	                                
	                                i = j; break;
	                                }
	                        }
	                }   
	                else
	                {
	                tsort_reduced[num_reduced++] = tsort[i];
	                }
	        }

	if(num_reduced)
		{
		if(mode == TR_SORT_RVS) /* reverse of current order */
			{
			for(i=0;i<=(num_reduced/2);i++)
				{
				Trptr t_tmp = tsort_reduced[i];
								
				j = num_reduced-i-1;
				tsort_reduced[i] = tsort_reduced[j];
				tsort_reduced[j] = t_tmp;
				}			
			}
			else
			{
			if(cptr)
				{
				qsort(tsort_reduced, num_reduced, sizeof(Trptr), cptr);
				}
			}
		}

	i = 0;
	for(j=0;j<num_reduced;j++)
		{
		if(!tsort_reduced[j]->is_sort_group)
			{
			tsort[i++] = tsort_reduced[j];
			}
			else
			{
			Trptr trav = tsort_reduced[j]->t_grp;
			free_2(tsort_reduced[j]);
			while(trav)
				{
				tsort[i++] = trav;
				trav = trav->t_next;
				}
			}
		}
	}

tsort_pnt=tsort;
for(i=0;i<GLOBALS->traces.total;i++)
        {
        t=*(tsort_pnt++);

	if(!i)
		{
		GLOBALS->traces.first=t;
		t->t_prev=NULL;
		}
		else
		{
		prev->t_next=t;
		t->t_prev=prev;
		}

	prev=t;

#ifdef WAVE_HIERFIX
	if((subst=t->name))
	        while((ch=(*subst)))
        	        {
        	        if(ch==VCDNAM_HIERSORT) { *subst=GLOBALS->hier_delimeter; } /* restore hier */
        	        subst++;
        	        }
#endif
        }

GLOBALS->traces.last=prev;
prev->t_next=NULL;

return(1);
}  


Trptr GiveNextTrace(Trptr t)
{
  if(!t) return(t); /* should not happen */

  /* if(t->name) { printf("NEXT: %s %x\n", t->name, t->flags); } */
  UpdateTraceSelection(t);
  if (IsGroupBegin(t) && IsClosed(t))
    {
      Trptr next = t->t_match;
      if (next)
	return (IsCollapsed(next) ? GiveNextTrace(next) : next);
      return NULL;
    }
  else
    {
      Trptr next = t->t_next;
      if (next)
	return (IsCollapsed(next) ? GiveNextTrace(next) : next);
      return NULL;
    }
}

static Trptr GivePrevTraceSkipUpdate(Trptr t, int skip)
{
  if(!t) return(t); /* should not happen */

  /* if(t->name) { printf("PREV: %s\n", t->name); } */
  if(!skip) { UpdateTraceSelection(t); }
  if (IsGroupEnd(t) && IsClosed(t))
    {
      Trptr prev = t->t_match;
      if (prev)
	return (IsCollapsed(prev) ? GivePrevTrace(prev) : prev);
      return NULL;
    }
  else
    {
      Trptr prev = t->t_prev;
      if (prev)
	return (IsCollapsed(prev) ? GivePrevTrace(prev) : prev);
      return NULL;
    }
}

Trptr GivePrevTrace(Trptr t)
{
return(GivePrevTraceSkipUpdate(t, 0));
}


/* propogate selection info down into groups */
void UpdateTraceSelection(Trptr t)
{
  if ((t->t_match) && (IsGroupBegin(t) || IsGroupEnd(t)) && (IsSelected(t) || IsSelected(t->t_match)))
    {
      t->flags          |= TR_HIGHLIGHT;
      t->t_match->flags |= TR_HIGHLIGHT;
    }
  else
  if ((t->t_grp) && IsSelected(t->t_grp))
    {
      t->flags |= TR_HIGHLIGHT;
    }
  else
  if(t->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH))  /* seek to real xact trace if present... */
        {
	if(!(t->flags & TR_HIGHLIGHT))
		{
	        Trptr tscan = t;
	        int bcnt = 0;
	        while((tscan) && (tscan = GivePrevTraceSkipUpdate(tscan, 1)))
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
	         
	        if((tscan)&&(tscan->vector)&&(IsSelected(tscan)))
	                {
	                bvptr bv = tscan->n.vec;
	                do
	                        {
	                        bv = bv->transaction_chain; /* correlate to blank trace */
	                        } while(bv && (bcnt--));
	                if(bv)
	                        {
				t->flags |= TR_HIGHLIGHT;
	                        }
	                }
		}
        }
}


int UpdateTracesVisible(void)
{
  Trptr t = GLOBALS->traces.first;
  int cnt = 0;

  while(t)
    {
      t = GiveNextTrace(t);
      cnt++;
    }

  GLOBALS->traces.visible = cnt;
  return(cnt);
}

/* where is trace t_in in the list of displayable traces */
int GetTraceNumber(Trptr t_in)
{
  Trptr t = GLOBALS->traces.first;
  int i   = 0;
  int num = -1;

  while(t)
    {
      if (t == t_in)
	{
	  num = i;
	  break;
	}
      i++;
      t = GiveNextTrace(t);
    }

  return(num);
}

unsigned IsShadowed( Trptr t )
{

  if (t->t_grp)
    {
      if (HasWave(t->t_grp))
	{
	  return IsSelected(t->t_grp);
	}
      else
	{
	  return IsShadowed(t->t_grp);
	}
    }

  return 0;
}

char* GetFullName( Trptr t, int *was_packed )
{
  if (HasAlias(t) || !HasWave(t))
    {
      return (t->name_full);
    }
  else if (t->vector)
    {
      return (t->n.vec->bvname);
      
    }
  else
    {
      return (hier_decompress_flagged(t->n.nd->nname, was_packed));
    }
}


/*
 * sanity checking to make sure there are not any group open/close mismatches
 */
void EnsureGroupsMatch(void)
{
Trptr t = GLOBALS->traces.first;
Trptr last_good = t;
Trptr t2;
int oc_cnt = 0;
int underflow_sticky = 0;
Trptr tkill_undeflow = NULL;

while(t)
	{
	if(t->flags & TR_GRP_MASK)
		{
		if(t->flags & TR_GRP_BEGIN)
			{
			oc_cnt++;
			}
		else
		if(t->flags & TR_GRP_END)
			{
			oc_cnt--;
			if(oc_cnt == 0)
				{
				if(!underflow_sticky)
					{
					last_good = t->t_next;
					}
				}
			}

		if(oc_cnt < 0) 
			{
			if(!underflow_sticky)
				{
				tkill_undeflow = t;
				}
			underflow_sticky = 1;
			}
		}
		else
		{
		if((oc_cnt == 0) && (!underflow_sticky))
			{
			last_good = t->t_next;
			}
		}

	t = t->t_next;
	}

if((underflow_sticky) || (oc_cnt > 0))
	{
	t = last_good;
	while(t)
		{
		t2 = t->t_next;
		RemoveTrace(t, 0); /* conservatively don't set "dofree", if there is a reload memory will reclaim */
		t = t2;
		}
	}
}

/*
 * $Id$
 * $Log$
 * Revision 1.31  2010/09/10 05:58:40  gtkwave
 * structor reordering of VectorEnt to benefit 32-bit architectures
 *
 * Revision 1.30  2010/08/02 20:44:35  gtkwave
 * added gtkwave::cbTracesUpdated
 *
 * Revision 1.29  2010/07/06 16:06:09  gtkwave
 * defensive re-link fix in PasteBuffer
 *
 * Revision 1.28  2010/07/01 17:58:23  gtkwave
 * Segmentation fault on signal reordering - ID: 3023401
 *
 * Revision 1.27  2010/06/23 05:45:34  gtkwave
 * warnings fixes
 *
 * Revision 1.26  2010/04/15 00:30:23  gtkwave
 * don't propagate ttranslate filter into groups
 *
 * Revision 1.25  2010/04/13 18:11:42  gtkwave
 * propagate trace highlighting into transactions
 *
 * Revision 1.24  2010/04/07 01:50:45  gtkwave
 * improved name handling for bvname, add $next transaction operation
 *
 * Revision 1.23  2010/04/06 06:19:06  gtkwave
 * deallocate transaction trace name
 *
 * Revision 1.22  2010/04/04 19:09:57  gtkwave
 * rename name->bvname in struct BitVector for easier grep tracking
 *
 * Revision 1.21  2010/04/04 07:12:40  gtkwave
 * deallocate transaction cache on FreeTrace
 *
 * Revision 1.20  2010/03/31 16:32:20  gtkwave
 * stale marker fix for ttranslate on save file loads before GUI initialized
 *
 * Revision 1.19  2010/03/31 15:42:47  gtkwave
 * added preliminary transaction filter support
 *
 * Revision 1.18  2010/03/14 07:09:49  gtkwave
 * removed ExtNode and merged with Node
 *
 * Revision 1.17  2010/02/28 21:59:50  gtkwave
 * defensive relinking of t_prev in cut and paste buffers
 *
 * Revision 1.16  2010/02/26 18:19:00  gtkwave
 * defensive re-link of t_prev
 *
 * Revision 1.15  2010/01/23 03:21:11  gtkwave
 * hierarchy fixes when characters < "." are in the signal names
 *
 * Revision 1.14  2010/01/22 02:10:49  gtkwave
 * added second pattern search capability
 *
 * Revision 1.13  2009/12/24 20:55:27  gtkwave
 * warnings cleanups
 *
 * Revision 1.12  2009/11/05 23:11:09  gtkwave
 * added EnsureGroupsMatch()
 *
 * Revision 1.11  2009/11/03 07:08:21  gtkwave
 * enabled reverse when groups present
 *
 * Revision 1.10  2009/11/02 22:43:43  gtkwave
 * enable sorting on groups (need to do reverse yet)
 *
 * Revision 1.9  2009/11/02 05:45:14  gtkwave
 * temporarily disable sort when groups present
 *
 * Revision 1.8  2009/09/14 03:00:08  gtkwave
 * bluespec code integration
 *
 * Revision 1.7  2008/12/18 01:31:29  gtkwave
 * integrated experimental autoscroll code on signal adds
 *
 * Revision 1.6  2008/08/05 17:49:39  gtkwave
 * comment trace dnd fix
 *
 * Revision 1.5  2008/07/18 17:27:00  gtkwave
 * adding hierpack code
 *
 * Revision 1.4  2008/06/11 08:01:40  gtkwave
 * gcc 4.3.x compiler warning fixes
 *
 * Revision 1.3  2008/01/02 18:17:26  gtkwave
 * added standard click semantics with user_standard_clicking rc variable
 *
 * Revision 1.2  2007/08/26 21:35:39  gtkwave
 * integrated global context management from SystemOfCode2007 branch
 *
 * Revision 1.1.1.1.2.4  2007/08/25 19:43:45  gtkwave
 * header cleanups
 *
 * Revision 1.1.1.1.2.3  2007/08/07 03:18:54  kermin
 * Changed to pointer based GLOBAL structure and added initialization function
 *
 * Revision 1.1.1.1.2.2  2007/08/06 03:50:45  gtkwave
 * globals support for ae2, gtk1, cygwin, mingw.  also cleaned up some machine
 * generated structs, etc.
 *
 * Revision 1.1.1.1.2.1  2007/08/05 02:27:18  kermin
 * Semi working global struct
 *
 * Revision 1.1.1.1  2007/05/30 04:27:20  gtkwave
 * Imported sources
 *
 * Revision 1.4  2007/05/28 00:55:05  gtkwave
 * added support for arrays as a first class dumpfile datatype
 *
 * Revision 1.3  2007/04/29 04:13:49  gtkwave
 * changed anon union defined in struct Node to a named one as anon unions
 * are a gcc extension
 *
 * Revision 1.2  2007/04/20 02:08:11  gtkwave
 * initial release
 *
 */

