
AC_DEFUN([SST_ENABLE_DEBUG_OUTPUT], [

  AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
      [Enables additional debug output to be compiled by components and the SST core.])])

  AS_IF([test "x$enable_debug" = "xyes" ],
	[AC_DEFINE([__SST_DEBUG_OUTPUT__], [1], [Defines additional debug output is to be printed])])

])
