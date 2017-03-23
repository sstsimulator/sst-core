
AC_DEFUN([SST_CHECK_PICKY], [
  use_picky="yes"

  AC_ARG_ENABLE([picky-warnings], [AS_HELP_STRING([--disable-picky-warnings],
      [Disable the use of compiler flags -Wall -Wextra within the SST core.])])

  AS_IF([test "x$enable_picky_warnings" = "xno"], [use_picky="no"])
 
])
