/*
 * Copyright (c) 2006 Tony Bybell.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "wave_locale.h"

/* size *must* match in gtkwave */
#define WAVE_PARTIAL_VCD_RING_BUFFER_SIZE (1024*1024)

char *buf_top, *buf_curr, *buf;
char *consume_ptr;


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

void put_8(char *p, unsigned int v)
{
if(p >= (buf + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE))
        {
        p -= WAVE_PARTIAL_VCD_RING_BUFFER_SIZE;
        }

*p = (unsigned char)v;
}

void put_32(char *p, unsigned int v)
{
put_8(p++, (v>>24));
put_8(p++, (v>>16));
put_8(p++, (v>>8));
put_8(p,   (v>>0));
}

int consume(void)	/* for testing only...similar code also is on the receiving end in gtkwave */
{
char mybuff[32769];
int rc;

if((rc = *consume_ptr))
	{
	unsigned int len = get_32(consume_ptr+1);
	int i;

	for(i=0;i<len;i++)
		{
		mybuff[i] = get_8(consume_ptr+i+5);
		}
	mybuff[i] = 0;
	printf("%s", mybuff);

	*consume_ptr = 0;
	consume_ptr = consume_ptr+i+5;
	if(consume_ptr >= (buf + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE))
	        {
	        consume_ptr -= WAVE_PARTIAL_VCD_RING_BUFFER_SIZE;
	        }
	}

return(rc);
}


void emit_string(char *s)
{
int len = strlen(s);
long l_top, l_curr;
int consumed;
int blksiz;

for(;;)
	{
	while(!*buf_top)
		{
		if((blksiz = get_32(buf_top+1)))
			{
			buf_top += 1 + 4 + blksiz;
			if(buf_top >= (buf + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE))
			        {  
			        buf_top -= WAVE_PARTIAL_VCD_RING_BUFFER_SIZE;
			        }
			}
			else
			{
			break;
			}
		}

	l_top = (long)buf_top;
	l_curr = (long)buf_curr;

	if(l_curr >= l_top)
		{
		consumed = l_curr - l_top;
		}
		else
		{
		consumed = (l_curr + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE) - l_top;
		}
	
	if((consumed + len + 16) > WAVE_PARTIAL_VCD_RING_BUFFER_SIZE) /* just a guardband, it's oversized */
		{
		struct timeval tv;
	
	        tv.tv_sec = 0;
	        tv.tv_usec = 1000000 / 100;
	        select(0, NULL, NULL, NULL, &tv);
		continue;
		}
		else
		{
		char *ss, *sd;
		put_32(buf_curr + 1, len);
		
		sd = buf_curr + 1 + 4;
		ss = s;
		while(*ss)
			{
			put_8(sd, *ss);
			ss++;
			sd++;
			}
		put_8(sd, 0);	/* next valid */
		put_32(sd+1, 0);	/* next len */
		put_8(buf_curr, 1); /* current valid */

                buf_curr += 1 + 4 + len;
                if(buf_curr >= (buf + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE))
                        {
                        buf_curr -= WAVE_PARTIAL_VCD_RING_BUFFER_SIZE;
                        }

		break;
		}
	}
}


/*
 * example driver code.  this merely copies from stdin to the shared memory block.
 * emit_string() will ensure that buffer overruns do not occur; all you have to
 * do is write the block with the provision that the last character in the block is 
 * a newline so that the VCD parser doesn't get lost.  (in effect, when we run out
 * of buffer, gtkwave thinks it's EOF, but we restart again later.  if the last
 * character is a newline, we EOF on a null string which is OK.)
 * the shared memory ID will print on stdout.  pass that on to gtkwave for reading.
 */
int main(int argc, char **argv)
{
int consuming = 0;
int shmid = shmget(0, WAVE_PARTIAL_VCD_RING_BUFFER_SIZE, IPC_CREAT | 0600 );
struct shmid_ds ds;
char l_buf[32769];
char *old_buf = "dummy";
FILE *f;

WAVE_LOCALE_FIX

if(argc != 1)
	{
	f = fopen(argv[1], "rb");
	if(!f)
		{
		fprintf(stderr, "Could not open '%s', exiting.\n", argv[1]);
		perror("Why");
		exit(255);
		}
	}
	else
	{
	f = stdin;
	}

if(shmid >= 0)
	{
	buf_top = buf_curr = buf = shmat(shmid, NULL, 0);
	memset(buf, 0, WAVE_PARTIAL_VCD_RING_BUFFER_SIZE);

#ifdef __linux__
	shmctl(shmid, IPC_RMID, &ds); /* mark for destroy, linux allows queuing up destruction now */
#endif

	printf("%08X\n", shmid);
	fflush(stdout);

	consume_ptr = buf;

	while(fgets(l_buf, 32768, f))
		{
		/* all writes must have an end of line character for gtkwave's VCD reader */
		emit_string((old_buf = l_buf));
		if(!*buf) { consuming = 1; }
		}

	while(!consuming)
		{
                struct timeval tv;
         
                tv.tv_sec = 0;
                tv.tv_usec = 1000000 / 5;
                select(0, NULL, NULL, NULL, &tv);

		if((!*buf)||(!*old_buf)) { consuming = 1; }
		}


#ifndef __linux__
	shmctl(shmid, IPC_RMID, &ds); /* mark for destroy */
#endif
	}

return(0);
}

/*
 * $Id$
 * $Log$
 * Revision 1.2  2007/04/20 02:08:18  gtkwave
 * initial release
 *
 */

