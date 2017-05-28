AC_DEFUN([SST_CHECK_FPIC],
[
  CXXFLAGS_saved="$CXXFLAGS"
  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  SST_ELEMENT_FPIC_FLAGS="-fPIC"
  CXXFLAGS="$CXXFLAGS $SST_ELEMENT_FPIC_FLAGS"

  AC_LANG_PUSH([C++])
  AC_CHECK_HEADER([stdio.h], [sst_check_fpic_happy="yes"], [sst_check_fpic_happy="no"])
  AC_LANG_POP([C++])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  CXXFLAGS="$CXXFLAGS_saved"

  AS_IF([test "x$sst_check_fpic_happy" = "xyes" ],
	[], [SST_ELEMENT_FPIC_FLAGS=""])

  AC_SUBST([SST_ELEMENT_FPIC_FLAGS])

  AC_MSG_CHECKING([for position-independent code flags])
  AS_IF([test "x$sst_check_fpic_happy" = "xyes" ], 
	[AC_MSG_RESULT(["$SST_ELEMENT_FPIC_FLAGS"])],
	["not found"])

])
