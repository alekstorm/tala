AC_PREREQ(2.59) # FIXME should be 2.60
AC_INIT(Praat, 0.1, alek.storm@gmail.com, praat, http://github.com/alekstorm/Praat)
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES([GTK], [gtk+-2.0])
PKG_CHECK_MODULES([FLAC], [flac])
PKG_CHECK_MODULES([MAD], [mad])
PKG_CHECK_MODULES([PA], [portaudio-2.0])
PKG_CHECK_MODULES([GSL], [gsl])
# fix collision between FLAC and system header files by requiring the former to be qualified with a FLAC/ prefix when #include'd
FLAC_CFLAGS=`echo $FLAC_CFLAGS | sed "s/\/FLAC$//"` # FIXME use $SED

AC_ARG_WITH(glpk-include,
            AC_HELP_STRING([--with-glpk-include=PATH],
                           [location of glpk headers [[/usr/local/include]]]
                          ),
            [GLPK_INCLUDE=$withval],
            [GLPK_INCLUDE="/usr/local/include"])

AC_ARG_WITH(glpk-lib, 
            AC_HELP_STRING([--with-glpk-lib=PATH],
                           [location of glpk library [[/usr/local/lib]]]
                          ),
            [GLPK_LIB=$withval],
            [GLPK_LIB="/usr/local/lib"])

GLPK_CFLAGS="-I${GLPK_INCLUDE}"
GLPK_LIBS="-L${GLPK_LIB} -lglpk"
AC_SUBST(GLPK_LIBS)

AC_CHECK_HEADER([${GLPK_INCLUDE}/glpk.h],, 
    AC_MSG_ERROR([Could not find glpk.h in ${GLPK_INCLUDE}: 
      use --with-glpk-include or GLPK_INCLUDE to specify the include path.]))

LDFLAGS="$LDFLAGS $GLPK_LIBS"
AC_CHECK_LIB([glpk], [glp_create_prob], ,
    AC_MSG_ERROR([Could not link to glpk in ${GLPK_LIB}: 
      use --with-glpk-lib or GLPK_LIB to specify the lib path.]))

AC_CHECK_FUNC([glp_print_ranges], ,
    AC_MSG_ERROR([GLPK version must be >= 4.42.]))

AC_ARG_ENABLE([debug],
[  --enable-debug    Turn on debugging],
[case "${enableval}" in
  yes) DEBUG="-g" ;;
  no)  DEBUG="" ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],[debug=false])

COMMON="-DUNIX -Dlinux $DEBUG $GTK_CFLAGS $FLAC_CFLAGS $MAD_CFLAGS $PA_CFLAGS $GSL_CFLAGS $GLPK_CFLAGS -m64 -Wreturn-type -Wunused -Wunused-parameter -Wuninitialized -O"
AC_SUBST([COMMON])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h fenv.h float.h inttypes.h limits.h locale.h mach/mach.h malloc.h memory.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h unistd.h utime.h wchar.h wctype.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_FSEEKO
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([alarm bzero clock_gettime fesetround floor getcwd gethostbyname gettimeofday memchr memmove memset mkdir pow select setlocale socket sqrt strchr strdup strerror strrchr strstr strtol strtoul utime])

AC_PROG_LIBTOOL
#AC_PROG_SED # FIXME use this when upgraded to 2.60

AC_CONFIG_FILES([Makefile
                  src/Makefile
                  src/artsynth/Makefile
                  src/ola/Makefile
                  src/FFNet/Makefile
                  src/LPC/Makefile
                  src/dwsys/Makefile
                  src/dwtools/Makefile
                  src/fon/Makefile
                  src/gram/Makefile
                  src/kar/Makefile
                  src/num/Makefile
                  src/stat/Makefile
                  src/sys/Makefile
                  src/sys/io/Makefile
                  src/sys/melder/Makefile
                  src/ui/Makefile
                  src/ui/editors/Makefile
                  src/ui/init/Makefile
                  src/ui/sendpraat/Makefile])
AM_INIT_AUTOMAKE(praat, 0.1)
AC_OUTPUT
