AM_CFLAGS = $(COMMON) -I @abs_top_srcdir@/src -std=gnu99
AM_CPPFLAGS = $(COMMON) -I @abs_top_srcdir@/src

noinst_LIBRARIES = libLPC.a
libLPC_a_SOURCES = Cepstrum.c Cepstrumc.c Cepstrum_and_Spectrum.c \
	LPC.c LPC_and_Cepstrumc.c LPC_and_Formant.c LPC_and_LFCC.c \
	LPC_and_Polynomial.c \
	LPC_to_Spectrum.c  LPC_to_Spectrogram.c \
	LPC_and_Tube.c \
	Sound_and_LPC.c  Sound_and_LPC_robust.c \
	Sound_and_Cepstrum.c Tube.c
