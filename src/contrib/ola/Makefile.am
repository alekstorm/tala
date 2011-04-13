AM_CFLAGS = $(COMMON) -I @abs_top_srcdir@/src -std=gnu99
AM_CPPFLAGS = $(COMMON) -I @abs_top_srcdir@/src

noinst_LIBRARIES = libOla.a
libOla_a_SOURCES = KNN.c \
   KNN_threads.c Pattern_to_Categories_cluster.c KNN_prune.c FeatureWeights.c praat_contrib_Ola_KNN.c
