LIBS = $(GTK_LIBS) $(FLAC_LIBS) $(MAD_LIBS) $(PA_LIBS) $(GSL_LIBS) $(GLPK_LIBS) -lm -lasound

RANLIB = ls
ICON = # FIXME icons
MAIN_ICON =

SUBDIRS = num kar sys stat fon dwsys dwtools LPC gram FFNet artsynth ola ui

AM_CFLAGS = $(COMMON) -I @abs_top_srcdir@/src -std=gnu99
AM_CPPFLAGS = $(COMMON) -I @abs_top_srcdir@/src

bin_PROGRAMS = praat
lib_LTLIBRARIES = libpraat.la

libpraat_la_SOURCES =
# Dummy C++ source to cause C++ linking.
nodist_EXTRA_libpraat_la_SOURCES = dummy.cxx
libpraat_la_LIBADD = \
	fon/libfon.la \
	ola/libOla.la \
	LPC/libLPC.la \
	FFNet/libFFNet.la \
	gram/libgram.la \
	dwtools/libdwtools.la \
	stat/libstat.la \
	dwsys/libdwsys.la \
	sys/libsys.la \
	sys/melder/libmelder.la \
	sys/io/libio.la \
	num/libnum.la \
	artsynth/libartsynth.la \
	kar/libkar.la

praat_SOURCES = ui/main_Praat.cpp
praat_LDADD = \
	ui/init/libinit.la \
	ui/editors/libeditors.la \
	ui/sendpraat/libsendpraat.la \
	ui/libui.la \
	libpraat.la
