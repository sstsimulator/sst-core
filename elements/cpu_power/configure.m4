AC_DEFUN([SST_cpu_power_CONFIG], [
  AC_ARG_WITH([sim-panalyzer],
    [AS_HELP_STRING([--with-panalyzer@<:@=DIR@:>@],
      [Use Sim-Panalyzer package installed in optionally specified DIR])])

  AS_IF([test "$with_panalyzer" = "no"], [happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test ! -z "$with_panalyzer" -a "$with_panalyzer" != "yes"],
    [SIMPANALYZER_CPPFLAGS="-I$with_panalyzer/include"
     CPPFLAGS="$SIMPANALYZER_CPPFLAGS $CPPFLAGS"
     SIMPANALYZER_LDFLAGS="-L$with_panalyzer"
     LDFLAGS="$SIMPANALYZER_LDFLAGS $LDFLAGS"],
    [SIMPANALYZER_CPPFLAGS=
     SIMPANALYZER_LDFLAGS=])

  AC_LANG_PUSH(C++)  
  AC_CHECK_LIB([sim-panalyzer], [lv1_panalyzer], 
    [SIMPANALYZER_LIB="-lsim-panalyzer"], [happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([SIMPANALYZER_CPPFLAGS])
  AC_SUBST([SIMPANALYZER_LDFLAGS])
  AC_SUBST([SIMPANALYZER_LIB])

  AS_IF([test "$happy" = "yes"], [$1], [$2])




  AC_ARG_WITH([McPAT],
    [AS_HELP_STRING([--with-McPAT@<:@=DIR@:>@],
      [Use McPAT package installed in optionally specified DIR])])

  AS_IF([test "$with_McPAT" = "no"], [happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test ! -z "$with_McPAT" -a "$with_McPAT" != "yes"],
    [MCPAT_CPPFLAGS="-I$with_McPAT/include"
     CPPFLAGS="$MCPAT_CPPFLAGS $CPPFLAGS"
     MCPAT_LDFLAGS="-L$with_McPAT"
     LDFLAGS="$MCPAT_LDFLAGS $LDFLAGS"],
    [MCPAT_CPPFLAGS=
     MCPAT_LDFLAGS=])

  AC_LANG_PUSH(C++)  
  AC_CHECK_LIB([McPAT], [main], 
    [MCPAT_LIB="-lMcPAT"], [happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([MCPAT_CPPFLAGS])
  AC_SUBST([MCPAT_LDFLAGS])
  AC_SUBST([MCPAT_LIB])

  AS_IF([test "$happy" = "yes"], [$1], [$2])


])
