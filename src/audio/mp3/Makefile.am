AM_CFLAGS = $(COMMON) -I @abs_top_srcdir@/src -std=gnu99
AM_CPPFLAGS = $(COMMON) -I @abs_top_srcdir@/src

noinst_LIBRARIES = libmp3.a
libmp3_a_SOURCES = mp3.c \
	mad_bit.c \
	mad_decoder.c \
	mad_fixed.c \
	mad_frame.c \
	mad_huffman.c \
	mad_layer12.c \
	mad_layer3.c \
	mad_stream.c \
	mad_synth.c \
	mad_timer.c \
	mad_version.c
