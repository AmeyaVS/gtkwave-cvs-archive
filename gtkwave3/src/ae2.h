/* 
 * Copyright (c) Tony Bybell 2004.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_AE2RDR_H
#define WAVE_AE2RDR_H

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include "vcd.h"
#include "lx2.h"

#define AET2_RDLOAD "AE2LOAD | "
#define AET2_TIMEFAC "AE2(!!TIME!!)"
#define AET2_PRECFAC "AE2(!!PREC!!)"

struct ae2_ncycle_autosort
{
struct ae2_ncycle_autosort *next;
};

struct regex_links
{
struct regex_links *next;
void *pnt;
};


TimeType 	ae2_main(char *fname, char *skip_start, char *skip_end, char *indirect_fname);
void 		import_ae2_trace(nptr np);
void 		ae2_set_fac_process_mask(nptr np);
void 		ae2_import_masked(void);


#ifndef _MSC_VER
/*
 * texsim prototypes/libae2rw interfacing...
 */
#define	MAXFACLEN 	 65536
typedef unsigned long	 FACIDX;

struct facref
{
FACIDX s;                         /* symbol table key */
char *facname;                    /* ptr to facility name */
int offset;                       /* offset of reference */ 
int length;                       /* length of reference */  
unsigned int row;                 /* row number for arrays */
unsigned int row_high;            /* row number for arrays */
};
typedef struct facref FACREF;

typedef void* 	AE2_HANDLE;
typedef unsigned long 	AE2_SYMBOL;
typedef void 	(*AE2_SEVERROR) (const char*, ...);
typedef void 	(*AE2_MSG) (int, const char*, ...);
typedef void* 	(*AE2_ALLOC) (size_t size);
typedef void 	(*AE2_FREE) (void* ptr, size_t size);

void 		ae2_initialize(AE2_SEVERROR error_fn, AE2_MSG msg_fn, AE2_ALLOC alloc_fn, AE2_FREE free_fn);
AE2_HANDLE 	ae2_read_initialize(FILE* file);
uint64_t 	ae2_read_start_cycle(AE2_HANDLE handle);
uint64_t 	ae2_read_end_cycle(AE2_HANDLE handle);  
unsigned long   ae2_read_find_symbol(AE2_HANDLE handle, const char* name, FACREF* fr);
unsigned long 	ae2_read_num_sections(AE2_HANDLE handle);
uint64_t* 	ae2_read_ith_section_range(AE2_HANDLE handle, unsigned long section_idx);
time_t          ae2_read_model_timestamp(AE2_HANDLE handle);
unsigned long	ae2_read_num_symbols(AE2_HANDLE handle);
unsigned long 	ae2_read_symbol_name(AE2_HANDLE handle, unsigned long symbol_idx, char* name);
unsigned long	ae2_read_symbol_rows(AE2_HANDLE handle, unsigned long symbol_idx);
unsigned long	ae2_read_symbol_length(AE2_HANDLE handle, unsigned long symbol_idx);
unsigned long	ae2_read_symbol_sparse_flag(AE2_HANDLE handle, unsigned long symbol_idx);
unsigned long 	ae2_read_value(AE2_HANDLE handle, FACREF* fr, uint64_t cycle, char* value);
uint64_t 	ae2_read_next_value(AE2_HANDLE handle, FACREF* fr, uint64_t cycle, char* value);
void 		ae2_read_end(AE2_HANDLE handle);

unsigned long	ae2_read_num_sparse_rows(AE2_HANDLE handle, unsigned long symbol_idx, uint64_t cycle);
uint64_t 	ae2_read_ith_sparse_row(AE2_HANDLE handle, unsigned long symbol_idx, uint64_t cycle, unsigned long idx);

#endif

#endif

/*
 * $Id$
 * $Log$
 * Revision 1.7  2010/04/27 23:10:56  gtkwave
 * made inttype.h inclusion conditional
 *
 * Revision 1.6  2008/09/27 19:08:39  gtkwave
 * compiler warning fixes
 *
 * Revision 1.5  2008/02/19 22:00:28  gtkwave
 * added aetinfo support
 *
 * Revision 1.4  2007/08/31 22:42:43  gtkwave
 * 3.1.0 RC3 updates
 *
 * Revision 1.3  2007/08/26 21:35:39  gtkwave
 * integrated global context management from SystemOfCode2007 branch
 *
 * Revision 1.1.1.1.2.1  2007/07/28 19:50:39  kermin
 * Merged in the main line
 *
 * Revision 1.2  2007/06/23 02:37:27  gtkwave
 * static section size is now dynamic
 *
 * Revision 1.1.1.1  2007/05/30 04:27:30  gtkwave
 * Imported sources
 *
 * Revision 1.2  2007/04/20 01:39:00  gtkwave
 * initial release
 *
 */
