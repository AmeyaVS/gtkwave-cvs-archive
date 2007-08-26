/* 
 * Copyright (c) Tony Bybell 1999-2003.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef GTKWAVE_STRACE_H
#define GTKWAVE_STRACE_H

#include <gtk/gtk.h>
#include <string.h>
#include <stdarg.h>
#include "debug.h"
#include "analyzer.h"
#include "currenttime.h"
#include "bsearch.h"

enum strace_directions
{ STRACE_BACKWARD, STRACE_FORWARD };

enum st_stype
        {ST_DC, ST_HIGH, ST_MID, ST_X, ST_LOW, ST_STRING,
         ST_RISE, ST_FALL, ST_ANY, WAVE_STYPE_COUNT};

struct strace_defer_free
{
struct strace_defer_free *next;
Trptr defer;
};
         
struct strace_back
{
struct strace *parent;
int which;
};

struct strace
{
struct strace *next;   
char *string;           /* unmalloc this when all's done! */
Trptr trace;
char value;
char search_result;

union
	{
        hptr    h;             /* what makes up this trace */
        vptr    v;
      	} his;

struct strace_back *back[WAVE_STYPE_COUNT];    /* dealloc these too! */   
};


struct timechain
{
struct timechain *next;
TimeType t;
};


struct mprintf_buff_t
{
struct mprintf_buff_t *next;
char *str;
};

struct item_mark_string {
   char *str;
   unsigned char idx;
};

void strace_search(int direction);
void strace_maketimetrace(int mode); /* 1=create, zero=delete */

void swap_strace_contexts(void);
void delete_strace_context(void);
void cache_actual_pattern_mark_traces(void);

int mprintf(const char *fmt, ... );
void delete_mprintf(void);

#endif

/*
 * $Id$
 * $Log$
 * Revision 1.1.1.1.2.2  2007/08/25 19:43:46  gtkwave
 * header cleanups
 *
 * Revision 1.1.1.1.2.1  2007/08/05 02:27:23  kermin
 * Semi working global struct
 *
 * Revision 1.1.1.1  2007/05/30 04:27:20  gtkwave
 * Imported sources
 *
 * Revision 1.2  2007/04/20 02:08:17  gtkwave
 * initial release
 *
 */

