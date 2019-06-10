AC_DEFUN([SST_CHECK_PARMETIS],
[
  sst_check_parmetis_happy="yes"
  sst_check_parmetis_requested="yes"

  AC_ARG_WITH([parmetis],
    [AS_HELP_STRING([--with-parmetis@<:@=DIR@:>@],
      [Use ParMETIS package installed in optionally specified DIR])],
    [sst_check_parmetis_requested="yes"],
    [sst_check_parmetis_requested="no"])

  AS_IF([test "$with_parmetis" = "no"], [sst_check_parmetis_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  CXX_saved="$CXX"
  CC_saved="$CC"

  AS_IF([test "x$MPICXX" != "x"], [CXX="$MPICXX"])
  AS_IF([test "x$MPICC" != "x"], [CC="$MPICC"])

  AS_IF([test "$sst_check_parmetis_happy" = "yes"], [
    AS_IF([test ! -z "$with_parmetis" -a "$with_parmetis" != "yes"],
      [PARMETIS_CPPFLAGS="$METIS_CPPFLAGS -I$with_parmetis/include"
       CPPFLAGS="$PARMETIS_CPPFLAGS $CPPFLAGS"
       PARMETIS_LDFLAGS="-L$with_parmetis/lib $METIS_LDFLAGS"
       LDFLAGS="$PARMETIS_LDFLAGS $METIS_LDFLAGS $LDFLAGS"],
      [PARMETIS_CPPFLAGS=
       PARMETIS_LDFLAGS=])

    AC_CHECK_HEADERS([parmetis.h], [], [sst_check_parmetis_happy="no"])
    AC_CHECK_LIB([parmetis], [ParMETIS_V3_PartMeshKway], 
      [PARMETIS_LIB="-lparmetis"], [sst_check_parmetis_happy="no"], [$METIS_LIB -lm])])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  CXX="$CXX_saved"
  CC="$CC_saved"

  AC_SUBST([PARMETIS_CPPFLAGS])
  AC_SUBST([PARMETIS_LDFLAGS])
  AC_SUBST([PARMETIS_LIB])

  AC_MSG_CHECKING([for ParMETIS partitioning library])
  AC_MSG_RESULT([$sst_check_parmetis_happy])

  # if user doesn't specify --with-paremtis then string is empty, otherwise is has a value
  # if the value is not "no" then we treat as a path, and MUST find in order to be
  # successful
  AS_IF([test "x$sst_check_parmetis_requested" = "xyes" -a "x$sst_check_parmetis_happy" != "xyes"],
	[AC_MSG_FAILURE([ParMETIS was requested by configure (--with-parmetis=$with_parmetis) but the required libraries and header files could not be found.])])
  AS_IF([test "$sst_check_parmetis_happy" = "yes"], [$1], [$2])
])
