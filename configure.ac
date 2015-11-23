
AC_INIT([SSTCore], [6.0], [sst-users@sst-simulator.org])

AC_PREREQ([2.59])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE([1.9.6 foreign dist-bzip2 subdir-objects no-define tar-ustar])

# If Automake supports silent rules, enable them.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_pattern_allow([LT_LIBEXT])

AC_CONFIG_HEADER([src/sst_config.h])

# Lets check for the standard compilers and basic options
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_C99
AC_C_INLINE

AC_PROG_CXX
AC_CACHE_SAVE

AC_PROG_CPP
AC_CANONICAL_HOST

AC_CACHE_SAVE
AC_HEADER_TIME

LT_CONFIG_LTDL_DIR([sst/core/libltdl])
LT_INIT([shared disable-static dlopen])
LTDL_INIT([recursive])

AC_SUBST([LTDLINCL])
AC_SUBST([LIBLTDL])

AC_CACHE_SAVE

AC_CONFIG_FILES([
  Makefile
  src/Makefile
])
AC_OUTPUT

