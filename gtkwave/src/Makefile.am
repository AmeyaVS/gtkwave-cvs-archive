## -*- makefile -*-
##

SUBDIRS=	$(LIBZ_DIR) $(LIBBZ2_DIR) helpers
DIST_SUBDIRS=	libz libbz2 helpers

AET2LIB = 
AIXFLAGS = -bmaxdata:0xd0000000/dsa -D_WAVE_BE32
BIGFILES = -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
#CFLAGS = -O2 -DWAVE_USE_GTK2 -I./libz -I./libbz2 -I./helpers -I../
#OBSOLETE_CFLAGS = -DGTK_ENABLE_BROKEN
#DEBUGS = -DDEBUG_FACILITIES -DDEBUG_PRINTF -DDEBUG_MALLOC -DSTRICT_VCD_ONLY -DDEBUG_MALLOC_LINES

bin_PROGRAMS=	gtkwave twinwave

AM_CFLAGS=	-I$(srcdir)/.. -I$(srcdir)/helpers $(GTK_CFLAGS) $(LIBZ_CFLAGS) $(LIBBZ2_CFLAGS)

gtkwave_SOURCES= \
	ae2.c ae2.h analyzer.c analyzer.h baseconvert.c bitvec.c \
	bsearch.c bsearch.h busy.c busy.h \
	clipping.c clipping.h color.c color.h currenttime.c currenttime.h \
	debug.c debug.h discardbuttons.c \
	entry.c fetchbuttons.c fgetdynamic.c fgetdynamic.h file.c fonts.c getopt.c \
	getopt1.c ghw.c ghw.h ghwlib.c ghwlib.h gnu-getopt.h gnu_regex.h gtk12compat.h \
	help.c helpers/lxt2_read.c \
	helpers/lxt_write.c helpers/vzt_read.c hiersearch.c \
	logfile.c lx2.c lx2.h lxt.c lxt.h main.c main.h markerbox.c menu.c menu.h mouseover.c \
	pagebuttons.c pipeio.c pipeio.h pixmaps.c pixmaps.h print.c print.h \
	ptranslate.c ptranslate.h rc.c rc.h \
	regex.c regex_wave.h renderopt.c rgb.c search.c shiftbuttons.c showchange.c \
	signalwindow.c simplereq.c splash.c status.c strace.c strace.h symbol.c symbol.h \
	timeentry.c translate.c translate.h tree.c tree.h treesearch.c vcd.c vcd.h vcd_keywords.c \
	vcd_partial.c vcd_recoder.c vcd_saver.c vcd_saver.h vlist.c vlist.h vzt.c vzt.h wavealloca.h \
	wavewindow.c zoombuttons.c

gtkwave_LDFLAGS= $(GTK_LIBS) $(LIBZ_LDADD) $(LIBBZ2_LDADD)

twinwave_SOURCES= twinwave.c
twinwave_LDFLAGS= $(GTK_LIBS)

vcd_keywords.c: vcd_keywords.gperf
	printf "$(GPERF) -o -i 1 -C -k 1,\044 -L C -H keyword_hash -N check_identifier -tT $(srcdir)/vcd_keywords.gperf >vcd_keywords.c" | sh

BUILT_SOURCES= vcd_keywords.c

# I'm listing treesearch_gtk2.c here instead of in SOURCES because we don't directly
# compile it.  Rather it is #include'd by treesearch.c.
EXTRA_DIST= treesearch_gtk2.c vcd_keywords.gperf

