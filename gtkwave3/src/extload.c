/* 
 * Copyright (c) Tony Bybell 2009-2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include <stdio.h>
#include "vzt.h"
#include "lx2.h"

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "symbol.h"
#include "vcd.h"
#include "lxt2_read.h"
#include "vzt_read.h"
#include "lxt.h"
#include "extload.h"
#include "debug.h"
#include "busy.h"
#include "hierpack.h"

#ifndef EXTLOAD_SUFFIX

const char *extload_loader_fail_msg = "Sorry, EXTLOAD support was not compiled into this executable, exiting.\n\n";

TimeType extload_main(char *fname, char *skip_start, char *skip_end)
{
fprintf(stderr, "%s", extload_loader_fail_msg);
exit(255);

return(0); /* for vc++ */
}

void import_extload_trace(nptr np)
{
fprintf(stderr, "%s", extload_loader_fail_msg);
exit(255);
}

#else

static int last_modification_check(void)
{
#if !defined __MINGW32__ && !defined _MSC_VER
struct stat buf;
int rc;

errno = 0;
rc = stat(GLOBALS->loaded_file_name, &buf);

if(GLOBALS->extload_lastmod)
	{
	if(GLOBALS->extload_already_errored)
		{
		return(0);
		}
	else
	if(rc != 0)
		{
		fprintf(stderr, EXTLOAD"stat error on '%s'\n", GLOBALS->loaded_file_name);
		perror("Why");
		errno = 0;
		GLOBALS->extload_already_errored = 1;
		return(0);		
		}
	else
	if(GLOBALS->extload_lastmod != buf.st_mtime)
		{
		fprintf(stderr, EXTLOAD"file '%s' was modified!\n", GLOBALS->loaded_file_name);
		GLOBALS->extload_already_errored = 1;
		return(0);		
		}
		else
		{
		return(1);
		}
	}
	else
	{
	GLOBALS->extload_lastmod = buf.st_mtime;
	return(1);
	}

#else
return(1);
#endif
}


static char *get_varname(void)
{
static char sbuff[65537];
char * rc;

for(;;)
	{
	rc = fgets(sbuff, 65536, GLOBALS->extload);
	if(!rc)
		{
		return(NULL);
		}

        if(rc[0] == 'V')
                {
                if(!strncmp("Var: ", rc, 5))
			{
			char *pnt = rc + 5;
			char *cpyto = sbuff;

			while(*pnt)
				{
				if(!isspace(*pnt))
					{
					pnt++;
					}
					else
					{
					break;
					}
				}
			while(*pnt)
				{
				if(isspace(*pnt))
					{
					pnt++;
					}
					else
					{
					break;
					}
				}
			if(*pnt)
				{
				while(*pnt)
					{
					if((*pnt == '[')||(isspace(*pnt))) break;
					if(*pnt == '\\') /* this is not strictly correct, but fixes generic ranges from icarus */
						{
						pnt++;
						continue;
						}
					*(cpyto++) = *(pnt++);
					}
				*cpyto = 0;
				return(sbuff);
				}
			}
		}
	}
}

/*
 * mainline
 */
TimeType extload_main(char *fname, char *skip_start, char *skip_end)
{
char sbuff[65537];
int max_idcode;
unsigned int msk = 0;

int i;
struct Node *n;
struct symbol *s, *prevsymroot=NULL, *prevsym=NULL;
struct symbol *sym_block = NULL;
struct Node *node_block = NULL;
char **namecache = NULL;

if(!(GLOBALS->extload=fopen(fname, "rb")))
	{
	GLOBALS->extload_already_errored = 1;
	return(LLDescriptor(0)); 	/* look at GLOBALS->vzt_vzt_c_1 in caller for success status... */
	}
fclose(GLOBALS->extload);

/* SPLASH */                            splash_create();

last_modification_check();
sprintf(sbuff, "%s -info %s 2>&1", EXTLOAD_PATH, fname);
GLOBALS->extload = popen(sbuff, "r");
for(;;)
	{
	char * rc = fgets(sbuff, 65536, GLOBALS->extload);
	if(!rc) break;

	switch(rc[0])
		{
		case 's':
			if(!strncmp("scale unit         :", rc, 20))
				{
				char *pnt = rc+20;

				GLOBALS->time_scale = atoi(rc+20);
				GLOBALS->time_dimension = 'n';
				while(*pnt)
					{
					if(isalpha(*pnt))
						{
						GLOBALS->time_dimension = tolower(*pnt);
						break;
						}
					pnt++;
					}
				
				msk |= 1;
				}
			break;

		case 'm':
			if(!strncmp("minimum xtag       : (", rc, 22))
				{
				unsigned int lo = 0, hi = 0;
				sscanf(rc + 22, "%d %d", &hi, &lo);
				GLOBALS->min_time = (TimeType)((((UTimeType)hi)<<32) + ((UTimeType)lo));

				msk |= 2;
				}
			else
			if(!strncmp("maximum xtag       : (", rc, 22))
				{
				unsigned int lo = 0, hi = 0;
				sscanf(rc + 22, "%d %d", &hi, &lo);
				GLOBALS->max_time = (TimeType)((((UTimeType)hi)<<32) + ((UTimeType)lo));

				msk |= 4;
				}
			else
			if(!strncmp("max var idcode     :", rc, 20))
				{
				sscanf(rc + 21, "%d", &max_idcode);

				msk |= 8;
				}

			break;

		case 'v':
			if(!strncmp("var creation cnt   :", rc, 20))
				{
				sscanf(rc + 21, "%d", &GLOBALS->numfacs);

				msk |= 16;
				}

		case 'f':
			if(!strncmp("file status        : finished", rc, 29))
				{
				msk |= 32;
				}
			break;

		default:
			break;
		}
	}
pclose(GLOBALS->extload);

if(msk != (1+2+4+8+16+32))
	{
	fprintf(stderr, EXTLOAD"Could not initialize '%s' properly.\n", fname);
	if((msk & (1+2+4+8+16+32)) == (1+2+4+8+16))
		{
		fprintf(stderr, EXTLOAD"File is not finished dumping.\n");
		}
	GLOBALS->extload_already_errored = 1;
	return(LLDescriptor(0));
	}

GLOBALS->min_time *= GLOBALS->time_scale;
GLOBALS->max_time *= GLOBALS->time_scale;

GLOBALS->mvlfacs_vzt_c_3=(struct fac *)calloc_2(GLOBALS->numfacs,sizeof(struct fac));
GLOBALS->vzt_table_vzt_c_1=(struct lx2_entry *)calloc_2(GLOBALS->numfacs, sizeof(struct lx2_entry));
namecache=(char **)calloc_2(GLOBALS->numfacs, sizeof(char *));
sym_block = (struct symbol *)calloc_2(GLOBALS->numfacs, sizeof(struct symbol));
node_block=(struct Node *)calloc_2(GLOBALS->numfacs,sizeof(struct Node));
GLOBALS->extload_idcodes=(unsigned int *)calloc_2(GLOBALS->numfacs, sizeof(unsigned int));
GLOBALS->extload_inv_idcodes=(int *)calloc_2(max_idcode+1, sizeof(int));

if(!last_modification_check()) { GLOBALS->extload_already_errored = 1; return(LLDescriptor(0)); }
sprintf(sbuff, "%s -hier_tree %s 2>&1", EXTLOAD_PATH, fname);
GLOBALS->extload = popen(sbuff, "r");
i = 0;
for(;;)
	{
	char * rc = fgets(sbuff, 65536, GLOBALS->extload);
	if(!rc) break;

	if(rc[0] == 'V')
		{
		if(!strncmp("Var:", rc, 4))
			{
			char *pnt = rc + 5;
			char *last_l = NULL;
			char typ[64];
			char *esc = NULL;
			char *lb = NULL;
			char *colon = NULL;
			char *rb = NULL;
			int state = 0;

			sscanf(rc + 5, "%s", typ);

			while(*pnt)
				{
				if((pnt[0] == 'l') && (pnt[1] == ':'))
					{
					last_l = pnt;
					}
				else if(pnt[0] == '\\')
					{
					esc = pnt;
					}
				else if(pnt[0] == '[')
					{
					lb = pnt;
					colon = NULL;
					state = 1;
					}
				else if(pnt[0] == ']')
					{
					rb = pnt;
					state = 0;
					}
				else if(pnt[0] == ':')
					{
					if(state)
						{
						colon = pnt;
						}
					}
				
				pnt++;
				}

			if(last_l)
				{
				unsigned int l, r;
				char s1[32], s3[32], s4[32];
				unsigned int d2;
				sscanf(last_l, "l:%d r:%d %s %d %s %s", &l, &r, s1, &d2, s3, s4);

				GLOBALS->extload_idcodes[i] = d2;
				if(GLOBALS->extload_inv_idcodes[d2] == 0) GLOBALS->extload_inv_idcodes[d2] = i+1; /* root alias */

				GLOBALS->mvlfacs_vzt_c_3[i].array_height=0;

				if(!strcmp("vcd_real", typ))
					{
					GLOBALS->mvlfacs_vzt_c_3[i].flags = VZT_RD_SYM_F_DOUBLE;
					node_block[i].msi=0;				
					node_block[i].lsi=0;				
					GLOBALS->mvlfacs_vzt_c_3[i].len=64;
					}
				else
				if(!strcmp("vcd_integer", typ))
					{
					GLOBALS->mvlfacs_vzt_c_3[i].flags = VZT_RD_SYM_F_INTEGER;
					node_block[i].msi=0;				
					node_block[i].lsi=0;				
					GLOBALS->mvlfacs_vzt_c_3[i].len=32;
					}
				else
					{
					int len_parse = 1;

					GLOBALS->mvlfacs_vzt_c_3[i].len=(l>r) ? (l-r+1) : (r-l+1);

					if(esc && lb && rb)
						{
						node_block[i].msi = atoi(lb+1);
						if(colon)
							{
							node_block[i].lsi = atoi(colon+1);
							}
							else
							{
							node_block[i].lsi = node_block[i].msi;
							}						

						len_parse = (node_block[i].msi > node_block[i].lsi)
								? (node_block[i].msi - node_block[i].lsi + 1)
								: (node_block[i].lsi - node_block[i].msi + 1);

						if(len_parse != GLOBALS->mvlfacs_vzt_c_3[i].len)
							{
							node_block[i].msi=l;				
							node_block[i].lsi=r;				
							}
						}
						else
						{
						node_block[i].msi=l;				
						node_block[i].lsi=r;				
						}

					GLOBALS->mvlfacs_vzt_c_3[i].flags = VZT_RD_SYM_F_BITS; 
					}
				}

			i++;
			}
		}

	}

pclose(GLOBALS->extload);

if(i==GLOBALS->numfacs)
	{
	fprintf(stderr, EXTLOAD"Finished building %d facs.\n", GLOBALS->numfacs);
	}
	else
	{
	fprintf(stderr, EXTLOAD"Fac count mismatch: %d expected vs %d found, exiting.\n", GLOBALS->numfacs, i);
	GLOBALS->extload_already_errored = 1;
	return(LLDescriptor(0));
	}
/* SPLASH */                            splash_sync(1, 5);

if(!last_modification_check()) { GLOBALS->extload_already_errored = 1; return(LLDescriptor(0)); }
sprintf(sbuff, "%s -hier_tree %s 2>&1", EXTLOAD_PATH, fname);
GLOBALS->extload = popen(sbuff, "r");

/* do your stuff here..all useful info has been initialized by now */

if(!GLOBALS->hier_was_explicitly_set)    /* set default hierarchy split char */
        {
        GLOBALS->hier_delimeter='.';
        }

if(GLOBALS->numfacs)
	{
	char *fnam = get_varname();
	int flen = strlen(fnam);
	namecache[0]=malloc_2(flen+1);
	strcpy(namecache[0], fnam);
	}

for(i=0;i<GLOBALS->numfacs;i++)
        {
	char buf[65537];
	char *str;	
	struct fac *f;

	if(i!=(GLOBALS->numfacs-1))
		{
		char *fnam = get_varname();
		int flen = strlen(fnam);
		namecache[i+1]=malloc_2(flen+1);
		strcpy(namecache[i+1], fnam);
		}

	if(i>1)
		{
		free_2(namecache[i-2]);
		namecache[i-2] = NULL;
		}

	f=GLOBALS->mvlfacs_vzt_c_3+i;

	if((f->len>1)&& (!(f->flags&(VZT_RD_SYM_F_INTEGER|VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING))) )
		{
		int len=sprintf(buf, "%s[%d:%d]", namecache[i],node_block[i].msi, node_block[i].lsi);
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
	else if ( 
			((f->len==1)&&(!(f->flags&(VZT_RD_SYM_F_INTEGER|VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))&&
			((i!=GLOBALS->numfacs-1)&&(!strcmp(namecache[i], namecache[i+1]))))
			||
			(((i!=0)&&(!strcmp(namecache[i], namecache[i-1]))) &&
			(node_block[i].msi!=-1)&&(node_block[i].lsi!=-1))
		)
		{
		int len = sprintf(buf, "%s[%d]", namecache[i],node_block[i].msi);
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
		if((prevsym)&&(i>0)&&(!strcmp(namecache[i], namecache[i-1])))	/* allow chaining for search functions.. */
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
		str=malloc_2(strlen(namecache[i])+1);
		if(!GLOBALS->alt_hier_delimeter)
			{
			strcpy(str, namecache[i]);
			}
			else
			{
			strcpy_vcdalt(str, namecache[i], GLOBALS->alt_hier_delimeter);
			}
		s=&sym_block[i];
	        symadd_name_exists_sym_exists(s,str,0);
		prevsymroot = prevsym = NULL;

		if(f->flags&VZT_RD_SYM_F_INTEGER)
			{
			node_block[i].msi=31;
			node_block[i].lsi=0;
			GLOBALS->mvlfacs_vzt_c_3[i].len=32;
			}
		}
		
        n=&node_block[i];
        n->nname=s->name;
        n->mv.mvlfac = GLOBALS->mvlfacs_vzt_c_3+i;
	GLOBALS->mvlfacs_vzt_c_3[i].working_node = n;

	if((f->len>1)||(f->flags&(VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))
		{
		n->extvals = 1;
		}
                 
        n->head.time=-1;        /* mark 1st node as negative time */
        n->head.v.h_val=AN_X;
        s->n=n;
        }

for(i=0;((i<2)&&(i<GLOBALS->numfacs));i++)
	{
	if(namecache[i])
		{
		free_2(namecache[i]);
		namecache[i] = NULL;
		}
	}
free_2(namecache); namecache = NULL;
pclose(GLOBALS->extload);

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
                                
/* SPLASH */                            splash_sync(3, 5);  
        fprintf(stderr, EXTLOAD"Building facility hierarchy tree.\n");
                                         
        init_tree();
        for(i=0;i<GLOBALS->numfacs;i++)
                {
		build_tree_from_name(GLOBALS->facs[i]->name, i);
                }
/* SPLASH */                            splash_sync(4, 5);  
        treegraft(&GLOBALS->treeroot);
                                
        fprintf(stderr, EXTLOAD"Sorting facility hierarchy tree.\n");
        treesort(GLOBALS->treeroot, NULL);
/* SPLASH */                            splash_sync(5, 5);  
        order_facs_from_treesort(GLOBALS->treeroot, &GLOBALS->facs);
                                
        GLOBALS->facs_are_sorted=1;
        }
        else
	{
	for(i=0;i<GLOBALS->numfacs;i++)
		{
		char *subst;
#ifdef WAVE_HIERFIX
		char ch;
#endif
		int len;

		GLOBALS->facs[i]=&sym_block[i];
	        if((len=strlen(subst=GLOBALS->facs[i]->name))>GLOBALS->longestname) GLOBALS->longestname=len;
#ifdef WAVE_HIERFIX
		while((ch=(*subst)))
			{	
			if(ch==GLOBALS->hier_delimeter) { *subst=VCDNAM_HIERSORT; }	/* forces sort at hier boundaries */
			subst++;
			}
#endif
		}

/* SPLASH */                            splash_sync(3, 5);  
	fprintf(stderr, EXTLOAD"Sorting facilities at hierarchy boundaries.\n");
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
	fprintf(stderr, EXTLOAD"Building facility hierarchy tree.\n");

	init_tree();		
	for(i=0;i<GLOBALS->numfacs;i++)	
		{
		char *nf = GLOBALS->facs[i]->name;
	        build_tree_from_name(nf, i);
		}
/* SPLASH */                            splash_sync(5, 5);  
	treegraft(&GLOBALS->treeroot);
	treesort(GLOBALS->treeroot, NULL);
	}

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

	GLOBALS->min_time = b_start;
	GLOBALS->max_time = b_end;
	}

return(GLOBALS->max_time);
}


/*
 * extload callback (only does bits for now)
 */
static void extload_callback(TimeType *tim, int *facidx, char **value)
{
struct HistEnt *htemp = histent_calloc();
struct lx2_entry *l2e = GLOBALS->vzt_table_vzt_c_1+(*facidx);
struct fac *f = GLOBALS->mvlfacs_vzt_c_3+(*facidx);


GLOBALS->busycnt_vzt_c_2++; 
if(GLOBALS->busycnt_vzt_c_2==WAVE_BUSY_ITER)
	{
	busy_window_refresh();
	GLOBALS->busycnt_vzt_c_2 = 0;
	}

/* fprintf(stderr, "%lld %d %s\n", *tim, *facidx, *value); */

if(!(f->flags&(VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))
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
			case 'z':	htemp->v.h_val = AN_Z; break;
			default:	htemp->v.h_val = AN_X; break;
			}
	        }
	}
else if(f->flags&VZT_RD_SYM_F_DOUBLE)
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
static void ext_resolver(nptr np, nptr resolve)
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
 * actually import a extload trace but don't do it if it's already been imported 
 */
void import_extload_trace(nptr np)
{
struct HistEnt *htemp, *histent_tail;
int len, i;
struct fac *f;
int txidx, txidx_in_trace;
nptr nold = np;

if(!(f=np->mv.mvlfac)) return;	/* already imported */

txidx = f - GLOBALS->mvlfacs_vzt_c_3;
txidx_in_trace = GLOBALS->extload_idcodes[txidx];

if(GLOBALS->extload_inv_idcodes[txidx_in_trace] < 0)
	{
	txidx = (-GLOBALS->extload_inv_idcodes[txidx_in_trace]) - 1;
	
	np = GLOBALS->mvlfacs_vzt_c_3[txidx].working_node;

	if(!(f=np->mv.mvlfac)) 
		{
		ext_resolver(nold, np);
		return;	/* already imported */
		}
	}

GLOBALS->extload_inv_idcodes[txidx_in_trace] = - (txidx + 1); 

fprintf(stderr, EXTLOAD"Import: %s\n", np->nname);

/* new stuff */
len = np->mv.mvlfac->len;

if((f->array_height <= 1)&&(last_modification_check())) /* sorry, arrays not supported */
	{
	char sbuff[65537];
	TimeType tim;

	sprintf(sbuff, "%s -vc -vidcode %d %s 2>&1", EXTLOAD_PATH, txidx_in_trace, GLOBALS->loaded_file_name);
	GLOBALS->extload = popen(sbuff, "r");

	for(;;)
		{
	        char *rc = fgets(sbuff, 65536, GLOBALS->extload);
	        if(!rc)
	                {
	                break;
	                }

		if(isdigit(rc[0]))
			{
			rc = strchr(rc, '(');
			if(rc)
				{
				unsigned int lo = 0, hi = 0;
				sscanf(rc+1, "%d %d", &hi, &lo);
				tim = (TimeType)((((UTimeType)hi)<<32) + ((UTimeType)lo));
				
				rc = strchr(rc+1, ')');
				if(rc)
					{
					rc = strchr(rc+1, ':');
					if(rc)
						{
						char *rtn, *pnt;
						rc += 2;

						rtn = rc;
						while(*rtn)
							{
							if(isspace(*rtn)) { *rtn = 0; break; }
							rtn++;
							}

						pnt = rc;
						while(*pnt)
							{
							switch(*pnt)
								{
								case 'Z':
								case '3': *pnt = 'z'; break;

								case 'X':
								case '2': *pnt = 'x'; break;

								default: break;
								}

							pnt++;
							}

						extload_callback(&tim, &txidx, &rc);
						}
					}
				}
			}
		}

	pclose(GLOBALS->extload);
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

if(GLOBALS->vzt_table_vzt_c_1[txidx].histent_curr)
	{
	GLOBALS->vzt_table_vzt_c_1[txidx].histent_curr->next = htemp;
	htemp = GLOBALS->vzt_table_vzt_c_1[txidx].histent_head;
	}

if(!(f->flags&(VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))
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
        if(f->flags&VZT_RD_SYM_F_STRING) np->head.flags |= HIST_STRING;
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
        GLOBALS->vzt_table_vzt_c_1[txidx].numtrans++;
        }

np->head.time  = -2;
np->head.next = htemp;
np->numhist=GLOBALS->vzt_table_vzt_c_1[txidx].numtrans +2 /*endcap*/ +1 /*frontcap*/;

memset(GLOBALS->vzt_table_vzt_c_1+txidx, 0, sizeof(struct lx2_entry));	/* zero it out */

np->curr = histent_tail;
np->mv.mvlfac = NULL;	/* it's imported and cached so we can forget it's an mvlfac now */

if(nold!=np)
	{
	ext_resolver(nold, np);
	}
}

#endif

/*
 * $Id$
 * $Log$
 * Revision 1.12  2011/01/07 20:17:10  gtkwave
 * remove redundant fields from struct fac
 *
 * Revision 1.11  2010/05/27 06:56:39  gtkwave
 * printf warning fixes
 *
 * Revision 1.10  2010/03/19 17:50:57  gtkwave
 * compatibility fixes from fac struct changes
 *
 * Revision 1.9  2010/03/13 21:38:16  gtkwave
 * fixed && used in logical operations for allocating ExtNode
 *
 * Revision 1.8  2010/03/13 19:16:00  gtkwave
 * removal of useless symbol->nextinaet code
 *
 * Revision 1.7  2010/03/01 05:16:26  gtkwave
 * move compressed hier tree traversal to hierpack
 *
 * Revision 1.6  2010/01/23 03:21:11  gtkwave
 * hierarchy fixes when characters < "." are in the signal names
 *
 * Revision 1.5  2009/07/01 07:39:12  gtkwave
 * decorating hierarchy tree with module type info
 *
 * Revision 1.4  2009/02/16 17:16:05  gtkwave
 * extload error hardening and recovery
 *
 * Revision 1.3  2009/01/28 20:36:56  gtkwave
 * convert 3 value to z in value changes, likewise 2 to x
 *
 * Revision 1.2  2009/01/27 07:34:42  gtkwave
 * use atoi rather than atoi64
 *
 * Revision 1.1  2009/01/27 07:04:28  gtkwave
 * added extload external process loader capability
 *
 *
 */
