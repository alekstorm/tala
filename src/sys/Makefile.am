AM_CFLAGS = $(COMMON) -I @abs_top_srcdir@/src -std=gnu99
AM_CPPFLAGS = $(COMMON) -I @abs_top_srcdir@/src

SUBDIRS = melder io

noinst_LTLIBRARIES = libsys.la
libsys_la_SOURCES = \
	Collection.cpp \
	complex.cpp \
	Data.cpp \
	Script.cpp \
	Strings.cpp \
	Thing.cpp
