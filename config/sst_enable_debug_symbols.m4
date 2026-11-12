
dnl SST_ENABLE_DEBUG_SYMBOLS
dnl
dnl By default, replace the -g that AC_PROG_CC / AC_PROG_CXX add to CFLAGS and
dnl CXXFLAGS with -g1, which keeps function names and line tables (enough for
dnl readable backtraces) but drops variable and type information.  This
dnl substantially reduces compile memory, compile time, and binary size.
dnl --enable-debug-symbols (or --enable-debug) keeps the full -g.  User
dnl supplied CFLAGS or CXXFLAGS are never modified (each is handled separately).
dnl
dnl The result is exported to sst-config as DEBUG_SYMBOLS=yes|no so that
dnl sst-elements can use the same debug symbol level by default.
dnl
dnl Must be called after AC_PROG_CC and AC_PROG_CXX.  sst_user_cflags_set and
dnl sst_user_cxxflags_set must be recorded before AC_PROG_CC is called.
AC_DEFUN([SST_ENABLE_DEBUG_SYMBOLS], [

  AC_ARG_ENABLE([debug-symbols], [AS_HELP_STRING([--enable-debug-symbols],
      [Compile with full debug symbols (-g) instead of the default line-table-only
       debug information (-g1). Defaults to the value of --enable-debug. Does not
       modify CFLAGS or CXXFLAGS that are set explicitly.])],
      [], [enable_debug_symbols="$enable_debug"])

  AS_IF([test "x$enable_debug_symbols" != "xyes"], [enable_debug_symbols="no"])
  SST_DEBUG_SYMBOLS_STR="$enable_debug_symbols"
  AC_SUBST([SST_DEBUG_SYMBOLS_STR])

  AC_MSG_CHECKING([for debug symbol level])
  AS_IF([test "x$enable_debug_symbols" = "xyes"],
        [AC_MSG_RESULT([full (-g)])],
        [
    sst_save_CFLAGS="$CFLAGS"
    sst_save_CXXFLAGS="$CXXFLAGS"
    CFLAGS="-g1"
    CXXFLAGS="-g1"
    AC_LANG_PUSH([C])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [])], [sst_cc_g1="yes"], [sst_cc_g1="no"])
    AC_LANG_POP([C])
    AC_LANG_PUSH([C++])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [])], [sst_cxx_g1="yes"], [sst_cxx_g1="no"])
    AC_LANG_POP([C++])
    CFLAGS="$sst_save_CFLAGS"
    CXXFLAGS="$sst_save_CXXFLAGS"

    AS_IF([test "x$sst_cc_g1" = "xyes" && test "x$sst_cxx_g1" = "xyes"],
          [dnl Only rewrite the standalone -g token added by autoconf
           AS_IF([test "x$sst_user_cflags_set" != "xset"],
                 [CFLAGS=`echo " $CFLAGS " | sed -e 's/ -g / -g1 /' -e 's/^ *//' -e 's/ *$//'`])
           AS_IF([test "x$sst_user_cxxflags_set" != "xset"],
                 [CXXFLAGS=`echo " $CXXFLAGS " | sed -e 's/ -g / -g1 /' -e 's/^ *//' -e 's/ *$//'`])
           AC_MSG_RESULT([line tables only (-g1)])],
          [AC_MSG_RESULT([full (-g), compiler does not support -g1])])
  ])
  AS_IF([test "x$sst_user_cflags_set" = "xset"],
        [AC_MSG_NOTICE([CFLAGS set explicitly, its debug symbol flags are not modified])])
  AS_IF([test "x$sst_user_cxxflags_set" = "xset"],
        [AC_MSG_NOTICE([CXXFLAGS set explicitly, its debug symbol flags are not modified])])
])
