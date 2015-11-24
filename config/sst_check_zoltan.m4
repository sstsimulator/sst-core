# -*- Autoconf -*-
#
# SYNOPSIS
#
#   SST_CHECK_ZOLTAN
#
# DESCRIPTION
#
# LICENSE
#

AC_DEFUN([SST_CHECK_ZOLTAN],
[
  sst_check_zoltan_happy="yes"

  AC_ARG_WITH([zoltan],
    [AS_HELP_STRING([--with-zoltan@<:@=DIR@:>@],
      [Use Zoltan package installed in optionally specified DIR])])

  AS_IF([test "$with_zoltan" = "no"], [sst_check_zoltan_happy="no"])

  SST_CHECK_PARMETIS([ AC_DEFINE([HAVE_PARMETIS], [1], [Define if you have the ParMETIS library.])] )

#  AS_IF([test "$sst_check_zoltan_happy" = "yes"], 
#        [SST_CHECK_PARMETIS([AC_DEFINE([HAVE_PARMETIS], [1], [Define if you have the Parmetis library.])], [sst_check_zoltan_happy="no"])])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  CXX_saved="$CXX"
  CC_saved="$CC"

  AS_IF([test "x$MPICXX" != "x"], [CXX="$MPICXX"])
  AS_IF([test "x$MPICC" != "x"], [CC="$MPICC"])

  AS_IF([test "$sst_check_zoltan_happy" = "yes"], [
    AS_IF([test ! -z "$with_zoltan" -a "$with_zoltan" != "yes"],
      [ZOLTAN_CPPFLAGS="-I$with_zoltan/include"
       CPPFLAGS="$PARMETIS_CPPFLAGS $ZOLTAN_CPPFLAGS $CPPFLAGS"
       ZOLTAN_LDFLAGS="-L$with_zoltan/lib"
       LDFLAGS="$PARMETIS_LDFLAGS $ZOLTAN_LDFLAGS $LDFLAGS -lm"],
      [ZOLTAN_CPPFLAGS=
       ZOLTAN_LDFLAGS=])

    AC_CHECK_HEADERS([zoltan.h], [], [sst_check_zoltan_happy="no"])
    AC_CHECK_LIB([zoltan], [Zoltan_Initialize], 
      [ZOLTAN_LIB="-lzoltan"], [sst_check_zoltan_happy="no"], [$PARMETIS_LIB])])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  CXX="$CXX_saved"
  CC="$CC_saved"

  AC_SUBST([ZOLTAN_CPPFLAGS])
  AC_SUBST([ZOLTAN_LDFLAGS])
  AC_SUBST([ZOLTAN_LIB])

  AC_MSG_CHECKING([for Zoltan package])
  AC_MSG_RESULT([$sst_check_zoltan_happy])
  AS_IF([test "$sst_check_zoltan_happy" = "no" -a ! -z "$with_zoltan" -a "$with_zoltan" != "no"], [$3])
  AS_IF([test "$sst_check_zoltan_happy" = "yes"], [$1], [$2])
])
