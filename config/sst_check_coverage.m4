AC_DEFUN([SST_CHECK_COVERAGE], [
  AC_ARG_WITH([lcov],
     [AC_HELP_STRING([--with-lcov],
                     [Specify the full path of the lcov executable. Default assumes lcov is in PATH])], [
     LCOV_EXE=$withval
   ], [
     LCOV_EXE=lcov
   ]
  )

  AC_ARG_ENABLE([coverage],
     [AC_HELP_STRING([--(dis|en)able-coverage],
                     [Add flags enabling code coverage. This is for debug builds only and will have poor performance. Default is disabled])], [
     enable_coverage=$enableval
   ], [
     enable_coverage=no
   ]
  )
  COVERAGE_CXXFLAGS=""
  if test "X$enable_coverage" = "Xyes"; then
    COVERAGE_CXXFLAGS="-fprofile-arcs -ftest-coverage -fPIC"
    AC_MSG_CHECKING([code coverage flags])
    AC_MSG_RESULT([$COVERAGE_CXXFLAGS])
    AC_CHECK_PROG(have_lcov, "$LCOV_EXE", yes, no)
    if test "X$have_lcov" = "Xno"; then
      AC_MSG_ERROR([You have enabled code coverage, but $LCOV_EXE is not a valid program. Please give a valid lcov using --with-lcov])
    fi
    AC_SUBST([COVERAGE_CXXFLAGS])
    AC_SUBST([LCOV_EXE])
  fi
])

