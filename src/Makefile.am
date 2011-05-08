LIBS = $(GTK_LIBS) $(FLAC_LIBS) $(MAD_LIBS) $(PA_LIBS) $(GSL_LIBS) $(GLPK_LIBS) -lm -lasound

RANLIB = ls
ICON = # FIXME icons
MAIN_ICON =

SUBDIRS = num kar sys stat fon dwsys dwtools LPC gram FFNet artsynth ola ui

AM_CFLAGS = $(COMMON) -I @abs_top_srcdir@/src -std=gnu99
AM_CPPFLAGS = $(COMMON) -I @abs_top_srcdir@/src

bin_PROGRAMS = praat

praat_SOURCES = ui/main_Praat.cpp
praat_LDADD = \
	artsynth/libartsynth.a \
	ui/init/libinit.a \
	ui/libui.a \
	ui/editors/libeditors.a \
	ui/sendpraat/libsendpraat.a \
	fon/libfon.a \
	ui/libui.a \
	ola/libOla.a \
	LPC/libLPC.a \
	FFNet/libFFNet.a \
	gram/libgram.a \
	dwtools/libdwtools.a \
	artsynth/libartsynth.a \
	stat/libstat.a \
	dwsys/libdwsys.a \
	sys/libsys.a \
	num/libnum.a \
	kar/libkar.a
