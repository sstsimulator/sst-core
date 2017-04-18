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
          [
            HDF5_CFLAGS="-I$with_hdf5/include"
            HDF5_LDFLAGS="-L$with_hdf5/lib"
          ],
          [
            AC_PATH_PROG([PKGCONFIG], [pkg-config])
            AS_IF([test ! -z "${PKGCONFIG}" ],
                [
                    echo "PKGCONFIG = $PKGCONFIG"
                    HDF5_LDFLAGS=`${PKGCONFIG} hdf5 && ${PKGCONFIG} --libs hdf5`
                    HDF5_CFLAGS=`${PKGCONFIG} hdf5 && ${PKGCONFIG} --cflags hdf5`
                ],
                [
                    HDF5_CFLAGS=
                    HDF5_LDFLAGS=
                ]
            )
          ]
        )
    ]
  )

  CPPFLAGS="${CPPFLAGS} ${HDF5_CFLAGS}"
  LDFLAGS="${LDFLAGS} ${HDF5_LDFLAGS} -lhdf5 -lhdf5_cpp"
  
  AC_LANG_PUSH([C++])
  AC_CHECK_HEADER([H5Cpp.h], [], [sst_check_hdf5_happy="no"])
  AC_CHECK_LIB([hdf5], [H5F_open],
    [HDF5_LIBS="-lhdf5 -lhdf5_cpp"], [sst_check_hdf5_happy="no"])

  AC_MSG_CHECKING("For C++ ABI compatibility with HDF5Cpp library")
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
        [#include <H5Cpp.h>],
        [
            std::string name = "";
            H5::H5File file(name, H5F_ACC_TRUNC);
        ]
    )],
    [],
    [sst_check_hdf5_happy="no"]
  )
  AC_MSG_RESULT($sst_check_hdf5_happy)

  AC_LANG_POP([C++])


  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([HDF5_CFLAGS])
  AC_SUBST([HDF5_LDFLAGS])
  AC_SUBST([HDF5_LIBS])
  AS_IF([test "x$sst_check_hdf5_happy" = "xyes"], [AC_DEFINE([HAVE_HDF5],[1],[Defines whether we have the hdf5 library])])
  AM_CONDITIONAL([USE_HDF5], [test "x$sst_check_hdf5_happy" = "xyes"])

  AC_MSG_CHECKING([for hdf5 library])
  AC_MSG_RESULT([$sst_check_hdf5_happy])
  AS_IF([test "$sst_check_hdf5_happy" = "no" -a ! -z "$with_hdf5" -a "$with_hdf5" != "no"], [$3])
  AS_IF([test "$sst_check_hdf5_happy" = "yes"], [$1], [$2])
])
