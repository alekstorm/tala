AM_CFLAGS = $(COMMON) -I @abs_top_srcdir@/src -std=gnu99
AM_CPPFLAGS = $(COMMON) -I @abs_top_srcdir@/src

noinst_LIBRARIES = libaudio.a
libaudio_a_SOURCES = pa_unix_hostapis.c pa_linux_alsa.c \
	pa_front.c pa_unix_util.c pa_debugprint.c pa_cpuload.c \
	pa_allocation.c pa_process.c pa_converters.c pa_dither.c \
	pa_stream.c
