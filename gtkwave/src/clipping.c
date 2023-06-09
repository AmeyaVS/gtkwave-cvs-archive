/*
 * Copyright (c) Tony Bybell 2005
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>

#define x1 coords[0]
#define y1 coords[1]
#define x2 coords[2]
#define y2 coords[3]

#define rx1 rect[0]
#define ry1 rect[1]
#define rx2 rect[2]
#define ry2 rect[3]

/* function currently unused...for future use */

int wave_lineclip(int *coords, int *rect)
{
int msk1, msk2;

return(1);

msk1 = (x1<rx1);
msk1|= (x1>rx2)<<1;
msk1|= (y1<ry1)<<2;
msk1|= (y1>ry2)<<3;

msk2 = (x2<rx1);
msk2|= (x2>rx2)<<1;
msk2|= (y2<ry1)<<2;
msk2|= (y2>ry2)<<3;

if(!(msk1|msk2)) return(1); /* trivial accept, all points are inside rectangle */
if(msk1&msk2) return(0);    /* trivial reject, common x or y out of range */

if(y1==y2)
	{
	if(x1<rx1) x1 = rx1; else if(x1>rx2) x1 = rx2;
	if(x2<rx1) x2 = rx1; else if(x2>rx2) x2 = rx2;
	}
else
if(x1==x2)
	{
	if(y1<ry1) y1 = ry1; else if(y1>ry2) y1 = ry2;
	if(y2<ry1) y2 = ry1; else if(y2>ry2) y2 = ry2;
	}
else
	{
	float m = (y2-y1)/(x2-x1);
	float b = y1 - m*x1;

	if(x1<rx1) { x1 = rx1; y1 = m*x1 + b; }
	else if(x1>rx2) { x1 = rx2; y1 = m*x1 + b; }

	if(y1<ry1) { y1 = ry1; x1 = (y1 - b) / m; } 
	else if(y1>ry2) { y1 = ry2; x1 = (y1 - b) / m; }

	if(x2<rx1) { x2 = rx1; y2 = m*x2 + b; }
	else if(x2>rx2) { x2 = rx2; y2 = m*x2 + b; }

	if(y2<ry1) { y2 = ry1; x2 = (y2 - b) / m; } 
	else if(y2>ry2) { y2 = ry2; x2 = (y2 - b) / m; }
	}

msk1 = (x1<rx1);
msk1|= (x1>rx2)<<1;
msk1|= (y1<ry1)<<2;
msk1|= (y1>ry2)<<3;

msk2 = (x2<rx1);
msk2|= (x2>rx2)<<1;
msk2|= (y2<ry1)<<2;
msk2|= (y2>ry2)<<3;

return(!msk1 && !msk2); /* see if points are really inside */
}

/*
 * $Id$
 * $Log$
 */

