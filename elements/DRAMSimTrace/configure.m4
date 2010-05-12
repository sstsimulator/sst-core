dnl -*- Autoconf -*-

AC_DEFUN([SST_DRAMSimTrace_CONFIG], [
  AC_ARG_WITH([dramsim],
    [AS_HELP_STRING([--with-dramsim@<:@=DIR@:>@],
      [Use DRAMSim package installed in optionally specified DIR])])

  AS_IF([test "$with_dramsim" = "no"], [happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test ! -z "$with_dramsim" -a "$with_dramsim" != "yes"],
    [DRAMSIM_CPPFLAGS="-I$with_dramsim"
     CPPFLAGS="$DRAMSIM_CPPFLAGS $CPPFLAGS"
     DRAMSIM_LDFLAGS="-L$with_dramsim"
     LDFLAGS="$DRAMSIM_LDFLAGS $LDFLAGS"],
    [DRAMSIM_CPPFLAGS=
     DRAMSIM_LDFLAGS=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([MemorySystem.h], [], [happy="no"])
  AC_CHECK_LIB([dramsim], [libdramsim_is_present], 
    [DRAMSIM_LIB="-ldramsim"], [happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([DRAMSIM_CPPFLAGS])
  AC_SUBST([DRAMSIM_LDFLAGS])
  AC_SUBST([DRAMSIM_LIB])

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
