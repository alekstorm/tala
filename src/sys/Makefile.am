AM_CFLAGS = $(COMMON) -I @abs_top_srcdir@/src -std=gnu99
AM_CPPFLAGS = $(COMMON) -I @abs_top_srcdir@/src

SUBDIRS = melder io

noinst_LIBRARIES = libsys.a
libsys_a_SOURCES = \
	Collection.cpp \
	complex.cpp \
	Data.cpp \
	Script.cpp \
	Simple.cpp \
	Strings.cpp \
	Thing.cpp
