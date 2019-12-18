
AC_DEFUN([SST_CHECK_PYTHON], [
  AC_ARG_WITH([python],
    [AS_HELP_STRING([--with-python@<:@=DIR@:>@],
      [Use Python package installed in optionally specified DIR])])

  sst_check_python_happy="yes"

  FOUND_PYTHON="no"
  PYTHON_CONFIG_EXE=""
  PYTHON2_DEF_CONFIG_EXE=""
  PYTHON3_DEF_CONFIG_EXE=""
  PYTHON_DEF_CONFIG_EXE=""
  PYTHON_VERSION3="no"
  PYTHON_DEF_VERSION3="no"
  
  AC_PATH_PROGS([PYTHON2_DEF_CONFIG_EXE], ["python2.6-config" "python2.7-config"], ["NOTFOUND"])

  AS_IF([test $PYTHON2_DEF_CONFIG_EXE = "NOTFOUND"], 
        [AC_PATH_PROGS([PYTHON3_DEF_CONFIG_EXE], ["python3.5-config" "python3.6-config" "python3.7-config"], ["NOTFOUND"])])

  AS_IF([test $PYTHON2_DEF_CONFIG_EXE = "NOTFOUND"],
        [AS_IF([test $PYTHON3_DEF_CONFIG_EXE != "NOTFOUND"], 
                [PYTHON_DEF_VERSION3="yes"],
                [AC_PATH_PROG([PYTHON2_DEF_CONFIG_EXE], ["python-config"], ["NOTFOUND"])])])

  AS_IF([test "$PYTHON2_DEF_CONFIG_EXE" != "NOTFOUND"], 
        [PYTHON_DEF_CONFIG_EXE=$PYTHON2_DEF_CONFIG_EXE], 
        [PYTHON_DEF_CONFIG_EXE=$PYTHON3_DEF_CONFIG_EXE])

  AS_IF([test -n "$with_python"],
       [AC_PATH_PROGS([PYTHON_CONFIG_EXE], ["python3.5-config" "python3.6-config" "python3.7-config"], [""], ["$with_python/bin"])])

  AS_IF([test -n "$PYTHON_CONFIG_EXE"], [PYTHON_VERSION3="yes"])
  
  AS_IF([test -n "$with_python" -a -z "$PYTHON_CONFIG_EXE"], 
        [AC_PATH_PROGS([PYTHON_CONFIG_EXE], ["python2.6-config" "python2.7-config" "python-config"], [""], ["$with_python/bin$PATH_SEPARATOR$with_python"])])

  AS_IF([test -z "$PYTHON_CONFIG_EXE"], [PYTHON_CONFIG_EXE=$PYTHON_DEF_CONFIG_EXE; PYTHON_VERSION3=$PYTHON_DEF_VERSION3])

  AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
        [PYTHON_CPPFLAGS=`$PYTHON_CONFIG_EXE --includes`])
  
  AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
        [PYTHON_LDFLAGS=`$PYTHON_CONFIG_EXE --ldflags`])
        
  AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
        [PYTHON_LIBS=`$PYTHON_CONFIG_EXE --libs`])
  AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
        [PYTHON_PREFIX=`$PYTHON_CONFIG_EXE --prefix`])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  CPPFLAGS="$PYTHON_CPPFLAGS $CPPFLAGS"
  LDFLAGS="$PYTHON_LDFLAGS $LDFLAGS"
  LIBS="$PYTHON_LIBS $LIBS"

  AC_LANG_PUSH(C++)
  
  AC_CHECK_HEADERS([Python.h], [sst_check_python_happy="yes"], [sst_check_python_happy="no"])
  
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([PYTHON_LIBS])
  AC_SUBST([PYTHON_CPPFLAGS])
  AC_SUBST([PYTHON_LDFLAGS])
  AC_SUBST([PYTHON_CONFIG_EXE])
  AC_SUBST([PYTHON_VERSION3])

  AM_CONDITIONAL([SST_CONFIG_HAVE_PYTHON], [test "$sst_check_python_happy" = "yes"])
  AM_CONDITIONAL([SST_CONFIG_HAVE_PYTHON3], [test "$PYTHON_VERSION3" = "yes"])

  AS_IF([test "$PYTHON_VERSION3" = "yes"], 
        [AC_DEFINE([SST_CONFIG_HAVE_PYTHON3], [1], [Set to 1 if Python version is 3])])
  AS_IF([test "$sst_check_python_happy" = "yes"],
        [AC_DEFINE([SST_CONFIG_HAVE_PYTHON], [1], [Set to 1 if Python was found])])
  AS_IF([test "$sst_check_python_happy" != "yes"],
	[AC_MSG_ERROR([Python was not successfully located but this is a prerequisite to build SST])])
  AS_IF([test "$sst_check_python_happy" = "yes"], [$1], [$2])
])
