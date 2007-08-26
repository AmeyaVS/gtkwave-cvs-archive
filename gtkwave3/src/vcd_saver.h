/*
 * Copyright (c) Tony Bybell 2005.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef VCD_SAVER_H
#define VCD_SAVER_H

#include "vcd.h"
#include "strace.h"

enum vcd_export_typ { WAVE_EXPORT_VCD, WAVE_EXPORT_LXT };
enum vcd_saver_rc { VCDSAV_OK, VCDSAV_EMPTY, VCDSAV_FILE_ERROR };

int save_nodes_to_export(const char *fname, int export_typ);

/* from helpers/scopenav.c */
extern void free_hier(void);
extern char *output_hier(char *name);

#endif

/*
 * $Id$
 * $Log$
 * Revision 1.1.1.1.2.2  2007/08/25 19:43:46  gtkwave
 * header cleanups
 *
 * Revision 1.1.1.1.2.1  2007/08/05 02:27:28  kermin
 * Semi working global struct
 *
 * Revision 1.1.1.1  2007/05/30 04:27:50  gtkwave
 * Imported sources
 *
 * Revision 1.2  2007/04/20 02:08:18  gtkwave
 * initial release
 *
 */

