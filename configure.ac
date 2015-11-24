

AC_INIT([SSTCore], [6.0], [sst-users@sst-simulator.org])

AC_PREREQ([2.59])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE([1.9.6 foreign dist-bzip2 subdir-objects no-define tar-ustar])

# If Automake supports silent rules, enable them.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_pattern_allow([LT_LIBEXT])

AC_CONFIG_HEADER([src/sst/core/sst_config.h])

# Lets check for the standard compilers and basic options
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_C99
AC_C_INLINE
AC_PROG_MAKE_SET

AC_PROG_CXX
AC_CACHE_SAVE

AC_PROG_CPP
AC_CANONICAL_HOST

AC_CACHE_SAVE
AC_HEADER_TIME

LT_CONFIG_LTDL_DIR([src/sst/core/libltdl])
LT_INIT([shared disable-static dlopen])
LTDL_INIT([recursive])

if test "x$with_included_ltdl" != "xyes"; then
   save_CFLAGS="$CFLAGS"
   save_LDFLAGS="$LDFLAGS"
   save_LIBS="$LIBS"
   CFLAGS="$CFLAGS $LTDLINCL"
   LDFLAGS="$LDFLAGS $LIBLTDL"
   AC_CHECK_LIB([ltdl], [lt_dladvise_init],
                 [],
                 [AC_MSG_ERROR([installed libltdl is too old])])
   LIBS="$save_LIBS"
   LDFLAGS="$save_LDFLAGS"
   CFLAGS="$save_CFLAGS"
fi

AC_SUBST([LTDLINCL])
AC_SUBST([LIBLTDL])

dnl Fix flags - seems to be only way to make libltdl play nice
CPPFLAGS='-I$(top_builddir) -I$(top_srcdir) -I$(top_srcdir)/src'" $CPPFLAGS"

AC_HEADER_TIME
AC_CHECK_HEADERS([c_asm.h dlfcn.h intrinsics.h mach/mach_time.h sys/time.h sys/stat.h sys/types.h unistd.h])

AC_CACHE_SAVE

SST_CHECK_MPI([], [AC_MSG_ERROR([Could not find MPI package])])
SST_CHECK_LIBZ()

SST_CHECK_ZOLTAN([have_zoltan=1],[have_zoltan=0],[AC_MSG_ERROR([Zoltan requested but not found])])
AS_IF([test "x$have_zoltan" = "x1"], [AC_DEFINE_UNQUOTED([HAVE_ZOLTAN], [1],
	[Define if you have the Zoltan library.])])
AM_CONDITIONAL([HAVE_ZOLTAN], [test "$have_zoltan" = 1])

AC_CACHE_SAVE

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/sst/Makefile
  src/sst/core/Makefile
  src/sst/core/libltdl/Makefile
])
AC_OUTPUT

