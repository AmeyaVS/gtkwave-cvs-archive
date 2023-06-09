/* 
 * Copyright (c) Tony Bybell 1999-2006.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include <string.h>
#include <ctype.h>
#include "fgetdynamic.h"
#include "debug.h"

int fgetmalloc_len;

char *fgetmalloc(FILE *handle)
{
struct vlist_t *v;
char *pnt;
int i, ch;

v = vlist_create(sizeof(char), 0);

for(;;)
	{
	ch=fgetc(handle);
	if((ch==EOF)||(ch==0x00)||(ch=='\n')||(ch=='\r')) break;

	pnt = (char *)vlist_alloc(&v, 0);
	*pnt = (char)ch;
	}

fgetmalloc_len = vlist_size(v);

if(!fgetmalloc_len) 
	{
	pnt = NULL;
	}
	else
	{
	pnt=malloc_2(fgetmalloc_len+1);
	for(i=0;i<fgetmalloc_len;i++)
		{
		pnt[i] = *((char *)vlist_locate(v, i));
		}
	pnt[i] = 0;
	}

vlist_destroy(v);
return(pnt);
}


/*
 * remove any leading and trailing spaces
 */
char *fgetmalloc_stripspaces(FILE *handle)
{
char *s = fgetmalloc(handle);
int len;

if(s) 
	{
	char *s2 = s + strlen(s) - 1;
	while(isspace(*s2) && (s2 != s)) { *s2 = 0; s2--; }

	s2 = s;
	while(*s2 && isspace(*s2)) { s2++; }

	if((len = strlen(s2)))
		{
		char *s3 = malloc_2(len + 1);
		strcpy(s3, s2);
		free_2(s);
		s = s3;

		fgetmalloc_len = len;
		}
		else
		{
		free_2(s); s = NULL;
		fgetmalloc_len = 0;
		}
	}

return(s);
}

/*
 * $Id$
 * $Log$
 */

