/* 
 * Copyright (c) Tony Bybell 1999-2005.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/* example-start menu menufactory.h */

#ifndef __MENUFACTORY_H__
#define __MENUFACTORY_H__

#include <gtk/gtk.h>
#include <stdio.h>

#ifndef _MSC_VER
	#include <strings.h>
#endif

#include <errno.h>
#include "currenttime.h"
#include "fgetdynamic.h"
#include "strace.h"
#include "debug.h"
#include "symbol.h"
#include "main.h"

void get_main_menu (GtkWidget *, GtkWidget **menubar);
int file_quit_cmd_callback (GtkWidget *widget, gpointer data);
int set_wave_menu_accelerator(char *str);

int execute_script(char *name);
FILE *script_handle;

extern char *filesel_writesave;

#endif

/*
 * $Id$
 * $Log$
 */

