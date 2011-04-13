AM_CFLAGS = $(COMMON) -I @abs_top_srcdir@/src -std=gnu99
AM_CPPFLAGS = $(COMMON) -I @abs_top_srcdir@/src

noinst_LIBRARIES = libdwsys.a
libdwsys_a_SOURCES = Collection_extensions.c Command.c \
	Eigen.c Index.c \
	NUM2.c NUMhuber.c NUMlapack.c NUMmachar.c \
	NUMf2c.c NUMcblas.c NUMclapack.c NUMfft_d.c NUMsort2.c \
	Permutation.c Permutation_and_Index.c \
	regularExp.c SimpleVector.c Simple_extensions.c \
	SVD.c
