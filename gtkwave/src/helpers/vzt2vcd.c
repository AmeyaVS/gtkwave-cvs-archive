/*
 * Copyright (c) 2003-2007 Tony Bybell.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <config.h>
#include "vzt_read.h"

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <time.h>

#include "wave_locale.h"

static int flat_earth = 0;
static int vectorize = 0;

extern void free_hier(void);
extern char *output_hier(char *name);


/*
 * generate a vcd identifier for a given facindx
 */
static char *vcdid(int value)
{
static char buf[16];
int i;

for(i=0;i<15;i++)
        {
        buf[i]=(char)((value%94)+33); /* for range 33..126 */
        value=value/94;
        if(!value) {buf[i+1]=0; break;}
        }

return(buf);
}

static char *vcd_truncate_bitvec(char *s)
{
char l, r;

r=*s;
if(r=='1')
        {
        return s;
        }
        else
        {
        s++;
        }

for(;;s++)
        {
        l=r; r=*s;
        if(!r) return (s-1);

        if(l!=r)
                {
                return(((l=='0')&&(r=='1'))?s:s-1);
                }
        }
}


static vztint64_t vcd_prevtime;
char vcd_blackout;

void vcd_callback(struct vzt_rd_trace **lt, vztint64_t *pnt_time, vztint32_t *pnt_facidx, char **pnt_value)
{
struct vzt_rd_geometry *g = vzt_rd_get_fac_geometry(*lt, *pnt_facidx);

/* fprintf(stderr, "%lld %d %s\n", *pnt_time, *pnt_facidx, *pnt_value); */

if(vcd_prevtime != *pnt_time)
	{
	vcd_prevtime = *pnt_time;
	printf("#%lld\n", *pnt_time);
	}

if(!(*pnt_value)[0])
	{
	if(!vcd_blackout)
		{
		vcd_blackout = 1;
		printf("$dumpoff\n");
		}

	return;
	}
	else
	{
	if(vcd_blackout)
		{
		vcd_blackout = 0;
		printf("$dumpon\n");
		}	
	}

if(g->flags & VZT_RD_SYM_F_DOUBLE)
	{
        printf("r%s %s\n", *pnt_value, vcdid(*pnt_facidx));
        }
else
if(g->flags & VZT_RD_SYM_F_STRING)
	{
        printf("s%s %s\n", *pnt_value, vcdid(*pnt_facidx));
        }
else
	{
        if(g->len==1)
        	{
                printf("%c%s\n", (*pnt_value)[0], vcdid(*pnt_facidx));
                }
                else
                {                        
                printf("b%s %s\n", vcd_truncate_bitvec(*pnt_value), vcdid(*pnt_facidx));
                }
	}                               
}


int process_vzt(char *fname)
{
struct vzt_rd_trace *lt;
char *netname;

lt=vzt_rd_init(fname);
if(lt)
	{
	int i;
	int numfacs;
	char time_dimension;
	int time_scale = 1;
	signed char scale;
	time_t walltime;

	if(vectorize) { vzt_rd_vectorize(lt); }
	
	numfacs = vzt_rd_get_num_facs(lt);
	vzt_rd_set_fac_process_mask_all(lt);
	vzt_rd_set_max_block_mem_usage(lt, 0);	/* no need to cache blocks */

	scale = vzt_rd_get_timescale(lt);
        switch(scale)
                {
                case 0:         time_dimension = 's'; break;
         
                case -1:        time_scale = 100; 		time_dimension = 'm'; break;
                case -2:        time_scale = 10;
                case -3:                                        time_dimension = 'm'; break;

                case -4:        time_scale = 100; 		time_dimension = 'u'; break;
                case -5:        time_scale = 10;
                case -6:                                        time_dimension = 'u'; break;
                
                case -10:       time_scale = 100; 		time_dimension = 'p'; break;
                case -11:       time_scale = 10;
                case -12:                                       time_dimension = 'p'; break;
                  
                case -13:       time_scale = 100; 		time_dimension = 'f'; break;
                case -14:       time_scale = 10;
                case -15:                                       time_dimension = 'f'; break;
         
                case -7:        time_scale = 100; 		time_dimension = 'n'; break;
                case -8:        time_scale = 10;
                case -9:
                default:                                        time_dimension = 'n'; break;
                }

        time(&walltime);
        printf("$date\n");
        printf("\t%s",asctime(localtime(&walltime)));
        printf("$end\n");
        printf("$version\n\tvzt2vcd\n$end\n");
	printf("$timescale %d%c%c $end\n", time_scale, time_dimension, !scale ? ' ' : 's');

	for(i=0;i<numfacs;i++)
		{
		struct vzt_rd_geometry *g = vzt_rd_get_fac_geometry(lt, i);
 		vztint32_t newindx = vzt_rd_get_alias_root(lt, i);

		if(vectorize)
			{
	                if(g->len==0) continue;
			}

		if(!flat_earth)
			{
			netname = output_hier(vzt_rd_get_facname(lt, i));
			}
			else
			{
			netname = vzt_rd_get_facname(lt, i);
			}

                if(g->flags & VZT_RD_SYM_F_DOUBLE)
                	{
                        printf("$var real 1 %s %s $end\n", vcdid(newindx), netname);
                        }
                else
                if(g->flags & VZT_RD_SYM_F_STRING)
                       {
                       printf("$var real 1 %s %s $end\n", vcdid(newindx), netname);
                       }
                else
			
                        {
                       	if(g->len==1)
                       		{
                                if(g->msb!=-1)
                                	{
                                        printf("$var wire 1 %s %s[%d] $end\n", vcdid(newindx), netname, g->msb);
                                        }
                                        else
                                        {
                                        printf("$var wire 1 %s %s $end\n", vcdid(newindx), netname);
                                        }  
				}
                                else
                                {
				if(!(g->flags & VZT_RD_SYM_F_INTEGER))
					{
	                                printf("$var wire %d %s %s[%d:%d] $end\n", g->len, vcdid(newindx), netname, g->msb, g->lsb);
					}
					else
					{
	                                printf("$var integer %d %s %s $end\n", g->len, vcdid(newindx), netname);
					}
                                }
                        }
		}

	if(!flat_earth)
		{
		output_hier(""); /* flush any remaining hierarchy if not back to toplevel */
		free_hier();
		}

	printf("$enddefinitions $end\n");
	printf("$dumpvars\n");

	vcd_prevtime = vzt_rd_get_start_time(lt)-1;

	vzt_rd_iter_blocks(lt, vcd_callback, NULL);

	if(vcd_prevtime!=vzt_rd_get_end_time(lt))
		{
		printf("#%lld\n", vzt_rd_get_end_time(lt));
		}

	vzt_rd_close(lt);
	}
	else
	{
	fprintf(stderr, "vzt_rd_init failed\n");
	return(255);
	}

return(0);
}

/*******************************************************************************/

void print_help(char *nam)
{
#ifdef __linux__ 
printf(
"Usage: %s [OPTION]... [VZTFILE]\n\n"
"  -v, --vztname=FILE         specify VZT input filename\n"
"  -f, --flatearth            emit flattened hierarchies\n"
"  -c, --coalesce             coalesce bitblasted vectors\n"
"  -h, --help                 display this help then exit\n\n"
"VCD is emitted to stdout.\n\n"
"Report bugs to <bybell@nc.rr.com>.\n",nam);
#else
printf(
"Usage: %s [OPTION]... [VZTFILE]\n\n"
"  -l                         specify VZT input filename\n"
"  -f                         emit flattened hierarchies\n"
"  -c                         coalesce bitblasted vectors\n"
"  -h                         display this help then exit\n\n"

"VCD is emitted to stdout.\n\n"
"Report bugs to <bybell@nc.rr.com>.\n",nam);
#endif

exit(0);
}


int main(int argc, char **argv)
{
char opt_errors_encountered=0;
char *lxname=NULL;
int c;
int rc;

WAVE_LOCALE_FIX

while (1)
        {
        int option_index = 0;
                        
#ifdef __linux__
        static struct option long_options[] =
                {
		{"vztname", 1, 0, 'v'},
		{"coalesce", 0, 0, 'c'},
		{"flatearth", 0, 0, 'f'},
                {"help", 0, 0, 'h'},
                {0, 0, 0, 0}  
                };
                
        c = getopt_long (argc, argv, "v:cfh", long_options, &option_index);
#else
        c = getopt      (argc, argv, "v:cfh");
#endif
                        
        if (c == -1) break;     /* no more args */
                        
        switch (c)
                {
		case 'v':
			if(lxname) free(lxname);
                        lxname = malloc(strlen(optarg)+1);
                        strcpy(lxname, optarg);
			break;

		case 'c': vectorize=1;
			break;

		case 'f': flat_earth=1;
			break;

                case 'h':
			print_help(argv[0]);
                        break;
                        
                case '?':
                        opt_errors_encountered=1;
                        break;
                        
                default:
                        /* unreachable */
                        break;
                }
        }
                        
if(opt_errors_encountered)
        {
        print_help(argv[0]);
        }

if (optind < argc)
        {               
        while (optind < argc)
                {
                if(!lxname)
                        {
                        lxname = malloc(strlen(argv[optind])+1);
                        strcpy(lxname, argv[optind++]);
                        }
                }
        }
                        
if(!lxname)
        {
        print_help(argv[0]);
        }

rc=process_vzt(lxname);
free(lxname);

return(rc);
}

/*
 * $Id$
 * $Log$
 * Revision 1.2  2007/04/20 02:08:19  gtkwave
 * initial release
 *
 */

