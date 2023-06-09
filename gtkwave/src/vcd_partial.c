/* 
 * Copyright (c) Tony Bybell 1999-2007.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */


/*
 * vcd.c 			23jan99ajb
 * evcd parts 			29jun99ajb
 * profiler optimizations 	15jul99ajb
 * more profiler optimizations	25jan00ajb
 * finsim parameter fix		26jan00ajb
 * vector rechaining code	03apr00ajb
 * multiple var section code	06apr00ajb
 * fix for duplicate nets	19dec00ajb
 * support for alt hier seps	23dec00ajb
 * fix for rcs identifiers	16jan01ajb
 * coredump fix for bad VCD	04apr02ajb
 * min/maxid speedup            27feb03ajb
 * bugfix on min/maxid speedup  06jul03ajb
 * escaped hier modification    20feb06ajb
 * added partial loader support 04aug06ajb
 * added real_parameter vartype 04aug06ajb
 * added in/out port vartype	31jan07ajb
 * use gperf for port vartypes  19feb07ajb
 * MTI SV implicit-var fix      05apr07ajb
 * MTI SV len=0 is real var     05apr07ajb
 */
#include <config.h>
#include "vcd.h"

#if !defined _MSC_VER && !defined __MINGW32__
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#undef VCD_BSEARCH_IS_PERFECT		/* bsearch is imperfect under linux, but OK under AIX */

static hptr add_histent_p(TimeType time, struct Node *n, char ch, int regadd, char *vector);
static void add_tail_histents(void);
static void vcd_build_symbols(void);
static void vcd_cleanup(void);
static void evcd_strcpy(char *dst, char *src);

static off_t vcdbyteno=0;
static int error_count=0;	/* should always be zero */

static int header_over=0;
static int dumping_off=0;
static TimeType start_time=-1;
static TimeType end_time=-1;
static TimeType current_time=-1;

static int num_glitches=0;
static int num_glitch_regions=0;

static struct vcdsymbol *pv=NULL, *rootv=NULL;
static char *vcdbuf=NULL, *vst=NULL, *vend=NULL;

/*******************************************************************************/

#define WAVE_PARTIAL_VCD_RING_BUFFER_SIZE (1024*1024)

static char *consume_ptr;
static char *buf;

unsigned int get_8(char *p)
{
if(p >= (buf + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE))
        {
        p-= WAVE_PARTIAL_VCD_RING_BUFFER_SIZE;
        }

return((unsigned int)((unsigned char)*p));
}

unsigned int get_32(char *p)
{
unsigned int rc =
        (get_8(p++) << 24) |
        (get_8(p++) << 16) |
        (get_8(p++) <<  8) |
        (get_8(p)   <<  0) ;

return(rc);
}

static int consume_countdown = 100000;

int consume(void)       /* for testing only */
{
int len;

consume_countdown--;
if(!consume_countdown)
	{
	consume_countdown = 100000;
	return(0);
	}

if((len = *consume_ptr))
        {
        int i;

        len = get_32(consume_ptr+1);
        for(i=0;i<len;i++)
                {
                vcdbuf[i] = get_8(consume_ptr+i+5);
                }
        vcdbuf[i] = 0;

        *consume_ptr = 0;
        consume_ptr = consume_ptr+i+5;
        if(consume_ptr >= (buf + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE))
                {
                consume_ptr -= WAVE_PARTIAL_VCD_RING_BUFFER_SIZE;
                }
        }

return(len);
}


/******************************************************************/

enum Tokens   { T_VAR, T_END, T_SCOPE, T_UPSCOPE,
		T_COMMENT, T_DATE, T_DUMPALL, T_DUMPOFF, T_DUMPON,
		T_DUMPVARS, T_ENDDEFINITIONS, 
		T_DUMPPORTS, T_DUMPPORTSOFF, T_DUMPPORTSON, T_DUMPPORTSALL,
		T_TIMESCALE, T_VERSION, T_VCDCLOSE,
		T_EOF, T_STRING, T_UNKNOWN_KEY };

static char *tokens[]={ "var", "end", "scope", "upscope",
		 "comment", "date", "dumpall", "dumpoff", "dumpon",
		 "dumpvars", "enddefinitions",
		 "dumpports", "dumpportsoff", "dumpportson", "dumpportsall",
		 "timescale", "version", "vcdclose",
		 "", "", "" };

#define NUM_TOKENS 18

static int T_MAX_STR=1024;	/* was originally a const..now it reallocs */
static char *yytext=NULL;
static int yylen=0, yylen_cache=0;

#define T_GET tok=get_token();if((tok==T_END)||(tok==T_EOF))break;

/******************************************************************/

static struct vcdsymbol *vcdsymroot=NULL, *vcdsymcurr=NULL;
static struct vcdsymbol **sorted=NULL;
static struct vcdsymbol **indexed=NULL;

#ifdef DEBUG_PRINTF
static char *vartypes[]={ "event", "parameter",
                "integer", "real", "real_parameter", "reg", "supply0",
                "supply1", "time", "tri", "triand", "trior",
                "trireg", "tri0", "tri1", "wand", "wire", "wor", "port", "in", "out", "inout",
                "$end", "", "", "", ""};  
#endif

static const unsigned char varenums[] = {  V_EVENT, V_PARAMETER,
                V_INTEGER, V_REAL, V_REAL_PARAMETER, V_REG, V_SUPPLY0,
                V_SUPPLY1, V_TIME, V_TRI, V_TRIAND, V_TRIOR,
                V_TRIREG, V_TRI0, V_TRI1, V_WAND, V_WIRE, V_WOR, V_PORT, V_IN, V_OUT, V_INOUT,
                V_END, V_LB, V_COLON, V_RB, V_STRING };

#define NUM_VTOKENS 23  

static int numsyms=0;

/******************************************************************/

static struct queuedevent *queuedevents=NULL;

/******************************************************************/
 
static unsigned int vcd_minid = ~0;
static unsigned int vcd_maxid = 0;

static unsigned int vcdid_hash(char *s, int len)
{  
unsigned int val=0;
int i;

s+=(len-1);
                 
for(i=0;i<len;i++)
        {
        val *= 95;				/* was 94 but XL uses '!' as right hand side chars which act as leading zeros */
        val += (((unsigned char)*s) - 32);	/* was 33 but XL ... */
        s--;
        }

return(val);
}

/******************************************************************/

/*
 * bsearch compare
 */
static int vcdsymbsearchcompare(const void *s1, const void *s2)
{
char *v1;
struct vcdsymbol *v2;

v1=(char *)s1;
v2=*((struct vcdsymbol **)s2);

return(strcmp(v1, v2->id));
}


/*
 * actual bsearch
 */
static struct vcdsymbol *bsearch_vcd(char *key, int len)
{
struct vcdsymbol **v;
struct vcdsymbol *t;

if(indexed)
        {
        unsigned int hsh = vcdid_hash(key, len);
        if((hsh>=vcd_minid)&&(hsh<=vcd_maxid))
                {
                return(indexed[hsh-vcd_minid]);
                }

	return(NULL);
        }

if(sorted)
	{
	v=(struct vcdsymbol **)bsearch(key, sorted, numsyms, 
		sizeof(struct vcdsymbol *), vcdsymbsearchcompare);

	if(v)
		{
		#ifndef VCD_BSEARCH_IS_PERFECT
			for(;;)
				{
				t=*v;
		
				if((v==sorted)||(strcmp((*(--v))->id, key)))
					{
					return(t);
					}
				}
		#else
			return(*v);
		#endif
		}
		else
		{
		return(NULL);
		}
	}
	else
	{
	static int err = 0;
	if(!err)
		{
		fprintf(stderr, "Near byte %d, VCD search table NULL..is this a VCD file?\n", (int)(vcdbyteno+(vst-vcdbuf)));
		err=1;
		}
	return(NULL);
	}
}


/*
 * sort on vcdsymbol pointers
 */
static int vcdsymcompare(const void *s1, const void *s2)
{
struct vcdsymbol *v1, *v2;

v1=*((struct vcdsymbol **)s1);
v2=*((struct vcdsymbol **)s2);

return(strcmp(v1->id, v2->id));
}


/*
 * create sorted (by id) table
 */
static void create_sorted_table(void)
{
struct vcdsymbol *v;
struct vcdsymbol **pnt;
unsigned int vcd_distance;

if(sorted) 
	{
	free_2(sorted);	/* this means we saw a 2nd enddefinition chunk! */
	sorted=NULL;
	}

if(indexed)
	{
	free_2(indexed);
	indexed=NULL;
	}

if(numsyms)
	{
        vcd_distance = vcd_maxid - vcd_minid + 1;

        if(vcd_distance <= VCD_INDEXSIZ)
                {
                indexed = (struct vcdsymbol **)calloc_2(vcd_distance, sizeof(struct vcdsymbol *));
         
		/* printf("%d symbols span ID range of %d, using indexing...\n", numsyms, vcd_distance); */

                v=vcdsymroot;
                while(v)
                        {
                        if(!indexed[v->nid - vcd_minid]) indexed[v->nid - vcd_minid] = v;
                        v=v->next;
                        }
                }
                else
		{	
		pnt=sorted=(struct vcdsymbol **)calloc_2(numsyms, sizeof(struct vcdsymbol *));
		v=vcdsymroot;
		while(v)
			{
			*(pnt++)=v;
			v=v->next;
			}
	
		qsort(sorted, numsyms, sizeof(struct vcdsymbol *), vcdsymcompare);
		}
	}
}

/******************************************************************/

/*
 * single char get inlined/optimized
 */
static void getch_alloc(void)
{
vend=vst=vcdbuf=(char *)calloc_2(1,VCD_BSIZ);
}


static int getch_fetch(void)
{
size_t rd;

errno = 0;

vcdbyteno+=(vend-vcdbuf);
rd=consume();
vend=(vst=vcdbuf)+rd;

if(!rd) return(-1);

return((int)(*(vst++)));
}

#define getch() ((vst!=vend)?((int)(*(vst++))):(getch_fetch()))


static char *varsplit=NULL, *vsplitcurr=NULL;
static int getch_patched(void)
{
char ch;

ch=*vsplitcurr;
if(!ch)
	{
	return(-1);
	}
	else
	{
	vsplitcurr++;
	return((int)ch);
	}
}

/*
 * simple tokenizer
 */
static int get_token(void)
{
int ch;
int i, len=0;
int is_string=0;
char *yyshadow;

for(;;)
	{
	ch=getch();
	if(ch<0) return(T_EOF);
	if(ch<=' ') continue;	/* val<=' ' is a quick whitespace check      */
	break;			/* (take advantage of fact that vcd is text) */
	}
if(ch=='$') 
	{
	yytext[len++]=ch;
	for(;;)
		{
		ch=getch();
		if(ch<0) return(T_EOF);
		if(ch<=' ') continue;
		break;
		}
	}
	else
	{
	is_string=1;
	}

for(yytext[len++]=ch;;yytext[len++]=ch)
	{
	if(len==T_MAX_STR)
		{
		yytext=(char *)realloc_2(yytext, (T_MAX_STR=T_MAX_STR*2)+1);
		}
	ch=getch();
	if(ch<=' ') break;
	}
yytext[len]=0;	/* terminator */

if(is_string) 
	{
	yylen=len;
	return(T_STRING);
	}

yyshadow=yytext;
do
{
yyshadow++;
for(i=0;i<NUM_TOKENS;i++)
	{
	if(!strcmp(yyshadow,tokens[i]))
		{
		return(i);
		}
	}

} while(*yyshadow=='$'); /* fix for RCS ids in version strings */

return(T_UNKNOWN_KEY);
}


static int var_prevch=0;
static int get_vartoken_patched(int match_kw)
{
int ch;
int len=0;

if(!var_prevch)
	{
	for(;;)
		{
		ch=getch_patched();
		if(ch<0) { free_2(varsplit); varsplit=NULL; return(V_END); }
		if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')) continue;
		break;
		}
	}
	else
	{
	ch=var_prevch;
	var_prevch=0;
	}
	
if(ch=='[') return(V_LB);
if(ch==':') return(V_COLON);
if(ch==']') return(V_RB);

for(yytext[len++]=ch;;yytext[len++]=ch)
	{
	if(len==T_MAX_STR)
		{
		yytext=(char *)realloc_2(yytext, (T_MAX_STR=T_MAX_STR*2)+1);
		}
	ch=getch_patched();
	if(ch<0) { free_2(varsplit); varsplit=NULL; break; }
	if((ch==':')||(ch==']'))
		{
		var_prevch=ch;
		break;
		}
	}
yytext[len]=0;	/* terminator */

if(match_kw)
	{
	int vt = vcd_keyword_code(yytext, len);
	if(vt != V_STRING)
		{
		if(ch<0) { free_2(varsplit); varsplit=NULL; }
		return(vt);
		}
	}

yylen=len;
if(ch<0) { free_2(varsplit); varsplit=NULL; }
return(V_STRING);
}

static int get_vartoken(int match_kw)
{
int ch;
int len=0;

if(varsplit)
	{
	int rc=get_vartoken_patched(match_kw);
	if(rc!=V_END) return(rc);
	var_prevch=0;
	}

if(!var_prevch)
	{
	for(;;)
		{
		ch=getch();
		if(ch<0) return(V_END);
		if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')) continue;
		break;
		}
	}
	else
	{
	ch=var_prevch;
	var_prevch=0;
	}
	
if(ch=='[') return(V_LB);
if(ch==':') return(V_COLON);
if(ch==']') return(V_RB);

if(ch=='#')     /* for MTI System Verilog '$var reg 64 >w #implicit-var###VarElem:ram_di[0.0] [63:0] $end' style declarations */
        {       /* debussy simply escapes until the space */
        yytext[len++]= '\\';
        }

for(yytext[len++]=ch;;yytext[len++]=ch)
	{
	if(len==T_MAX_STR)
		{
		yytext=(char *)realloc_2(yytext, (T_MAX_STR=T_MAX_STR*2)+1);
		}
	ch=getch();
	if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')||(ch<0)) break;
	if((ch=='[')&&(yytext[0]!='\\'))
		{
		varsplit=yytext+len;		/* keep looping so we get the *last* one */
		}
	else
	if(((ch==':')||(ch==']'))&&(!varsplit)&&(yytext[0]!='\\'))
		{
		var_prevch=ch;
		break;
		}
	}
yytext[len]=0;	/* absolute terminator */
if((varsplit)&&(yytext[len-1]==']'))
	{
	char *vst;
	vst=malloc_2(strlen(varsplit)+1);
	strcpy(vst, varsplit);

	*varsplit=0x00;		/* zero out var name at the left bracket */
	len=varsplit-yytext;

	varsplit=vsplitcurr=vst;
	var_prevch=0;
	}
	else
	{
	varsplit=NULL;
	}

if(match_kw)
	{
	int vt = vcd_keyword_code(yytext, len);
	if(vt != V_STRING)
		{
		return(vt);
		}
	}

yylen=len;
return(V_STRING);
}

static int get_strtoken(void)
{
int ch;
int len=0;

if(!var_prevch)
      {
      for(;;)
              {
              ch=getch();
              if(ch<0) return(V_END);
              if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')) continue;
              break;
              }
      }
      else
      {
      ch=var_prevch;
      var_prevch=0;
      }
      
for(yytext[len++]=ch;;yytext[len++]=ch)
      {
	if(len==T_MAX_STR)
		{
		yytext=(char *)realloc_2(yytext, (T_MAX_STR=T_MAX_STR*2)+1);
		}
      ch=getch();
      if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')||(ch<0)) break;
      }
yytext[len]=0;        /* terminator */

yylen=len;
return(V_STRING);
}

static void sync_end(char *hdr)
{
int tok;

if(hdr) DEBUG(fprintf(stderr,"%s",hdr));
for(;;)
	{
	tok=get_token();
	if((tok==T_END)||(tok==T_EOF)) break;
	if(hdr)DEBUG(fprintf(stderr," %s",yytext));
	}
if(hdr) DEBUG(fprintf(stderr,"\n"));
}


static void parse_valuechange(void)
{
struct vcdsymbol *v;
char *vector;
int vlen;
hptr hsuf;

switch(yytext[0])
	{
	case '0':
	case '1':
	case 'x': case 'X':
	case 'z': case 'Z':
	case 'h': case 'H':
	case 'u': case 'U':
	case 'w': case 'W':
	case 'l': case 'L':
	case '-':
		if(yylen>1)
			{
			v=bsearch_vcd(yytext+1, yylen-1);	
			if(!v)
				{
				fprintf(stderr,"Near byte %d, Unknown VCD identifier: '%s'\n",(int)(vcdbyteno+(vst-vcdbuf)),yytext+1);
				}
				else
				{
				if(v->vartype!=V_EVENT)
					{
					v->value[0]=yytext[0];
					DEBUG(fprintf(stderr,"%s = '%c'\n",v->name,v->value[0]));

					v->narray[0]->curr = v->app_array[0];
					hsuf = add_histent_p(current_time,v->narray[0],v->value[0],1, NULL);
					v->app_array[0] = v->narray[0]->curr;
					v->narray[0]->curr->next = v->tr_array[0];
					if(v->narray[0]->harray) { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }

					}
					else
					{
					v->value[0]=(dumping_off)?'x':'1'; /* only '1' is relevant */
					if(current_time!=(v->ev->last_event_time+1))
						{
						/* dump degating event */
						DEBUG(fprintf(stderr,"#"TTFormat" %s = '%c' (event)\n",v->ev->last_event_time+1,v->name,'0'));
						v->narray[0]->curr = v->app_array[0];
						add_histent_p(v->ev->last_event_time+1,v->narray[0],'0',1, NULL);
						v->app_array[0] = v->narray[0]->curr;
						v->narray[0]->curr->next = v->tr_array[0];
						}
					DEBUG(fprintf(stderr,"%s = '%c' (event)\n",v->name,v->value[0]));
					v->narray[0]->curr = v->app_array[0];
					add_histent_p(current_time,v->narray[0],v->value[0],1, NULL);
					v->app_array[0] = v->narray[0]->curr;
					v->narray[0]->curr->next = v->tr_array[0];
					if(v->narray[0]->harray) { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }

					v->ev->last_event_time=current_time;
					}
				}
			}
			else
			{
			fprintf(stderr,"Near byte %d, Malformed VCD identifier\n", (int)(vcdbyteno+(vst-vcdbuf)));
			}
		break;

	case 'b':
	case 'B':
		{
		/* extract binary number then.. */
		vector=malloc_2(yylen_cache=yylen); 
		strcpy(vector,yytext+1);
		vlen=yylen-1;

		get_strtoken();
		v=bsearch_vcd(yytext, yylen);	
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(vcdbyteno+(vst-vcdbuf)), yytext);
			free_2(vector);
			}
			else
			{
			if ((v->vartype==V_REAL)||((convert_to_reals)&&((v->vartype==V_INTEGER)||(v->vartype==V_PARAMETER))))
				{
				double *d;
				char *pnt;
				char ch;
				TimeType k=0;
		
				pnt=vector;
				while((ch=*(pnt++))) { k=(k<<1)|((ch=='1')?1:0); }
				free_2(vector);
			
				d=malloc_2(sizeof(double));
				*d=(double)k;
			
				if(!v)
					{
					fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(vcdbyteno+(vst-vcdbuf)), yytext);
					free_2(d);
					}
					else
					{
					v->narray[0]->curr = v->app_array[0];					
					add_histent_p(current_time, v->narray[0],'g',1,(char *)d);
					v->app_array[0] = v->narray[0]->curr;
					v->narray[0]->curr->next = v->tr_array[0];
					if(v->narray[0]->harray) 
					        { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }
					}
				break;
				}

			if(vlen<v->size) 	/* fill in left part */
				{
				char extend;
				int i, fill;

				extend=(vector[0]=='1')?'0':vector[0];

				fill=v->size-vlen;				
				for(i=0;i<fill;i++)
					{
					v->value[i]=extend;
					}
				strcpy(v->value+fill,vector);
				}
			else if(vlen==v->size) 	/* straight copy */
				{
				strcpy(v->value,vector);
				}
			else			/* too big, so copy only right half */
				{
				int skip;

				skip=vlen-v->size;
				strcpy(v->value,vector+skip);
				}
			DEBUG(fprintf(stderr,"%s = '%s'\n",v->name, v->value));

			if((v->size==1)||(!atomic_vectors))
				{
				int i;
				for(i=0;i<v->size;i++)
					{
					v->narray[i]->curr = v->app_array[i];
					add_histent_p(current_time, v->narray[i],v->value[i],1, NULL);
					v->app_array[i] = v->narray[i]->curr;
					v->narray[i]->curr->next = v->tr_array[i];
					if(v->narray[i]->harray) 
					        { free_2(v->narray[i]->harray); v->narray[i]->harray = NULL; }
					}
				free_2(vector);
				}
				else
				{
				if(yylen_cache!=(v->size+1))
					{
					free_2(vector);
					vector=malloc_2(v->size+1);
					}
				strcpy(vector,v->value);
				v->narray[0]->curr = v->app_array[0];
				add_histent_p(current_time, v->narray[0],0,1,vector);
				v->app_array[0] = v->narray[0]->curr;
				v->narray[0]->curr->next = v->tr_array[0];
				if(v->narray[0]->harray) 
				        { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }
				}

			}
		break;
		}

	case 'p':
		/* extract port dump value.. */
		vector=malloc_2(yylen_cache=yylen); 
		strcpy(vector,yytext+1);
		vlen=yylen-1;

		get_strtoken();	/* throw away 0_strength_component */
		get_strtoken(); /* throw away 0_strength_component */
		get_strtoken(); /* this is the id                  */
		v=bsearch_vcd(yytext, yylen);	
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(vcdbyteno+(vst-vcdbuf)), yytext);
			free_2(vector);
			}
			else
			{
			if ((v->vartype==V_REAL)||((convert_to_reals)&&((v->vartype==V_INTEGER)||(v->vartype==V_PARAMETER))))
				{
				double *d;
				char *pnt;
				char ch;
				TimeType k=0;
		
				pnt=vector;
				while((ch=*(pnt++))) { k=(k<<1)|((ch=='1')?1:0); }
				free_2(vector);
			
				d=malloc_2(sizeof(double));
				*d=(double)k;
			
				if(!v)
					{
					fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(vcdbyteno+(vst-vcdbuf)), yytext);
					free_2(d);
					}
					else
					{
					v->narray[0]->curr = v->app_array[0];
					add_histent_p(current_time, v->narray[0],'g',1,(char *)d);
					v->app_array[0] = v->narray[0]->curr;
					v->narray[0]->curr->next = v->tr_array[0];
					if(v->narray[0]->harray) 
					        { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }
					}
				break;
				}

			if(vlen<v->size) 	/* fill in left part */
				{
				char extend;
				int i, fill;

				extend='0';

				fill=v->size-vlen;				
				for(i=0;i<fill;i++)
					{
					v->value[i]=extend;
					}
				evcd_strcpy(v->value+fill,vector);
				}
			else if(vlen==v->size) 	/* straight copy */
				{
				evcd_strcpy(v->value,vector);
				}
			else			/* too big, so copy only right half */
				{
				int skip;

				skip=vlen-v->size;
				evcd_strcpy(v->value,vector+skip);
				}
			DEBUG(fprintf(stderr,"%s = '%s'\n",v->name, v->value));

			if((v->size==1)||(!atomic_vectors))
				{
				int i;
				for(i=0;i<v->size;i++)
					{
					v->narray[i]->curr = v->app_array[i];
					add_histent_p(current_time, v->narray[i],v->value[i],1, NULL);
					v->app_array[i] = v->narray[i]->curr;
					v->narray[i]->curr->next = v->tr_array[i];
					if(v->narray[i]->harray) 
					        { free_2(v->narray[i]->harray); v->narray[i]->harray = NULL; }
					}
				free_2(vector);
				}
				else
				{
				if(yylen_cache<v->size)
					{
					free_2(vector);
					vector=malloc_2(v->size+1);
					}
				strcpy(vector,v->value);
				v->narray[0]->curr = v->app_array[0];
				add_histent_p(current_time, v->narray[0],0,1,vector);
				v->app_array[0] = v->narray[0]->curr;
				v->narray[0]->curr->next = v->tr_array[0];
				if(v->narray[0]->harray) 
				        { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }
				}
			}
		break;


	case 'r':
	case 'R':
		{
		double *d;

		d=malloc_2(sizeof(double));
		sscanf(yytext+1,"%lg",d);
		
		get_strtoken();
		v=bsearch_vcd(yytext, yylen);	
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(vcdbyteno+(vst-vcdbuf)), yytext);
			free_2(d);
			}
			else
			{
			v->narray[0]->curr = v->app_array[0];
			add_histent_p(current_time, v->narray[0],'g',1,(char *)d);
			v->app_array[0] = v->narray[0]->curr;
			v->narray[0]->curr->next = v->tr_array[0];
			if(v->narray[0]->harray) 
			        { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }
			}

		break;
		}

#ifndef STRICT_VCD_ONLY
	case 's':
	case 'S':
		{
		char *d;

		d=(char *)malloc_2(yylen);
		strcpy(d, yytext+1);
		
		get_strtoken();
		v=bsearch_vcd(yytext, yylen);	
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(vcdbyteno+(vst-vcdbuf)), yytext);
			free_2(d);
			}
			else
			{
			v->narray[0]->curr = v->app_array[0];
			add_histent_p(current_time, v->narray[0],'s',1,(char *)d);
			v->app_array[0] = v->narray[0]->curr;
			v->narray[0]->curr->next = v->tr_array[0];
			if(v->narray[0]->harray) 
			        { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }
			}

		break;
		}
#endif
	}

}


static void evcd_strcpy(char *dst, char *src)
{
static char *evcd="DUNZduLHXTlh01?FAaBbCcf";
static char  *vcd="01xz0101xz0101xzxxxxxxx";

char ch;
int i;

while((ch=*src))
	{
	for(i=0;i<23;i++)
		{
		if(evcd[i]==ch)
			{
			*dst=vcd[i];
			break;
			}
		}	
	if(i==23) *dst='x';

	src++;
	dst++;
	}

*dst=0;	/* null terminate destination */
}


static void vcd_parse(void)
{
int tok;

for(;;)
	{
	switch(tok=get_token())
		{
		case T_COMMENT:
			sync_end("COMMENT:");
			break;
		case T_DATE:
			sync_end("DATE:");
			break;
		case T_VERSION:
			sync_end("VERSION:");
			break;
		case T_TIMESCALE:
			{
			int vtok;
			int i;
			char prefix=' ';

			vtok=get_token();
			if((vtok==T_END)||(vtok==T_EOF)) break;
			time_scale=atoi_64(yytext);
			if(!time_scale) time_scale=1;
			for(i=0;i<yylen;i++)
				{
				if((yytext[i]<'0')||(yytext[i]>'9'))
					{
					prefix=yytext[i];
					break;
					}
				}
			if(prefix==' ')
				{
				vtok=get_token();
				if((vtok==T_END)||(vtok==T_EOF)) break;
				prefix=yytext[0];		
				}
			switch(prefix)
				{
				case ' ':
				case 'm':
				case 'u':
				case 'n':
				case 'p':
				case 'f':
					time_dimension=prefix;
					break;
				case 's':
					time_dimension=' ';
					break;
				default:	/* unknown */
					time_dimension='n';
					break;
				}

			DEBUG(fprintf(stderr,"TIMESCALE: "TTFormat" %cs\n",time_scale, time_dimension));
			sync_end(NULL);
			}
			break;
		case T_SCOPE:
			T_GET;
			T_GET;
			if(tok==T_STRING)
				{
				struct slist *s;
				s=(struct slist *)calloc_2(1,sizeof(struct slist));
				s->len=yylen;
				s->str=(char *)malloc_2(yylen+1);
				strcpy(s->str, yytext);

				if(slistcurr)
					{
					slistcurr->next=s;
					slistcurr=s;
					}
					else
					{
					slistcurr=slistroot=s;
					}

				build_slisthier();
				DEBUG(fprintf(stderr, "SCOPE: %s\n",slisthier));
				}
			sync_end(NULL);
			break;
		case T_UPSCOPE:
			if(slistroot)
				{
				struct slist *s;

				s=slistroot;
				if(!s->next)
					{
					free_2(s->str);
					free_2(s);
					slistroot=slistcurr=NULL;
					}
				else
				for(;;)
					{
					if(!s->next->next)
						{
						free_2(s->next->str);
						free_2(s->next);
						s->next=NULL;
						slistcurr=s;
						break;
						}
					s=s->next;
					}
				build_slisthier();
				DEBUG(fprintf(stderr, "SCOPE: %s\n",slisthier));
				}
			sync_end(NULL);
			break;
		case T_VAR:
			if((header_over)&&(0))
			{
			fprintf(stderr,"$VAR encountered after $ENDDEFINITIONS near byte %d.  VCD is malformed, exiting.\n",
				(int)(vcdbyteno+(vst-vcdbuf)));
			exit(0);
			}
			else
			{
			int vtok;
			struct vcdsymbol *v=NULL;

			var_prevch=0;
			if(varsplit)
				{
				free_2(varsplit);
				varsplit=NULL;
				}
			vtok=get_vartoken(1);
			if(vtok>V_PORT) goto bail;

			v=(struct vcdsymbol *)calloc_2(1,sizeof(struct vcdsymbol));
			v->vartype=vtok;
			v->msi=v->lsi=vcd_explicit_zero_subscripts; /* indicate [un]subscripted status */

			if(vtok==V_PORT)
				{
				vtok=get_vartoken(0);
				if(vtok==V_STRING)
					{
					v->size=atoi_64(yytext);
					if(!v->size) v->size=1;
					}
					else 
					if(vtok==V_LB)
					{
					vtok=get_vartoken(1);
					if(vtok==V_END) goto err;
					if(vtok!=V_STRING) goto err;
					v->msi=atoi_64(yytext);
					vtok=get_vartoken(0);
					if(vtok==V_RB)
						{
						v->lsi=v->msi;
						v->size=1;
						}
						else
						{
						if(vtok!=V_COLON) goto err;
						vtok=get_vartoken(0);
						if(vtok!=V_STRING) goto err;
						v->lsi=atoi_64(yytext);
						vtok=get_vartoken(0);
						if(vtok!=V_RB) goto err;

						if(v->msi>v->lsi)
							{
							v->size=v->msi-v->lsi+1;
							}
							else
							{
							v->size=v->lsi-v->msi+1;
							}
						}
					}
					else goto err;

				vtok=get_strtoken();
				if(vtok==V_END) goto err;
				v->id=(char *)malloc_2(yylen+1);
				strcpy(v->id, yytext);
                                v->nid=vcdid_hash(yytext,yylen);

                                if(v->nid < vcd_minid) vcd_minid = v->nid;
                                if(v->nid > vcd_maxid) vcd_maxid = v->nid;

				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				if(slisthier_len)
					{
					v->name=(char *)malloc_2(slisthier_len+1+yylen+1);
					strcpy(v->name,slisthier);
					strcpy(v->name+slisthier_len,vcd_hier_delimeter);
					if(alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name+slisthier_len+1,yytext,alt_hier_delimeter);
						}
						else
						{
						if((strcpy_delimfix(v->name+slisthier_len+1,yytext)) && (yytext[0] != '\\'))
							{
							char *sd=(char *)malloc_2(slisthier_len+1+yylen+2);
							strcpy(sd,slisthier);
							strcpy(sd+slisthier_len,vcd_hier_delimeter);
							sd[slisthier_len+1] = '\\';
							strcpy(sd+slisthier_len+2,v->name+slisthier_len+1);
							free_2(v->name);
							v->name = sd;
							}
						}
					}
					else
					{
					v->name=(char *)malloc_2(yylen+1);
					if(alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name,yytext,alt_hier_delimeter);
						}
						else
						{
						if((strcpy_delimfix(v->name,yytext)) && (yytext[0] != '\\'))
							{
							char *sd=(char *)malloc_2(yylen+2);
							sd[0] = '\\';
							strcpy(sd+1,v->name);
							free_2(v->name);
							v->name = sd;
							}
						}
					}

                                if(pv)
                                        { 
                                        if(!strcmp(pv->name,v->name))
                                                {
                                                pv->chain=v;
                                                v->root=rootv;
                                                if(pv==rootv) pv->root=rootv;
                                                }
                                                else
                                                {
                                                rootv=v;
                                                }
                                        }
					else
					{
					rootv=v;
					}
                                pv=v;
				}
				else	/* regular vcd var, not an evcd port var */
				{
				vtok=get_vartoken(1);
				if(vtok==V_END) goto err;
				v->size=atoi_64(yytext);
				vtok=get_strtoken();
				if(vtok==V_END) goto err;
				v->id=(char *)malloc_2(yylen+1);
				strcpy(v->id, yytext);
                                v->nid=vcdid_hash(yytext,yylen);
                                
                                if(v->nid < vcd_minid) vcd_minid = v->nid;
                                if(v->nid > vcd_maxid) vcd_maxid = v->nid;

				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				if(slisthier_len)
					{
					v->name=(char *)malloc_2(slisthier_len+1+yylen+1);
					strcpy(v->name,slisthier);
					strcpy(v->name+slisthier_len,vcd_hier_delimeter);
					if(alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name+slisthier_len+1,yytext,alt_hier_delimeter);
						}
						else
						{
						if((strcpy_delimfix(v->name+slisthier_len+1,yytext)) && (yytext[0] != '\\'))
							{
                                                        char *sd=(char *)malloc_2(slisthier_len+1+yylen+2);
                                                        strcpy(sd,slisthier);
                                                        strcpy(sd+slisthier_len,vcd_hier_delimeter);
                                                        sd[slisthier_len+1] = '\\';
                                                        strcpy(sd+slisthier_len+2,v->name+slisthier_len+1);
                                                        free_2(v->name);
                                                        v->name = sd;
							}
						}
					}
					else
					{
					v->name=(char *)malloc_2(yylen+1);
					if(alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name,yytext,alt_hier_delimeter);
						}
						else
						{
                                                if((strcpy_delimfix(v->name,yytext)) && (yytext[0] != '\\'))
                                                        {
                                                        char *sd=(char *)malloc_2(yylen+2);
                                                        sd[0] = '\\';
                                                        strcpy(sd+1,v->name);
                                                        free_2(v->name);
                                                        v->name = sd;
                                                        }
						}
					}

                                if(pv)
                                        { 
                                        if(!strcmp(pv->name,v->name))
                                                {
                                                pv->chain=v;
                                                v->root=rootv;
                                                if(pv==rootv) pv->root=rootv;
                                                }
                                                else
                                                {
                                                rootv=v;
                                                }
                                        }
					else
					{
					rootv=v;
					}
                                pv=v;
				
				vtok=get_vartoken(1);
				if(vtok==V_END) goto dumpv;
				if(vtok!=V_LB) goto err;
				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				v->msi=atoi_64(yytext);
				vtok=get_vartoken(0);
				if(vtok==V_RB)
					{
					v->lsi=v->msi;
					goto dumpv;
					}
				if(vtok!=V_COLON) goto err;
				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				v->lsi=atoi_64(yytext);
				vtok=get_vartoken(0);
				if(vtok!=V_RB) goto err;
				}

			dumpv:
                        if(v->size == 0) { v->vartype = V_REAL; } /* MTI fix */

			if((v->vartype==V_REAL)||((convert_to_reals)&&((v->vartype==V_INTEGER)||(v->vartype==V_PARAMETER))))
				{
				v->vartype=V_REAL;
				v->size=1;		/* override any data we parsed in */
				v->msi=v->lsi=0;
				}
			else
			if((v->size>1)&&(v->msi<=0)&&(v->lsi<=0))
				{
				if(v->vartype==V_EVENT) 
					{
					v->size=1;
					}
					else
					{
					/* any criteria for the direction here? */
					v->msi=v->size-1;	
					v->lsi=0;
					}
				}
			else
			if((v->msi>v->lsi)&&((v->msi-v->lsi+1)!=v->size))
				{
				if((v->vartype!=V_EVENT)&&(v->vartype!=V_PARAMETER)) goto err;
				v->size=v->msi-v->lsi+1;
				}
			else
			if((v->lsi>=v->msi)&&((v->lsi-v->msi+1)!=v->size)) 
				{
				if((v->vartype!=V_EVENT)&&(v->vartype!=V_PARAMETER)) goto err;
				v->size=v->msi-v->lsi+1;
				}

			/* initial conditions */
			v->value=(char *)malloc_2(v->size+1);
			v->value[v->size]=0;
			v->narray=(struct Node **)calloc_2(v->size,sizeof(struct Node *));
			v->tr_array=(hptr *)calloc_2(v->size,sizeof(hptr));
			v->app_array=(hptr *)calloc_2(v->size,sizeof(hptr));
				{
				int i;
				if(atomic_vectors)
					{
					for(i=0;i<v->size;i++)
						{
						v->value[i]='x';
						}
					v->narray[0]=(struct Node *)calloc_2(1,sizeof(struct Node));
					v->narray[0]->head.time=-1;
					v->narray[0]->head.v.h_val=AN_X;
					}
					else
					{
					for(i=0;i<v->size;i++)
						{
						v->value[i]='x';
	
						v->narray[i]=(struct Node *)calloc_2(1,sizeof(struct Node));
						v->narray[i]->head.time=-1;
						v->narray[i]->head.v.h_val=AN_X;
						}
					}
				}

			if(v->vartype==V_EVENT)
				{
				struct queuedevent *q;
				v->ev=q=(struct queuedevent *)calloc_2(1,sizeof(struct queuedevent));
				q->sym=v;
				q->last_event_time=-1;		
				q->next=queuedevents;
				queuedevents=q;		
				}

			if(!vcdsymroot)
				{
				vcdsymroot=vcdsymcurr=v;
				}
				else
				{
				vcdsymcurr->next=v;
				vcdsymcurr=v;
				}
			numsyms++;

			DEBUG(fprintf(stderr,"VAR %s %d %s %s[%d:%d]\n",
				vartypes[v->vartype], v->size, v->id, v->name, 
					v->msi, v->lsi));
			goto bail;
			err:
			if(v)
				{
				error_count++;
				if(v->name) 
					{
					fprintf(stderr, "Near byte %d, $VAR parse error encountered with '%s'\n", (int)(vcdbyteno+(vst-vcdbuf)), v->name);
					free_2(v->name);
					}
					else
					{
					fprintf(stderr, "Near byte %d, $VAR parse error encountered\n", (int)(vcdbyteno+(vst-vcdbuf)));
					}
				if(v->id) free_2(v->id);
				if(v->value) free_2(v->value);
				free_2(v);
				pv = NULL;
				}

			bail:
			if(vtok!=V_END) sync_end(NULL);
			break;
			}
		case T_ENDDEFINITIONS:
			header_over=1;	/* do symbol table management here */
			create_sorted_table();
			if((!sorted)&&(!indexed))
				{
				fprintf(stderr, "No symbols in VCD file..nothing to do!\n");
				exit(1);
				}
			if(error_count)
				{
				fprintf(stderr, "\n%d VCD parse errors encountered, exiting.\n", error_count);
				exit(1);
				}
			return;
			break;
		case T_STRING:
			if(!header_over)
				{
				header_over=1;	/* do symbol table management here */
				create_sorted_table();
				if((!sorted)&&(!indexed)) break;
				}
				{
				/* catchall for events when header over */
				if(yytext[0]=='#')
					{
					TimeType time;
					time=atoi_64(yytext+1);
					
					if(start_time<0)
						{
						start_time=time;
						}

					current_time=time;
					if(end_time<time) end_time=time;	/* in case of malformed vcd files */
					DEBUG(fprintf(stderr,"#"TTFormat"\n",time));
					}
					else
					{
					parse_valuechange();
					}
				}
			break;
		case T_DUMPALL:	/* dump commands modify vals anyway so */
		case T_DUMPPORTSALL:
			break;	/* just loop through..                 */
		case T_DUMPOFF:
		case T_DUMPPORTSOFF:
			dumping_off=1;
			if((!blackout_regions)||((blackout_regions)&&(blackout_regions->bstart<=blackout_regions->bend)))
				{
				struct blackout_region_t *bt = calloc_2(1, sizeof(struct blackout_region_t));

				bt->bstart = current_time;
				bt->next = blackout_regions;
				blackout_regions = bt;
				}
			break;
		case T_DUMPON:
		case T_DUMPPORTSON:
			dumping_off=0;
			if((blackout_regions)&&(blackout_regions->bstart>blackout_regions->bend))
				{
				blackout_regions->bend = current_time;
				}
			break;
		case T_DUMPVARS:
		case T_DUMPPORTS:
			if(current_time<0)
				{ start_time=current_time=end_time=0; }
			break;
		case T_VCDCLOSE:
			break;	/* next token will be '#' time related followed by $end */
		case T_END:	/* either closure for dump commands or */
			break;	/* it's spurious                       */
		case T_UNKNOWN_KEY:
			sync_end(NULL);	/* skip over unknown keywords */
			break;
		case T_EOF:
			if((blackout_regions)&&(blackout_regions->bstart>blackout_regions->bend))
				{
				blackout_regions->bend = current_time;
				}
			return;
		default:
			DEBUG(fprintf(stderr,"UNKNOWN TOKEN\n"));
		}
	}
}


/*******************************************************************************/

hptr add_histent_p(TimeType time, struct Node *n, char ch, int regadd, char *vector)
{
struct HistEnt *he, *rc;
char heval;

if(!vector)
{
if(!(rc=n->curr))
	{
	he=histent_calloc();
        he->time=-1;
        he->v.h_val=AN_X;

	n->curr=he;
	n->head.next=he;

	add_histent_p(time,n,ch,regadd, vector);
	rc = he;
	}
	else
	{
	if(regadd) { time*=(time_scale); }

	if(ch=='0')              heval=AN_0; else
	if(ch=='1')              heval=AN_1; else
        if((ch=='x')||(ch=='X')) heval=AN_X; else
        if((ch=='z')||(ch=='Z')) heval=AN_Z; else
        if((ch=='h')||(ch=='H')) heval=AN_H; else
        if((ch=='u')||(ch=='U')) heval=AN_U; else
        if((ch=='w')||(ch=='W')) heval=AN_W; else
        if((ch=='l')||(ch=='L')) heval=AN_L; else
        /* if(ch=='-') */        heval=AN_DASH;		/* default */
	
	if((n->curr->v.h_val!=heval)||(time==start_time)||(vcd_preserve_glitches)) /* same region == go skip */ 
        	{
		if(n->curr->time==time)
			{
			DEBUG(printf("Warning: Glitch at time ["TTFormat"] Signal [%p], Value [%c->%c].\n",
				time, n, AN_STR[n->curr->v.h_val], ch));
			n->curr->v.h_val=heval;		/* we have a glitch! */

			num_glitches++;
			if(!(n->curr->flags&HIST_GLITCH))
				{
				n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
				num_glitch_regions++;
				}
			}
			else
			{
                	he=histent_calloc();
                	he->time=time;
                	he->v.h_val=heval;

                	n->curr->next=he;
			n->curr=he;
                	regions+=regadd;
			}
                }
       }
}
else
{
switch(ch)
	{
	case 's': /* string */
	{
	if(!(rc=n->curr))
		{
		he=histent_calloc();
		he->flags=(HIST_STRING|HIST_REAL);
	        he->time=-1;
	        he->v.h_vector=NULL;
	
		n->curr=he;
		n->head.next=he;
	
		add_histent_p(time,n,ch,regadd, vector);
		rc = he;
		}
		else
		{
		if(regadd) { time*=(time_scale); }
	
			if(n->curr->time==time)
				{
				DEBUG(printf("Warning: String Glitch at time ["TTFormat"] Signal [%p].\n",
					time, n));
				if(n->curr->v.h_vector) free_2(n->curr->v.h_vector);
				n->curr->v.h_vector=vector;		/* we have a glitch! */
	
				num_glitches++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					num_glitch_regions++;
					}
				}
				else
				{
	                	he=histent_calloc();
				he->flags=(HIST_STRING|HIST_REAL);
	                	he->time=time;
	                	he->v.h_vector=vector;
	
	                	n->curr->next=he;
				n->curr=he;
	                	regions+=regadd;
				}
	       }
	break;
	}
	case 'g': /* real number */
	{
	if(!(rc=n->curr))
		{
		he=histent_calloc();
		he->flags=HIST_REAL;
	        he->time=-1;
	        he->v.h_vector=NULL;
	
		n->curr=he;
		n->head.next=he;
	
		add_histent_p(time,n,ch,regadd, vector);
		rc = he;
		}
		else
		{
		if(regadd) { time*=(time_scale); }
	
		if(
		  (n->curr->v.h_vector&&vector&&(*(double *)n->curr->v.h_vector!=*(double *)vector))
			||(time==start_time)
			||(!n->curr->v.h_vector)
			||(vcd_preserve_glitches)
			) /* same region == go skip */ 
	        	{
			if(n->curr->time==time)
				{
				DEBUG(printf("Warning: Real number Glitch at time ["TTFormat"] Signal [%p].\n",
					time, n));
				if(n->curr->v.h_vector) free_2(n->curr->v.h_vector);
				n->curr->v.h_vector=vector;		/* we have a glitch! */
	
				num_glitches++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					num_glitch_regions++;
					}
				}
				else
				{
	                	he=histent_calloc();
				he->flags=HIST_REAL;
	                	he->time=time;
	                	he->v.h_vector=vector;
	
	                	n->curr->next=he;
				n->curr=he;
	                	regions+=regadd;
				}
	                }
			else
			{
			free_2(vector);
			}
	       }
	break;
	}
	default:
	{
	if(!(rc=n->curr))
		{
		he=histent_calloc();
	        he->time=-1;
	        he->v.h_vector=NULL;
	
		n->curr=he;
		n->head.next=he;
	
		add_histent_p(time,n,ch,regadd, vector);
		rc = he;
		}
		else
		{
		if(regadd) { time*=(time_scale); }
	
		if(
		  (n->curr->v.h_vector&&vector&&(strcmp(n->curr->v.h_vector,vector)))
			||(time==start_time)
			||(!n->curr->v.h_vector)
			||(vcd_preserve_glitches)
			) /* same region == go skip */ 
	        	{
			if(n->curr->time==time)
				{
				DEBUG(printf("Warning: Glitch at time ["TTFormat"] Signal [%p], Value [%c->%c].\n",
					time, n, AN_STR[n->curr->v.h_val], ch));
				if(n->curr->v.h_vector) free_2(n->curr->v.h_vector);
				n->curr->v.h_vector=vector;		/* we have a glitch! */
	
				num_glitches++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					num_glitch_regions++;
					}
				}
				else
				{
	                	he=histent_calloc();
	                	he->time=time;
	                	he->v.h_vector=vector;
	
	                	n->curr->next=he;
				n->curr=he;
	                	regions+=regadd;
				}
	                }
			else
			{
			free_2(vector);
			}
	       }
	break;
	}
	}
}

return(rc);
}


static void add_tail_histents(void)
{
int j;
struct vcdsymbol *v;
hptr rc;


/* do 'x' trailers */

v=vcdsymroot;
while(v)
	{
	if(v->vartype==V_REAL)
		{
		double *d;

		d=malloc_2(sizeof(double));
		*d=1.0;
		rc = add_histent_p(MAX_HISTENT_TIME-1, v->narray[0], 'g', 0, (char *)d);
		v->app_array[0] = rc;
		v->tr_array[0] = v->narray[0]->curr;
		}
	else
	if((v->size==1)||(!atomic_vectors))
	for(j=0;j<v->size;j++)
		{
		rc = add_histent_p(MAX_HISTENT_TIME-1, v->narray[j], 'x', 0, NULL);
		v->app_array[j] = rc;
		v->tr_array[j] = v->narray[j]->curr;
		}
	else
		{
		rc = add_histent_p(MAX_HISTENT_TIME-1, v->narray[0], 'x', 0, (char *)calloc_2(1,sizeof(char)));
		v->app_array[0] = rc;
		v->tr_array[0] = v->narray[0]->curr;
		}

	v=v->next;
	}

v=vcdsymroot;
while(v)
	{
	if(v->vartype==V_REAL)
		{
		double *d;

		d=malloc_2(sizeof(double));
		*d=0.0;
		add_histent_p(MAX_HISTENT_TIME, v->narray[0], 'g', 0, (char *)d);
		}
	else
	if((v->size==1)||(!atomic_vectors))
	for(j=0;j<v->size;j++)
		{
		add_histent_p(MAX_HISTENT_TIME, v->narray[j], 'z', 0, NULL);
		}
	else
		{
		add_histent_p(MAX_HISTENT_TIME, v->narray[0], 'z', 0, (char *)calloc_2(1,sizeof(char)));
		}

	v=v->next;
	}
}

/*******************************************************************************/

static void vcd_build_symbols(void)
{
int j;
int max_slen=-1;
struct sym_chain *sym_chain=NULL, *sym_curr=NULL;
int duphier=0;
char hashdirty;
struct vcdsymbol *v, *vprime;
char *str = NULL;

v=vcdsymroot;
while(v)
	{
	int msi;
	int delta;

		{
		int slen;
		int substnode;

		msi=v->msi;
		delta=((v->lsi-v->msi)<0)?-1:1;
		substnode=0;

		slen=strlen(v->name);
		str=(slen>max_slen)?(wave_alloca((max_slen=slen)+32)):(str); /* more than enough */
		strcpy(str,v->name);

		if(v->msi>=0)
			{
			strcpy(str+slen,vcd_hier_delimeter);
			slen++;
			}

		if((vprime=bsearch_vcd(v->id, strlen(v->id)))!=v) /* hash mish means dup net */
			{
			if(v->size!=vprime->size)
				{
				fprintf(stderr,"ERROR: Duplicate IDs with differing width: %s %s\n", v->name, vprime->name);
				}
				else
				{
				substnode=1;
				}
			}

		if(((v->size==1)||(!atomic_vectors))&&(v->vartype!=V_REAL))
			{
			struct symbol *s = NULL;
	
			for(j=0;j<v->size;j++)
				{
				if(v->msi>=0) 
					{
					if(!vcd_explicit_zero_subscripts)
						sprintf(str+slen,"%d",msi);
						else
						sprintf(str+slen-1,"[%d]",msi);
					}

				hashdirty=0;
				if(symfind(str, NULL))
					{
					char *dupfix=(char *)malloc_2(max_slen+32);
					hashdirty=1;
					DEBUG(fprintf(stderr,"Warning: %s is a duplicate net name.\n",str));

					do sprintf(dupfix, "$DUP%d%s%s", duphier++, vcd_hier_delimeter, str);
						while(symfind(dupfix, NULL));

					strcpy(str, dupfix);
					free_2(dupfix);
					duphier=0; /* reset for next duplicate resolution */
					}
					/* fallthrough */
					{
					s=symadd(str,hashdirty?hash(str):hashcache);
	
					s->n=v->narray[j];
					if(substnode)
						{
						struct Node *n, *n2;
	
						n=s->n;
						n2=vprime->narray[j];
						/* nname stays same */
						n->head=n2->head;
						n->curr=n2->curr;
						/* harray calculated later */
						n->numhist=n2->numhist;
						}
	
					s->n->nname=s->name;
					s->h=s->n->curr;
					if(!firstnode)
						{
						firstnode=curnode=s;
						}
						else
						{
						curnode->nextinaet=s;
						curnode=s;
						}
	
					numfacs++;
					DEBUG(fprintf(stderr,"Added: %s\n",str));
					}
				msi+=delta;
				}

			if((j==1)&&(v->root))
				{
				s->vec_root=(struct symbol *)v->root;		/* these will get patched over */
				s->vec_chain=(struct symbol *)v->chain;		/* these will get patched over */
				v->sym_chain=s;

				if(!sym_chain)
					{
					sym_curr=(struct sym_chain *)calloc_2(1,sizeof(struct sym_chain));
					sym_chain=sym_curr;
					}
					else
					{
					sym_curr->next=(struct sym_chain *)calloc_2(1,sizeof(struct sym_chain));
					sym_curr=sym_curr->next;
					}
				sym_curr->val=s;
				}
			}
			else	/* atomic vector */
			{
			if(v->vartype!=V_REAL)
				{
				sprintf(str+slen-1,"[%d:%d]",v->msi,v->lsi);
				}
				else
				{
				*(str+slen-1)=0;
				}


			hashdirty=0;
			if(symfind(str, NULL))
				{
				char *dupfix=(char *)malloc_2(max_slen+32);
				hashdirty=1;
				DEBUG(fprintf(stderr,"Warning: %s is a duplicate net name.\n",str));

				do sprintf(dupfix, "$DUP%d%s%s", duphier++, vcd_hier_delimeter, str);
					while(symfind(dupfix, NULL));

				strcpy(str, dupfix);
				free_2(dupfix);
				duphier=0; /* reset for next duplicate resolution */
				}
				/* fallthrough */
				{
				struct symbol *s;

				s=symadd(str,hashdirty?hash(str):hashcache);	/* cut down on double lookups.. */

				s->n=v->narray[0];
				if(substnode)
					{
					struct Node *n, *n2;

					n=s->n;
					n2=vprime->narray[0];
					/* nname stays same */
					n->head=n2->head;
					n->curr=n2->curr;
					/* harray calculated later */
					n->numhist=n2->numhist;
					n->ext=n2->ext;
					}
					else
					{
					struct ExtNode *en;
					en=(struct ExtNode *)malloc_2(sizeof(struct ExtNode));
					en->msi=v->msi;
					en->lsi=v->lsi;

					s->n->ext=en;
					}

				s->n->nname=s->name;
				s->h=s->n->curr;
				if(!firstnode)
					{
					firstnode=curnode=s;
					}
					else
					{
					curnode->nextinaet=s;
					curnode=s;
					}

				numfacs++;
				DEBUG(fprintf(stderr,"Added: %s\n",str));
				}
			}
		}

	v=v->next;
	}

if(sym_chain)
	{
	sym_curr=sym_chain;	
	while(sym_curr)
		{
		sym_curr->val->vec_root= ((struct vcdsymbol *)sym_curr->val->vec_root)->sym_chain;

		if ((struct vcdsymbol *)sym_curr->val->vec_chain)
			sym_curr->val->vec_chain=((struct vcdsymbol *)sym_curr->val->vec_chain)->sym_chain;

		DEBUG(printf("Link: ('%s') '%s' -> '%s'\n",sym_curr->val->vec_root->name, sym_curr->val->name, sym_curr->val->vec_chain?sym_curr->val->vec_chain->name:"(END)"));

		sym_chain=sym_curr;
		sym_curr=sym_curr->next;
		free_2(sym_chain);
		}
	}
}

/*******************************************************************************/

static void vcd_cleanup(void)
{
struct slist *s, *s2;

if(slisthier) { free_2(slisthier); slisthier=NULL; }
s=slistroot;
while(s)
	{
	s2=s->next;
	if(s->str)free_2(s->str);
	free_2(s);
	s=s2;
	}

slistroot=slistcurr=NULL; slisthier_len=0;
}

/*******************************************************************************/

TimeType vcd_partial_main(char *fname)
{
int shmid;

pv=rootv=NULL;
vcd_hier_delimeter[0]=hier_delimeter;

errno=0;	/* reset in case it's set for some reason */

yytext=(char *)malloc_2(T_MAX_STR+1);

if(!hier_was_explicitly_set) /* set default hierarchy split char */
	{
	hier_delimeter='.';
	}


if(!strcmp(fname, "-vcd"))
	{
	if(!fscanf(stdin, "%x", &shmid)) shmid = -1; /* allow use of -v flag to pass straight from stdin */
	}
	else
	{
	sscanf(fname, "%x", &shmid);	/* passed as a filename */
	}

#if !defined _MSC_VER && !defined __MINGW32__
errno = 0;
consume_ptr = buf = shmat(shmid, NULL, 0);
if(errno)
	{
	fprintf(stderr, "Could not attach shared memory ID %08x\n", shmid);
	perror("Why");
	exit(255);
	}
#else
fprintf(stderr, "Interactive VCD mode does not work with Windows, exiting.\n");
exit(255);
#endif

getch_alloc();		/* alloc membuff for vcd getch buffer */
build_slisthier();

vcd_preserve_glitches = 1; /* splicing dictates that we override */
while(!header_over) { vcd_parse(); }

if(varsplit)
	{
	free_2(varsplit);
	varsplit=NULL;
	}

if((!sorted)&&(!indexed))
	{
	fprintf(stderr, "No symbols in VCD file..is it malformed?  Exiting!\n");
	exit(1);
	}

add_tail_histents();
vcd_build_symbols();
vcd_sortfacs();
vcd_cleanup();

min_time=start_time*time_scale;
max_time=end_time*time_scale;

if((min_time==max_time)||(max_time==0))
        {
	min_time = max_time = 0;
        }

is_vcd=~0;
partial_vcd = ~0;

#ifdef __linux__
	{
	struct shmid_ds ds;
	shmctl(shmid, IPC_RMID, &ds); /* mark for destroy */
	}
#endif

return(max_time);
}

/*******************************************************************************/

static void regen_harray(Trptr t, nptr nd)
{
int i, histcount;
hptr histpnt;
hptr *harray;

if(!nd->harray)         /* make quick array lookup for aet display */
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
                fprintf( stderr, "Out of memory, can't add %s to analyzer\n",
                        nd->nname );
                free_2(t);
                }
        
        histpnt=&(nd->head);
        for(i=0;i<histcount;i++)
                {
                *harray=histpnt;
                 
                /* printf("%s, time: %d, val: %d\n", nd->nname,   
                        (*harray)->time, (*harray)->val); */
         
                harray++;
                histpnt=histpnt->next; 
                }
        }
}

/* mark vectors that need to be regenerated */
static void regen_trace_mark(Trptr t)
{
if(t->vector)
	{
	bvptr b = t->n.vec;
	bptr bts = b->bits;
	int i;

	for(i=0;i<bts->nbits;i++)
		{
		if(!bts->nodes[i]->harray)
			{
			t->interactive_vector_needs_regeneration = 1;
			return;
			}		
		}
	}
}

/* sweep through and regen nodes/dirty vectors */
static void regen_trace_sweep(Trptr t)
{
if(!t->vector)
	{
	if(t->n.nd) /* comment and blank traces don't have a valid node */
	if(!t->n.nd->harray)
		{
		regen_harray(t, t->n.nd);
		}
	}
else if(t->interactive_vector_needs_regeneration)
	{
	bvptr b = t->n.vec;
	bptr bts = b->bits;
	int i;
	bvptr b2;

	for(i=0;i<bts->nbits;i++)
		{
		if(!bts->nodes[i]->harray)
			{
			regen_harray(t, bts->nodes[i]);
			}		
		}


	if(!bts->name)
		{
		bts->name = "";
		b2 = bits2vector(bts);
		bts->name = NULL;
		}
		else
		{
		b2 = bits2vector(bts);
		}

	t->n.vec = b2;
	b2->bits=bts;

	free_2(b2->name);
	b2->name = b->name;

	for(i=0;i<b->numregions;i++)
		{
		free_2(b->vectors[i]);
		}

	free_2(b);
	}
}

/*******************************************************************************/

void kick_partial_vcd(void)
{
#if !defined _MSC_VER && !defined __MINGW32__

if(partial_vcd)
	{
	struct timeval tv;
	static int timeset = 0;

	tv.tv_sec = 0; 
	tv.tv_usec = 1000000 / 100;
	select(0, NULL, NULL, NULL, &tv);

	while(*consume_ptr)
		{
		Trptr t;
	
		vcd_parse();
	
		min_time=start_time*time_scale;
		max_time=end_time*time_scale;
	
		tims.last=max_time;
		tims.end=tims.last;             /* until the configure_event of wavearea */
	
		if(!timeset)
			{
			tims.first=tims.start=tims.laststart=min_time;
			timeset = 1;
			}
	
		update_endcap_times_for_partial_vcd();
		update_maxmarker_labels();
	
		t = traces.first; while(t) { regen_trace_mark(t); t = t->t_next; }
		t = traces.buffer; while(t) { regen_trace_mark(t); t = t->t_next; }

		t = traces.first; while(t) { regen_trace_sweep(t); t = t->t_next; }
		t = traces.buffer; while(t) { regen_trace_sweep(t); t = t->t_next; }
	
	        signalarea_configure_event(signalarea, NULL);
	        wavearea_configure_event(wavearea, NULL);
		while (gtk_events_pending()) gtk_main_iteration();
		}
	}

#endif

while (gtk_events_pending()) gtk_main_iteration();
}

/*
 * $Id$
 * $Log$
 * Revision 1.3  2007/04/29 06:07:28  gtkwave
 * fixed memory leaks in vcd parser
 *
 * Revision 1.2  2007/04/20 02:08:17  gtkwave
 * initial release
 *
 */

