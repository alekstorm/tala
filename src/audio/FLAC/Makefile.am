AM_CFLAGS = $(COMMON) -I @abs_top_srcdir@/src -std=gnu99
AM_CPPFLAGS = $(COMMON) -I @abs_top_srcdir@/src

noinst_LIBRARIES = libFLAC.a
libFLAC_a_SOURCES = flac_bitmath.c \
	flac_bitreader.c \
	flac_bitwriter.c \
	flac_cpu.c \
	flac_crc.c \
	flac_fixed.c \
	flac_float.c \
	flac_format.c \
	flac_lpc.c \
	flac_md5.c \
	flac_memory.c \
	flac_metadata_iterators.c \
	flac_metadata_object.c \
	flac_stream_decoder.c \
	flac_stream_encoder.c \
	flac_stream_encoder_framing.c \
	flac_window.c
