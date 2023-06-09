/* 
 * Copyright (c) Tony Bybell 1999-2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

/*
 * tree.h 12/05/98ajb
 */
#ifndef WAVE_TREE_H
#define WAVE_TREE_H

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "debug.h"
#include "symbol.h"
#include "vcd.h"
#include "tree_component.h"


/* Kind of the tree.  */
enum tree_kind
 {
   /* Unknown.  */
   TREE_UNKNOWN,

   /* An internal signal.  */
   TREE_SIGNAL,

   /* An in/out/inout signal.  */
   TREE_IN,
   TREE_OUT,
   TREE_INOUT,

   /* An element of a vector.  */
   TREE_VECTOREL,
   /* An element of a record.  */
   TREE_RECORDEL,

   /* A Subinstance.  */
   TREE_INSTANCE,

   /* A package (somewhat VHDL specific ?).  */
   TREE_PACKAGE,

   /* A base (source file).  Not yet implemented.  */
   TREE_BASE,

   /* Verilog scope types */
   TREE_VCD_ST_MODULE,
   TREE_VCD_ST_TASK,
   TREE_VCD_ST_FUNCTION,
   TREE_VCD_ST_BEGIN,
   TREE_VCD_ST_FORK,

   /* VHDL scope types */
   TREE_VHDL_ST_DESIGN,
   TREE_VHDL_ST_BLOCK,
   TREE_VHDL_ST_GENIF,
   TREE_VHDL_ST_GENFOR,
   TREE_VHDL_ST_INSTANCE,
   TREE_VHDL_ST_PACKAGE,

   /* VHDL signal types (still as part of scope in GHW) */
   TREE_VHDL_ST_SIGNAL,
   TREE_VHDL_ST_PORTIN,
   TREE_VHDL_ST_PORTOUT,
   TREE_VHDL_ST_PORTINOUT,
   TREE_VHDL_ST_BUFFER,
   TREE_VHDL_ST_LINKAGE

 };

#define WAVE_T_WHICH_UNDEFINED_COMPNAME (-1)
#define WAVE_T_WHICH_COMPNAME_START (-2)

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

struct tree
{
struct tree *next;
struct tree *child;
int t_which;		/* 'i' for facs[i] table, value of < 0 means not a full signame */

unsigned kind : 7; 	/* Kind of the leaf: ghwlib reads this as val & 0x7f so only 7 bits needed */
unsigned children_in_gui : 1; /* indicates that the child notes are in the gtk2 tree */
char name[1];
};

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif

/* names at the end of the main hierarchy 010104ajb    */

struct treechain
{
struct tree *tree;	/* top of list of selected item in hierarchy */
struct tree *label;	/* actual selected item in hierarchy */
struct treechain *next;
};


struct autocoalesce_free_list
{
struct autocoalesce_free_list *next;	/* list of coalesced names generated by treesearch gadget..only user of this struct */
char *name;				/* free up next time filtering is performed */
};



void init_tree(void);
void build_tree_from_name(const char *s, int which);
int treegraft(struct tree **t);
void treedebug(struct tree *t, char *s);
void maketree(GtkCTreeNode *subtree, struct tree *t);
#if WAVE_USE_GTK2
void maketree2(GtkCTreeNode *subtree, struct tree *t, int depth, GtkCTreeNode *graft);
#endif

char *leastsig_hiername(char *nam);
void allocate_and_decorate_module_tree_node(unsigned char ttype, const char *scopename, const char *compname, uint32_t scopename_len, uint32_t compname_len);


void treesort(struct tree *t, struct tree *p);
void order_facs_from_treesort(struct tree *t, void *v);

void treenamefix(struct tree *t);


#ifdef WAVE_USE_STRUCT_PACKING
#define WAVE_TALLOC_POOL_SIZE (64 * 1024)
#define WAVE_TALLOC_ALTREQ_SIZE (4 * 1024)
struct tree *talloc_2(size_t siz);
#else
#define talloc_2(x) calloc_2(1,(x))
#endif

#endif

/*
 * $Id$
 * $Log$
 * Revision 1.13  2011/01/19 16:18:19  gtkwave
 * fix for large allocations of tree alloc
 *
 * Revision 1.12  2011/01/19 06:36:31  gtkwave
 * added tree allocation pool when misaligned structs are enabled
 *
 * Revision 1.11  2011/01/18 00:00:12  gtkwave
 * preliminary tree component support
 *
 * Revision 1.10  2011/01/17 19:24:21  gtkwave
 * tree modifications to support decorated internal hierarchy nodes
 *
 * Revision 1.9  2010/12/17 06:29:20  gtkwave
 * Added --enable-struct-pack configure flag
 *
 * Revision 1.8  2010/09/23 22:04:55  gtkwave
 * added incremental SST build code
 *
 * Revision 1.7  2009/07/01 21:58:32  gtkwave
 * more GHW module type adds for icons in hierarchy window
 *
 * Revision 1.6  2009/07/01 18:22:35  gtkwave
 * added VHDL (GHW) instance types as icons
 *
 * Revision 1.5  2009/07/01 16:47:47  gtkwave
 * move decorated module alloc routine to tree.c
 *
 * Revision 1.4  2009/07/01 07:39:12  gtkwave
 * decorating hierarchy tree with module type info
 *
 * Revision 1.3  2009/06/29 18:16:23  gtkwave
 * adding framework for module type annotation on inner tree nodes
 *
 * Revision 1.2  2007/08/26 21:35:46  gtkwave
 * integrated global context management from SystemOfCode2007 branch
 *
 * Revision 1.1.1.1.2.2  2007/08/25 19:43:46  gtkwave
 * header cleanups
 *
 * Revision 1.1.1.1.2.1  2007/08/05 02:27:24  kermin
 * Semi working global struct
 *
 * Revision 1.1.1.1  2007/05/30 04:28:00  gtkwave
 * Imported sources
 *
 * Revision 1.2  2007/04/20 02:08:17  gtkwave
 * initial release
 *
 */

