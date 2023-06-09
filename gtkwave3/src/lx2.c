/* 
 * Copyright (c) Tony Bybell 2003-2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <stdio.h>
#include "lx2.h"

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "symbol.h"
#include "vcd.h"
#include "lxt.h"
#include "vzt.h"
#include "lxt2_read.h"
#include "debug.h"
#include "busy.h"
#include "hierpack.h"
#include "fst.h"

/*
 * mainline
 */
TimeType lx2_main(char *fname, char *skip_start, char *skip_end)
{
int i;
struct Node *n;
struct symbol *s, *prevsymroot=NULL, *prevsym=NULL;
signed char scale;
unsigned int numalias = 0;
struct symbol *sym_block = NULL;
struct Node *node_block = NULL;
char **f_name = NULL;

GLOBALS->lx2_lx2_c_1 = lxt2_rd_init(fname);
if(!GLOBALS->lx2_lx2_c_1)
        {
	return(LLDescriptor(0));        /* look at GLOBALS->lx2_lx2_c_1 in caller for success status... */
        }

/* SPLASH */                            splash_create();

/* lxt2_rd_set_max_block_mem_usage(lx2, 0); */

scale=(signed char)lxt2_rd_get_timescale(GLOBALS->lx2_lx2_c_1);
exponent_to_time_scale(scale);

GLOBALS->numfacs=lxt2_rd_get_num_facs(GLOBALS->lx2_lx2_c_1);
GLOBALS->mvlfacs_lx2_c_1=(struct fac *)calloc_2(GLOBALS->numfacs,sizeof(struct fac));
f_name = calloc_2(F_NAME_MODULUS+1,sizeof(char *));
GLOBALS->lx2_table_lx2_c_1=(struct lx2_entry *)calloc_2(GLOBALS->numfacs, sizeof(struct lx2_entry));
sym_block = (struct symbol *)calloc_2(GLOBALS->numfacs, sizeof(struct symbol));
node_block=(struct Node *)calloc_2(GLOBALS->numfacs,sizeof(struct Node));

for(i=0;i<GLOBALS->numfacs;i++)
	{
	GLOBALS->mvlfacs_lx2_c_1[i].array_height=lxt2_rd_get_fac_rows(GLOBALS->lx2_lx2_c_1, i);
	node_block[i].msi=lxt2_rd_get_fac_msb(GLOBALS->lx2_lx2_c_1, i);
	node_block[i].lsi=lxt2_rd_get_fac_lsb(GLOBALS->lx2_lx2_c_1, i);
	GLOBALS->mvlfacs_lx2_c_1[i].flags=lxt2_rd_get_fac_flags(GLOBALS->lx2_lx2_c_1, i);
	GLOBALS->mvlfacs_lx2_c_1[i].len=lxt2_rd_get_fac_len(GLOBALS->lx2_lx2_c_1, i);
	}

fprintf(stderr, LXT2_RDLOAD"Finished building %d facs.\n", GLOBALS->numfacs);
/* SPLASH */                            splash_sync(1, 5);

GLOBALS->first_cycle_lx2_c_1 = (TimeType) lxt2_rd_get_start_time(GLOBALS->lx2_lx2_c_1) * GLOBALS->time_scale;
GLOBALS->last_cycle_lx2_c_1 = (TimeType) lxt2_rd_get_end_time(GLOBALS->lx2_lx2_c_1) * GLOBALS->time_scale;
GLOBALS->total_cycles_lx2_c_1 = GLOBALS->last_cycle_lx2_c_1 - GLOBALS->first_cycle_lx2_c_1 + 1;

/* do your stuff here..all useful info has been initialized by now */

if(!GLOBALS->hier_was_explicitly_set)    /* set default hierarchy split char */
        {
        GLOBALS->hier_delimeter='.';
        }

if(GLOBALS->numfacs)
	{
	char *fnam = lxt2_rd_get_facname(GLOBALS->lx2_lx2_c_1, 0);
	int flen = strlen(fnam);
	f_name[0]=malloc_2(flen+1);
	strcpy(f_name[0], fnam);
	}

for(i=0;i<GLOBALS->numfacs;i++)
        {
	char buf[65537];
	char *str;	
	struct fac *f;

	if(i!=(GLOBALS->numfacs-1))
		{
		char *fnam = lxt2_rd_get_facname(GLOBALS->lx2_lx2_c_1, i+1);
		int flen = strlen(fnam);
		f_name[(i+1)&F_NAME_MODULUS]=malloc_2(flen+1);
		strcpy(f_name[(i+1)&F_NAME_MODULUS], fnam);
		}

	if(i>1)
		{
		free_2(f_name[(i-2)&F_NAME_MODULUS]);
		f_name[(i-2)&F_NAME_MODULUS] = NULL;
		}

	if(GLOBALS->mvlfacs_lx2_c_1[i].flags&LXT2_RD_SYM_F_ALIAS)
		{
		int alias = GLOBALS->mvlfacs_lx2_c_1[i].array_height;
		f=GLOBALS->mvlfacs_lx2_c_1+alias;

		while(f->flags&LXT2_RD_SYM_F_ALIAS)
			{
			f=GLOBALS->mvlfacs_lx2_c_1+f->array_height;
			}

		numalias++;
		}
		else
		{
		f=GLOBALS->mvlfacs_lx2_c_1+i;
		}

	if((f->len>1)&& (!(f->flags&(LXT2_RD_SYM_F_INTEGER|LXT2_RD_SYM_F_DOUBLE|LXT2_RD_SYM_F_STRING))) )
		{
		int len = sprintf(buf, "%s[%d:%d]", f_name[(i)&F_NAME_MODULUS],node_block[i].msi, node_block[i].lsi);
		str=malloc_2(len+1);
		if(!GLOBALS->alt_hier_delimeter)
			{
			strcpy(str, buf);
			}
			else
			{
			strcpy_vcdalt(str, buf, GLOBALS->alt_hier_delimeter);
			}
                s=&sym_block[i];
                symadd_name_exists_sym_exists(s,str,0);
		prevsymroot = prevsym = NULL;
		}
		else
		{
                int gatecmp = (f->len==1) && (!(f->flags&(LXT2_RD_SYM_F_INTEGER|LXT2_RD_SYM_F_DOUBLE|LXT2_RD_SYM_F_STRING))) && (node_block[i].msi!=-1) && (node_block[i].lsi!=-1);
                int revcmp = gatecmp && (i) && (!strcmp(f_name[(i)&F_NAME_MODULUS], f_name[(i-1)&F_NAME_MODULUS]));
		
		if(gatecmp)
			{
			int len = sprintf(buf, "%s[%d]", f_name[(i)&F_NAME_MODULUS],node_block[i].msi);
			str=malloc_2(len+1);
			if(!GLOBALS->alt_hier_delimeter)
				{
				strcpy(str, buf);
				}
				else
				{
				strcpy_vcdalt(str, buf, GLOBALS->alt_hier_delimeter);
				}
	                s=&sym_block[i];
	                symadd_name_exists_sym_exists(s,str,0);
			if((prevsym)&&(revcmp)&&(!strchr(f_name[(i)&F_NAME_MODULUS], '\\')))	/* allow chaining for search functions.. */
				{
				prevsym->vec_root = prevsymroot;
				prevsym->vec_chain = s;
				s->vec_root = prevsymroot;
				prevsym = s;
				}
				else
				{
				prevsymroot = prevsym = s;
				}
			}
			else
			{
			str=malloc_2(strlen(f_name[(i)&F_NAME_MODULUS])+1);
			if(!GLOBALS->alt_hier_delimeter)
				{
				strcpy(str, f_name[(i)&F_NAME_MODULUS]);
				}
				else
				{
				strcpy_vcdalt(str, f_name[(i)&F_NAME_MODULUS], GLOBALS->alt_hier_delimeter);
				}
	                s=&sym_block[i];
	                symadd_name_exists_sym_exists(s,str,0);
			prevsymroot = prevsym = NULL;
	
			if(f->flags&LXT2_RD_SYM_F_INTEGER)
				{
				node_block[i].msi=31;
				node_block[i].lsi=0;
				GLOBALS->mvlfacs_lx2_c_1[i].len=32;
				}
			}
		}
		
	n=&node_block[i];
        n->nname=s->name;
        n->mv.mvlfac = GLOBALS->mvlfacs_lx2_c_1+i;
	GLOBALS->mvlfacs_lx2_c_1[i].working_node = n;

	if((f->len>1)||(f->flags&(LXT2_RD_SYM_F_DOUBLE|LXT2_RD_SYM_F_STRING)))
		{
		n->extvals = 1;
		}
                 
        n->head.time=-1;        /* mark 1st node as negative time */
        n->head.v.h_val=AN_X;
        s->n=n;
        }

for(i=0;i<=F_NAME_MODULUS;i++)
	{
	if(f_name[(i)&F_NAME_MODULUS])
		{
		free_2(f_name[(i)&F_NAME_MODULUS]);
		f_name[(i)&F_NAME_MODULUS] = NULL;
		}
	}
free_2(f_name); f_name = NULL;

/* SPLASH */                            splash_sync(2, 5);
GLOBALS->facs=(struct symbol **)malloc_2(GLOBALS->numfacs*sizeof(struct symbol *));

if(GLOBALS->fast_tree_sort)
        {
        for(i=0;i<GLOBALS->numfacs;i++)
                {
                int len;
                GLOBALS->facs[i]=&sym_block[i];
                if((len=strlen(GLOBALS->facs[i]->name))>GLOBALS->longestname) GLOBALS->longestname=len;
                }

	if(numalias)
		{
		unsigned int idx_lft = 0;
		unsigned int idx_lftmax = GLOBALS->numfacs - numalias;  		
		unsigned int idx_rgh = GLOBALS->numfacs - numalias;  		
		struct symbol **facs_merge=(struct symbol **)malloc_2(GLOBALS->numfacs*sizeof(struct symbol *));

		fprintf(stderr, LXT2_RDLOAD"Merging in %d aliases.\n", numalias);

		for(i=0;i<GLOBALS->numfacs;i++)	/* fix possible tail appended aliases by remerging in partial one pass merge sort */
			{
			if(strcmp(GLOBALS->facs[idx_lft]->name, GLOBALS->facs[idx_rgh]->name) <= 0)
				{
				facs_merge[i] = GLOBALS->facs[idx_lft++];

				if(idx_lft == idx_lftmax)
					{
					for(i++;i<GLOBALS->numfacs;i++)
						{
						facs_merge[i] = GLOBALS->facs[idx_rgh++];
						}
					}
				}
				else
				{
				facs_merge[i] = GLOBALS->facs[idx_rgh++];

				if(idx_rgh == GLOBALS->numfacs)
					{
					for(i++;i<GLOBALS->numfacs;i++)
						{
						facs_merge[i] = GLOBALS->facs[idx_lft++];
						}
					}
				}
			}

		free_2(GLOBALS->facs); GLOBALS->facs = facs_merge;
		}
                 
/* SPLASH */                            splash_sync(3, 5);
        fprintf(stderr, LXT2_RDLOAD"Building facility hierarchy tree.\n");
        
        init_tree();
        for(i=0;i<GLOBALS->numfacs;i++)
                {
                int esc = 0;
                char *subst = GLOBALS->facs[i]->name;
                char ch;
                
                while((ch=(*subst)))
                        {
                        if(ch==GLOBALS->hier_delimeter) { if(esc) *subst = VCDNAM_ESCAPE; }
                        else if(ch=='\\') { esc = 1; GLOBALS->escaped_names_found_vcd_c_1 = 1; }
                        subst++;
                        }
                
                build_tree_from_name(GLOBALS->facs[i]->name, i);
                }
/* SPLASH */                            splash_sync(4, 5);
	if(GLOBALS->escaped_names_found_vcd_c_1)
	        {
		for(i=0;i<GLOBALS->numfacs;i++)
			{
		        char *subst, ch;
		        subst=GLOBALS->facs[i]->name;
		        while((ch=(*subst)))
		                {
		                if(ch==VCDNAM_ESCAPE) { *subst=GLOBALS->hier_delimeter; } /* restore back to normal */
		                subst++;
		                }
			}
	        }
        treegraft(&GLOBALS->treeroot);
        
        fprintf(stderr, LXT2_RDLOAD"Sorting facility hierarchy tree.\n");
        treesort(GLOBALS->treeroot, NULL);

/* SPLASH */                            splash_sync(5, 5);
        order_facs_from_treesort(GLOBALS->treeroot, &GLOBALS->facs);
	if(GLOBALS->escaped_names_found_vcd_c_1)  
	        {
	        treenamefix(GLOBALS->treeroot);   
	        }
                 
        GLOBALS->facs_are_sorted=1;
        }
        else
	{
	for(i=0;i<GLOBALS->numfacs;i++)
		{
		char *subst, ch;
		int len;
		int esc = 0;

		GLOBALS->facs[i]=&sym_block[i];
	        if((len=strlen(subst=GLOBALS->facs[i]->name))>GLOBALS->longestname) GLOBALS->longestname=len;
		while((ch=(*subst)))
			{	
#ifdef WAVE_HIERFIX
	                if(ch==GLOBALS->hier_delimeter) { *subst=(!esc) ? VCDNAM_HIERSORT : VCDNAM_ESCAPE; }    /* forces sort at hier boundaries */
#else
	                if((ch==GLOBALS->hier_delimeter)&&(esc)) { *subst = VCDNAM_ESCAPE; }    /* forces sort at hier boundaries */
#endif
	                else if(ch=='\\') { esc = 1; GLOBALS->escaped_names_found_vcd_c_1 = 1; }
	                subst++;
			}
		}

/* SPLASH */                            splash_sync(3, 5);
	fprintf(stderr, LXT2_RDLOAD"Sorting facilities at hierarchy boundaries.\n");
	wave_heapsort(GLOBALS->facs,GLOBALS->numfacs);

#ifdef WAVE_HIERFIX	
	for(i=0;i<GLOBALS->numfacs;i++)
		{
		char *subst, ch;
	
		subst=GLOBALS->facs[i]->name;
		while((ch=(*subst)))
			{	
			if(ch==VCDNAM_HIERSORT) { *subst=GLOBALS->hier_delimeter; }	/* restore back to normal */
			subst++;
			}
		}
#endif

	GLOBALS->facs_are_sorted=1;

/* SPLASH */                            splash_sync(4, 5);
	fprintf(stderr, LXT2_RDLOAD"Building facility hierarchy tree.\n");

	init_tree();		
	for(i=0;i<GLOBALS->numfacs;i++)	
		{
		char *nf = GLOBALS->facs[i]->name;
	        build_tree_from_name(nf, i);
		}
/* SPLASH */                            splash_sync(5, 5);
	if(GLOBALS->escaped_names_found_vcd_c_1)
	        {
		for(i=0;i<GLOBALS->numfacs;i++)
			{
		        char *subst, ch;
		        subst=GLOBALS->facs[i]->name;
		        while((ch=(*subst)))
		                {
		                if(ch==VCDNAM_ESCAPE) { *subst=GLOBALS->hier_delimeter; } /* restore back to normal */
		                subst++;
		                }
			}
	        }
	treegraft(&GLOBALS->treeroot);
	treesort(GLOBALS->treeroot, NULL);
	if(GLOBALS->escaped_names_found_vcd_c_1)  
	        {
	        treenamefix(GLOBALS->treeroot);   
	        }
	}

GLOBALS->min_time = GLOBALS->first_cycle_lx2_c_1; GLOBALS->max_time=GLOBALS->last_cycle_lx2_c_1;
GLOBALS->is_lx2 = LXT2_IS_LXT2;

if(skip_start || skip_end)
	{
	TimeType b_start, b_end;

	if(!skip_start) b_start = GLOBALS->min_time; else b_start = unformat_time(skip_start, GLOBALS->time_dimension);
	if(!skip_end) b_end = GLOBALS->max_time; else b_end = unformat_time(skip_end, GLOBALS->time_dimension);

	if(b_start<GLOBALS->min_time) b_start = GLOBALS->min_time;
	else if(b_start>GLOBALS->max_time) b_start = GLOBALS->max_time;

	if(b_end<GLOBALS->min_time) b_end = GLOBALS->min_time;
	else if(b_end>GLOBALS->max_time) b_end = GLOBALS->max_time;

        if(b_start > b_end)
                {
		TimeType tmp_time = b_start;
                b_start = b_end;
                b_end = tmp_time;
                }

	if(!lxt2_rd_limit_time_range(GLOBALS->lx2_lx2_c_1, b_start, b_end))
		{
		fprintf(stderr, LXT2_RDLOAD"--begin/--end options yield zero blocks, ignoring.\n");
		lxt2_rd_unlimit_time_range(GLOBALS->lx2_lx2_c_1);
		}
		else
		{
		GLOBALS->min_time = b_start;
		GLOBALS->max_time = b_end;
		}
	}

return(GLOBALS->max_time);
}


/*
 * lx2 callback (only does bits for now)
 */
static void lx2_callback(struct lxt2_rd_trace **lt, lxtint64_t *tim, lxtint32_t *facidx, char **value)
{
struct HistEnt *htemp = histent_calloc();
struct lx2_entry *l2e = GLOBALS->lx2_table_lx2_c_1+(*facidx);
struct fac *f = GLOBALS->mvlfacs_lx2_c_1+(*facidx);


GLOBALS->busycnt_lx2_c_1++;
if(GLOBALS->busycnt_lx2_c_1==WAVE_BUSY_ITER)
        {
        busy_window_refresh();
        GLOBALS->busycnt_lx2_c_1 = 0;
        }

/* fprintf(stderr, "%lld %d %s\n", *tim, *facidx, *value); */

if(!(f->flags&(LXT2_RD_SYM_F_DOUBLE|LXT2_RD_SYM_F_STRING)))
	{
	if(f->len>1)        
	        {
	        htemp->v.h_vector = (char *)malloc_2(f->len);
		memcpy(htemp->v.h_vector, *value, f->len);
	        }
	        else
	        {
		switch(**value)
			{
			case '0':	htemp->v.h_val = AN_0; break;
			case '1':	htemp->v.h_val = AN_1; break;
			case 'Z':
			case 'z':	htemp->v.h_val = AN_Z; break;
			default:	htemp->v.h_val = AN_X; break;
			}
	        }
	}
else if(f->flags&LXT2_RD_SYM_F_DOUBLE)
	{
	double *d = malloc_2(sizeof(double));
	sscanf(*value, "%lg", d);
	htemp->v.h_vector = (char *)d;
	htemp->flags = HIST_REAL;
	}
else	/* string */
	{
	char *s = malloc_2(strlen(*value)+1);
	strcpy(s, *value);
	htemp->v.h_vector = s;
	htemp->flags = HIST_REAL|HIST_STRING;
	}


htemp->time = (*tim) * (GLOBALS->time_scale);

if(l2e->histent_head)
	{
	l2e->histent_curr->next = htemp;
	l2e->histent_curr = htemp;
	}
	else
	{
	l2e->histent_head = l2e->histent_curr = htemp;
	}

l2e->numtrans++;
}


/*
 * this is the black magic that handles aliased signals...
 */
static void lx2_resolver(nptr np, nptr resolve)
{ 
np->extvals = resolve->extvals;
np->msi = resolve->msi;
np->lsi = resolve->lsi;
memcpy(&np->head, &resolve->head, sizeof(struct HistEnt));
np->curr = resolve->curr;
np->harray = resolve->harray;
np->numhist = resolve->numhist;
np->mv.mvlfac=NULL;
}



/* 
 * actually import an lx2 trace but don't do it if it's already been imported 
 */
void import_lx2_trace(nptr np)
{
struct HistEnt *htemp, *histent_tail;
int len, i;
struct fac *f;
int txidx;
nptr nold = np;

switch(GLOBALS->is_lx2)
	{
#ifdef AET2_IS_PRESENT
	case LXT2_IS_AET2: 	import_ae2_trace(np); return;
#endif
	case LXT2_IS_VZT:  	import_vzt_trace(np); return;
	case LXT2_IS_VLIST: 	import_vcd_trace(np); return;
	case LXT2_IS_FST: 	import_fst_trace(np); return;
	default: break; /* fallthrough */
	}

if(!(f=np->mv.mvlfac)) return;	/* already imported */

txidx = f - GLOBALS->mvlfacs_lx2_c_1;
if(np->mv.mvlfac->flags&LXT2_RD_SYM_F_ALIAS) 
	{
	txidx = lxt2_rd_get_alias_root(GLOBALS->lx2_lx2_c_1, txidx);
	np = GLOBALS->mvlfacs_lx2_c_1[txidx].working_node;

	if(!(f=np->mv.mvlfac)) 
		{
		lx2_resolver(nold, np);
		return;	/* already imported */
		}
	}

fprintf(stderr, "Import: %s\n", np->nname);

/* new stuff */
len = np->mv.mvlfac->len;

if(f->array_height <= 1) /* sorry, arrays not supported, but lx2 doesn't support them yet either */
	{
	lxt2_rd_set_fac_process_mask(GLOBALS->lx2_lx2_c_1, txidx);
	lxt2_rd_iter_blocks(GLOBALS->lx2_lx2_c_1, lx2_callback, NULL);
	lxt2_rd_clr_fac_process_mask(GLOBALS->lx2_lx2_c_1, txidx);
	}

histent_tail = htemp = histent_calloc();
if(len>1)
	{
	htemp->v.h_vector = (char *)malloc_2(len);
	for(i=0;i<len;i++) htemp->v.h_vector[i] = AN_Z;
	}
	else
	{
	htemp->v.h_val = AN_Z;		/* z */
	}
htemp->time = MAX_HISTENT_TIME;

htemp = histent_calloc();
if(len>1)
	{
	htemp->v.h_vector = (char *)malloc_2(len);
	for(i=0;i<len;i++) htemp->v.h_vector[i] = AN_X;
	}
	else
	{
	htemp->v.h_val = AN_X;		/* x */
	}
htemp->time = MAX_HISTENT_TIME-1;
htemp->next = histent_tail;			

if(GLOBALS->lx2_table_lx2_c_1[txidx].histent_curr)
	{
	GLOBALS->lx2_table_lx2_c_1[txidx].histent_curr->next = htemp;
	htemp = GLOBALS->lx2_table_lx2_c_1[txidx].histent_head;
	}

if(!(f->flags&(LXT2_RD_SYM_F_DOUBLE|LXT2_RD_SYM_F_STRING)))
        {
	if(len>1)
		{
		np->head.v.h_vector = (char *)malloc_2(len);
		for(i=0;i<len;i++) np->head.v.h_vector[i] = AN_X;
		}
		else
		{
		np->head.v.h_val = AN_X;	/* x */
		}
	}
        else
        {
        np->head.flags = HIST_REAL;
        if(f->flags&LXT2_RD_SYM_F_STRING) np->head.flags |= HIST_STRING;
        }

        {
        struct HistEnt *htemp2 = calloc_2(1, sizeof(struct HistEnt));
        htemp2->time = -1;
        if(len>1)
                {
                htemp2->v.h_vector = htemp->v.h_vector;
                }
                else
                {
                htemp2->v.h_val = htemp->v.h_val;
                }   
        htemp2->next = htemp;
        htemp = htemp2;
	GLOBALS->lx2_table_lx2_c_1[txidx].numtrans++;
        }

np->head.time  = -2;
np->head.next = htemp;
np->numhist=GLOBALS->lx2_table_lx2_c_1[txidx].numtrans +2 /*endcap*/ +1 /*frontcap*/;

memset(GLOBALS->lx2_table_lx2_c_1+txidx, 0, sizeof(struct lx2_entry));	/* zero it out */

np->curr = histent_tail;
np->mv.mvlfac = NULL;	/* it's imported and cached so we can forget it's an mvlfac now */

if(nold!=np)
	{
	lx2_resolver(nold, np);
	}
}


/* 
 * pre-import many traces at once so function above doesn't have to iterate...
 */
void lx2_set_fac_process_mask(nptr np)
{
struct fac *f;
int txidx;

switch(GLOBALS->is_lx2)
        {
#ifdef AET2_IS_PRESENT
        case LXT2_IS_AET2: 	ae2_set_fac_process_mask(np); return;
#endif
        case LXT2_IS_VZT:  	vzt_set_fac_process_mask(np); return;
	case LXT2_IS_VLIST: 	vcd_set_fac_process_mask(np); return;
	case LXT2_IS_FST: 	fst_set_fac_process_mask(np); return;
        default: break; /* fallthrough */
        }

if(!(f=np->mv.mvlfac)) return;	/* already imported */

txidx = f-GLOBALS->mvlfacs_lx2_c_1;

if(np->mv.mvlfac->flags&LXT2_RD_SYM_F_ALIAS) 
	{
	txidx = lxt2_rd_get_alias_root(GLOBALS->lx2_lx2_c_1, txidx);
	np = GLOBALS->mvlfacs_lx2_c_1[txidx].working_node;

	if(!(np->mv.mvlfac)) return;	/* already imported */
	}

if(np->mv.mvlfac->array_height <= 1) /* sorry, arrays not supported, but lx2 doesn't support them yet either */
	{
	lxt2_rd_set_fac_process_mask(GLOBALS->lx2_lx2_c_1, txidx);
	GLOBALS->lx2_table_lx2_c_1[txidx].np = np;
	}
}


void lx2_import_masked(void)
{
int txidx, i, cnt;

switch(GLOBALS->is_lx2)
        {
#ifdef AET2_IS_PRESENT
        case LXT2_IS_AET2: 	ae2_import_masked(); return;
#endif
        case LXT2_IS_VZT:  	vzt_import_masked(); return;
        case LXT2_IS_VLIST:  	vcd_import_masked(); return;
        case LXT2_IS_FST:  	fst_import_masked(); return;
        default: break; /* fallthrough */
        }

cnt = 0;
for(txidx=0;txidx<GLOBALS->numfacs;txidx++)
	{
	if(lxt2_rd_get_fac_process_mask(GLOBALS->lx2_lx2_c_1, txidx))
		{
		cnt++;
		}
	}

if(!cnt) return;

if(cnt>100)
	{
	fprintf(stderr, LXT2_RDLOAD"Extracting %d traces\n", cnt);
	}


set_window_busy(NULL);
lxt2_rd_iter_blocks(GLOBALS->lx2_lx2_c_1, lx2_callback, NULL);
set_window_idle(NULL);

for(txidx=0;txidx<GLOBALS->numfacs;txidx++)
	{
	if(lxt2_rd_get_fac_process_mask(GLOBALS->lx2_lx2_c_1, txidx))
		{
		struct HistEnt *htemp, *histent_tail;
		struct fac *f = GLOBALS->mvlfacs_lx2_c_1+txidx;
		int len = f->len;
		nptr np = GLOBALS->lx2_table_lx2_c_1[txidx].np;

		histent_tail = htemp = histent_calloc();
		if(len>1)
			{
			htemp->v.h_vector = (char *)malloc_2(len);
			for(i=0;i<len;i++) htemp->v.h_vector[i] = AN_Z;
			}
			else
			{
			htemp->v.h_val = AN_Z;		/* z */
			}
		htemp->time = MAX_HISTENT_TIME;
			
		htemp = histent_calloc();
		if(len>1)
			{
			htemp->v.h_vector = (char *)malloc_2(len);
			for(i=0;i<len;i++) htemp->v.h_vector[i] = AN_X;
			}
			else
			{
			htemp->v.h_val = AN_X;		/* x */
			}
		htemp->time = MAX_HISTENT_TIME-1;
		htemp->next = histent_tail;			

		if(GLOBALS->lx2_table_lx2_c_1[txidx].histent_curr)
			{
			GLOBALS->lx2_table_lx2_c_1[txidx].histent_curr->next = htemp;
			htemp = GLOBALS->lx2_table_lx2_c_1[txidx].histent_head;
			}

		if(!(f->flags&(LXT2_RD_SYM_F_DOUBLE|LXT2_RD_SYM_F_STRING)))
		        {
			if(len>1)
				{
				np->head.v.h_vector = (char *)malloc_2(len);
				for(i=0;i<len;i++) np->head.v.h_vector[i] = AN_X;
				}
				else
				{
				np->head.v.h_val = AN_X;	/* x */
				}
			}
		        else
		        {
		        np->head.flags = HIST_REAL;
		        if(f->flags&LXT2_RD_SYM_F_STRING) np->head.flags |= HIST_STRING;
		        }

	        {
	        struct HistEnt *htemp2 = calloc_2(1, sizeof(struct HistEnt));
	        htemp2->time = -1;
	        if(len>1)
	                {
	                htemp2->v.h_vector = htemp->v.h_vector;
	                }
	                else
	                {
	                htemp2->v.h_val = htemp->v.h_val;
	                }   
	        htemp2->next = htemp;
	        htemp = htemp2;
	        GLOBALS->lx2_table_lx2_c_1[txidx].numtrans++;
	        }

		np->head.time  = -2;
		np->head.next = htemp;
		np->numhist=GLOBALS->lx2_table_lx2_c_1[txidx].numtrans +2 /*endcap*/ +1 /*frontcap*/;

		memset(GLOBALS->lx2_table_lx2_c_1+txidx, 0, sizeof(struct lx2_entry));	/* zero it out */

		np->curr = histent_tail;
		np->mv.mvlfac = NULL;	/* it's imported and cached so we can forget it's an mvlfac now */
		lxt2_rd_clr_fac_process_mask(GLOBALS->lx2_lx2_c_1, txidx);
		}
	}
}

/*
 * $Id$
 * $Log$
 * Revision 1.21  2011/01/21 15:49:52  gtkwave
 * simplify strand coalesce routine and make same as FST's
 *
 * Revision 1.20  2011/01/13 17:20:39  gtkwave
 * rewrote hierarchy / facility packing code
 *
 * Revision 1.19  2011/01/07 20:17:10  gtkwave
 * remove redundant fields from struct fac
 *
 * Revision 1.18  2010/09/15 18:35:42  gtkwave
 * added F_NAME_MODULUS to reduce temp memory usage
 *
 * Revision 1.17  2010/06/02 03:51:30  gtkwave
 * don't autocoalesce escape identifiers
 *
 * Revision 1.16  2010/03/14 07:09:49  gtkwave
 * removed ExtNode and merged with Node
 *
 * Revision 1.15  2010/03/13 21:38:16  gtkwave
 * fixed && used in logical operations for allocating ExtNode
 *
 * Revision 1.14  2010/03/13 19:16:00  gtkwave
 * removal of useless symbol->nextinaet code
 *
 * Revision 1.13  2010/03/11 23:31:52  gtkwave
 * remove name field from struct fac
 *
 * Revision 1.12  2010/03/01 05:16:26  gtkwave
 * move compressed hier tree traversal to hierpack
 *
 * Revision 1.11  2010/01/23 03:21:11  gtkwave
 * hierarchy fixes when characters < "." are in the signal names
 *
 * Revision 1.10  2009/08/06 20:03:31  gtkwave
 * warnings fixes
 *
 * Revision 1.9  2009/07/01 07:39:12  gtkwave
 * decorating hierarchy tree with module type info
 *
 * Revision 1.8  2009/06/07 08:40:44  gtkwave
 * adding FST support
 *
 * Revision 1.7  2009/04/30 03:24:40  gtkwave
 * added VCDNAM_ESCAPE cases to loader
 *
 * Revision 1.6  2009/03/31 18:49:49  gtkwave
 * removal of warnings under cygwin compile
 *
 * Revision 1.5  2008/12/25 03:28:55  gtkwave
 * -Wshadow warning fixes
 *
 * Revision 1.4  2008/07/20 20:48:08  gtkwave
 * compressed hierarchy support
 *
 * Revision 1.3  2008/06/15 21:26:45  gtkwave
 * time -1 endcap add for LXT2 + VZT loaders
 *
 * Revision 1.2  2007/08/26 21:35:41  gtkwave
 * integrated global context management from SystemOfCode2007 branch
 *
 * Revision 1.1.1.1.2.4  2007/08/23 23:28:48  gtkwave
 * reload fail handling and retries
 *
 * Revision 1.1.1.1.2.3  2007/08/07 03:18:54  kermin
 * Changed to pointer based GLOBAL structure and added initialization function
 *
 * Revision 1.1.1.1.2.2  2007/08/06 03:50:47  gtkwave
 * globals support for ae2, gtk1, cygwin, mingw.  also cleaned up some machine
 * generated structs, etc.
 *
 * Revision 1.1.1.1.2.1  2007/08/05 02:27:20  kermin
 * Semi working global struct
 *
 * Revision 1.1.1.1  2007/05/30 04:27:37  gtkwave
 * Imported sources
 *
 * Revision 1.3  2007/04/29 04:13:49  gtkwave
 * changed anon union defined in struct Node to a named one as anon unions
 * are a gcc extension
 *
 * Revision 1.2  2007/04/20 02:08:13  gtkwave
 * initial release
 *
 */

