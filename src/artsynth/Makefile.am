AM_CFLAGS = $(COMMON) -I @abs_top_srcdir@/src -std=gnu99
AM_CPPFLAGS = $(COMMON) -I @abs_top_srcdir@/src

noinst_LIBRARIES = libartsynth.a
libartsynth_a_SOURCES = Speaker.cpp Articulation.cpp Artword.cpp \
     Art_Speaker.cpp Art_Speaker_to_VocalTract.cpp Artword_Speaker.cpp Artword_Speaker_Sound.cpp \
     Artword_Speaker_to_Sound.cpp Artword_to_Art.cpp \
     Delta.cpp Speaker_to_Delta.cpp Art_Speaker_Delta.cpp \
     ArtwordEditor.cpp praat_Artsynth.cpp
