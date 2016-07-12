AC_DEFUN([SST_CHECK_ZOLTAN],
[
  sst_check_zoltan_happy="yes"
  sst_check_zoltan_requested="yes"

  AC_ARG_WITH([zoltan],
    [AS_HELP_STRING([--with-zoltan@<:@=DIR@:>@],
      [Use Zoltan package installed in optionally specified DIR])],
    [sst_check_zoltan_requested="yes"],
    [sst_check_zoltan_requested="no"])

  AS_IF([test "$with_zoltan" = "no"], [sst_check_zoltan_happy="no"])

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

  # if user doesn't specify --with-zoltan then string is empty, otherwise is has a value
  # if the value is not "no" then we treat as a path, and MUST find in order to be
  # successful
  AS_IF([test "x$sst_check_zoltan_requested" = "xyes" -a "x$sst_check_zoltan_happy" != "xyes"],
	[AC_MSG_FAILURE([Zoltan was requested by configure (--with-zoltan=$with_zoltan) but the required libraries and header files could not be found.])])
  AS_IF([test "$sst_check_zoltan_happy" = "yes"], [$1], [$2])
])
