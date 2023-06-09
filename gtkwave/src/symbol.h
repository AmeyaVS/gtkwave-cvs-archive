/* 
 * Copyright (c) Tony Bybell 1999-2007
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_SYMBOL_H
#define WAVE_SYMBOL_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "wavealloca.h"
#include "analyzer.h"
#include "currenttime.h"
#include "tree.h"
#include "debug.h"

#define SYMPRIME 500009
#define WAVE_DECOMPRESSOR "gzip -cd "	/* zcat alone doesn't cut it for AIX */

#ifndef _MSC_VER
#include <unistd.h>
#include <inttypes.h>
#else
typedef long off_t;
#include <windows.h>
#include <io.h>
#endif

struct fac
{
int array_height;
int msb, lsb, len;
char *name;
unsigned int lastchange;
unsigned int flags;
struct Node *resolve_lxt_alias_to;
struct Node *working_node;
};


struct symbol
{
struct symbol *nextinaet;/* for aet node chaining */
struct HistEnt *h;	 /* points to previous one */

struct symbol *vec_root, *vec_chain;
	
struct symbol *next;	/* for hash chain */
char *name;
struct Node *n;

char selected;		/* for the clist object */
};


struct symchain		/* for restoring state of ->selected in signal regex search */
{
struct symchain *next;
struct symbol *symbol;
};


struct symbol *symfind(char *, unsigned int *);
struct symbol *symadd(char *, int);
struct symbol *symadd_name_exists(char *name, int hv);
int hash(char *s);

/* typically use zero for hashval as it doesn't matter if facs are sorted as symfind will bsearch... */
#define symadd_name_exists_sym_exists(s, nam, hv) \
(s)->name = (nam); \
(s)->next=sym[(hv)]; \
sym[(hv)]=(s);

void facsplit(char *, int *, int *);
int sigcmp(char *, char *);
void quicksort(struct symbol **, int, int);

void wave_heapsort(struct symbol **a, int num);

struct Bits *makevec(char *, char *);
struct Bits *makevec_annotated(char *, char *);
int maketraces(char *, int);

int parsewavline(char *, int);
int parsewavline_lx2(char *, int);

extern struct symbol **sym, **facs;
extern char facs_are_sorted;
extern int numfacs;
extern int regions;
extern struct symbol *firstnode;
extern struct symbol *curnode;
extern int longestname;
extern int hashcache;

/* additions to bitvec.c because of search.c/menu.c ==> formerly in analyzer.h */
bvptr bits2vector(struct Bits *b);
struct Bits *makevec_selected(char *vec, int numrows, char direction);
int add_vector_selected(char *alias, int numrows, char direction);
struct Bits *makevec_range(char *vec, int lo, int hi, char direction);
int add_vector_range(char *alias, int lo, int hi, char direction);
struct Bits *makevec_chain(char *vec, struct symbol *sym, int len);
int add_vector_chain(struct symbol *s, int len);
char *makename_chain(struct symbol *sym);

/* splash screen activation (version >= GTK2 only) */
void splash_create(void);
void splash_sync(off_t current, off_t total);  

#endif

/*
 * $Id$
 * $Log$
 * Revision 1.2  2007/04/20 02:08:17  gtkwave
 * initial release
 *
 */

