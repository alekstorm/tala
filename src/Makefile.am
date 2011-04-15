LIBS = $(GTK_LIBS) $(FLAC_LIBS) $(MAD_LIBS) $(PA_LIBS) $(GSL_LIBS) -lm -lasound

RANLIB = ls
ICON = # FIXME icons
MAIN_ICON =

SUBDIRS = num num/glpk kar mp3 sys stat fon dwsys dwtools LPC gram FFNet artsynth ola

AM_CFLAGS = $(COMMON) -I @abs_top_srcdir@/src -std=gnu99
AM_CPPFLAGS = $(COMMON) -I @abs_top_srcdir@/src

bin_PROGRAMS = praat

praat_SOURCES = main/main_Praat.cpp
praat_LDADD = fon/libfon.a ola/libOla.a \
		LPC/libLPC.a FFNet/libFFNet.a gram/libgram.a dwtools/libdwtools.a \
		artsynth/libartsynth.a fon/libfon.a stat/libstat.a dwsys/libdwsys.a \
		sys/libsys.a num/libnum.a num/glpk/libglpk.a kar/libkar.a mp3/libmp3.a
