AC_DEFUN([SST_CHECK_HDF5],
[
  sst_check_hdf5_happy="yes"

  AC_ARG_WITH([hdf5],
    [AS_HELP_STRING([--with-hdf5@<:@=DIR@:>@],
      [Use hdf5 (statistics format) library found in DIR])])

  AS_IF([test "$with_hdf5" = "no"], [sst_check_hdf5_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test "$sst_check_hdf5_happy" = "yes"], [
    AS_IF([test ! -z "$with_hdf5" -a "$with_hdf5" != "yes"],
      [HDF5_CPPFLAGS="-I$with_hdf5/include"
       CPPFLAGS="$HDF5_CPPFLAGS $CPPFLAGS"
       HDF5_LDFLAGS="-L$with_hdf5/lib"
       HDF5_LIBS="-lhdf5",
       LDFLAGS="$HDF5_LDFLAGS $LDFLAGS"],
      [HDF5_CPPFLAGS=
       HDF5_LDFLAGS=
       HDF5_LIBS=])])

  CPPFLAGS="${CPPFLAGS} ${HDF_CPPFLAGS}"
  LDFLAGS="${LDFLAGS} ${HDF_LDFLAGS}"
  
  AC_LANG_PUSH([C++])
  AC_CHECK_HEADER([H5Cpp.h], [], [sst_check_hdf5_happy="no"])
  AC_LANG_POP([C++])

  AC_CHECK_LIB([hdf5], [H5F_open],
    [HDF5_LIBS="-lhdf5 -lhdf5_cpp"], [sst_check_hdf5_happy="no"])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([HDF5_CPPFLAGS])
  AC_SUBST([HDF5_LDFLAGS])
  AC_SUBST([HDF5_LIBS])
  AS_IF([test "x$sst_check_hdf5_happy" = "xyes"], [AC_DEFINE([HAVE_HDF5],[1],[Defines whether we have the hdf5 library])])
  AM_CONDITIONAL([USE_HDF5], [test "x$sst_check_hdf5_happy" = "xyes"])

  AC_MSG_CHECKING([for hdf5 library])
  AC_MSG_RESULT([$sst_check_hdf5_happy])
  AS_IF([test "$sst_check_hdf5_happy" = "no" -a ! -z "$with_hdf5" -a "$with_hdf5" != "no"], [$3])
  AS_IF([test "$sst_check_hdf5_happy" = "yes"], [$1], [$2])
])
