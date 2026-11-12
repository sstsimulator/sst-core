
AC_DEFUN([SST_ENABLE_DEBUG_OUTPUT], [

  AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
      [Enables additional debug output to be compiled by components and the SST core,
       and compiles with full debug symbols (-g). Equivalent to
       --enable-debug-output --enable-debug-symbols.])])

  AC_ARG_ENABLE([debug-output],
    [AS_HELP_STRING([--enable-debug-output],
      [Enables additional debug output to be compiled by components and the SST core.
       Defaults to the value of --enable-debug.])],
    [], [enable_debug_output="$enable_debug"])

  AS_IF([test "x$enable_debug_output" = "xyes" ],
	[AC_DEFINE([__SST_DEBUG_OUTPUT__], [1], [Defines additional debug output is to be printed])])

])
