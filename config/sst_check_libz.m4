AC_DEFUN([SST_CHECK_LIBZ],
[
  sst_check_libz_happy="yes"

  AC_ARG_WITH([libz],
    [AS_HELP_STRING([--with-libz@<:@=DIR@:>@],
      [Use libz (compression routines) found in DIR])])

  AS_IF([test "$with_libz" = "no"], [sst_check_libz_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test "$sst_check_libz_happy" = "yes"], [
    AS_IF([test ! -z "$with_libz" -a "$with_libz" != "yes"],
      [LIBZ_CPPFLAGS="-I$with_libz/include"
       CPPFLAGS="$LIBZ_CPPFLAGS $CPPFLAGS"
       LIBZ_LDFLAGS="-L$with_libz/lib"
       LIBZ_LIBS="-lz",
       LDFLAGS="$LIBZ_LDFLAGS $LDFLAGS"],
      [LIBZ_CPPFLAGS=
       LIBZ_LDFLAGS=
       LIBZ_LIBS=])])
  
  AC_LANG_PUSH([C++])
  AC_CHECK_HEADER([zlib.h], [], [sst_check_libz_happy="no"])
  AC_LANG_POP([C++])

  AS_IF([test "$sst_check_libz_happy" != "no"], [AC_CHECK_LIB([z], [gzopen], [LIBZ_LIBS="-lz"], [sst_check_libz_happy="no"])])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([LIBZ_CPPFLAGS])
  AC_SUBST([LIBZ_LDFLAGS])
  AC_SUBST([LIBZ_LIBS])
  AS_IF([test "x$sst_check_libz_happy" = "xyes"], [AC_DEFINE([HAVE_LIBZ], [1], [Defines whether we have the libz library])])
  AM_CONDITIONAL([USE_LIBZ], [test "x$sst_check_libz_happy" = "xyes"])

  AC_MSG_CHECKING([for libz compression library])
  AC_MSG_RESULT([$sst_check_libz_happy])

  AS_IF([test "$sst_check_libz_happy" = "no" -a ! -z "$with_libz" -a "$with_libz" != "no"], [$3])
  AS_IF([test "$sst_check_libz_happy" = "yes"], [$1], [$2])
])
