AC_DEFUN([SST_CHECK_PYTHON], [
  AC_ARG_WITH([python],
    [AS_HELP_STRING([--with-python@<:@=DIR or EXEC@:>@],
      [Use Python package installed in optionally specified DIR or associated with the python-config utility specified by EXEC])])

  sst_check_python_happy="yes"

  PYTHON_CONFIG_EXE="NOTFOUND"
  PYTHON_VERSION3="no"

dnl check if user provided a specific python-config
  AS_IF([test ! -d "$with_python"],
    [AS_IF([test -x "$with_python"],
        [PYTHON_CONFIG_EXE=$with_python])])

dnl search python2-config
  AS_IF([test $PYTHON_CONFIG_EXE = "NOTFOUND"],
    [AS_IF([test -n "$with_python"],
        [AC_PATH_PROGS([PYTHON_CONFIG_EXE], ["python2-config" "python2.7-config" "python2.6-config"], ["NOTFOUND"], ["$with_python/bin"])],
        [AC_PATH_PROGS([PYTHON_CONFIG_EXE], ["python2-config" "python2.7-config" "python2.6-config"], ["NOTFOUND"])])])

dnl search python-config
  AS_IF([test $PYTHON_CONFIG_EXE = "NOTFOUND"],
    [AS_IF([test -n "$with_python"], 
        [AC_PATH_PROGS([PYTHON_CONFIG_EXE], ["python-config"], ["NOTFOUND"], ["$with_python/bin"])],
        [AC_PATH_PROGS([PYTHON_CONFIG_EXE], ["python-config"], ["NOTFOUND"])])])

dnl search python3-config
  AS_IF([test $PYTHON_CONFIG_EXE = "NOTFOUND"],
    [AS_IF([test -n "$with_python"],
        [AC_PATH_PROGS([PYTHON_CONFIG_EXE], ["python3-config" "python3.8-config" "python3.7-config" "python3.6-config" "python3.5-config"], ["NOTFOUND"], ["$with_python/bin"])],
        [AC_PATH_PROGS([PYTHON_CONFIG_EXE], ["python3-config" "python3.8-config" "python3.7-config" "python3.6-config" "python3.5-config"], ["NOTFOUND"])])])


  AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
        [PYTHON_CPPFLAGS=`$PYTHON_CONFIG_EXE --includes`])
  
  AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
        [PYTHON_LDFLAGS=`$PYTHON_CONFIG_EXE --ldflags`])
        
  AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
        [PYTHON_LIBS=`$PYTHON_CONFIG_EXE --libs`])

  AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
        [PYTHON_PREFIX=`$PYTHON_CONFIG_EXE --prefix`])
  
dnl Determine python version using the interpreter
dnl Assume a consistent naming convention for pythonX and pythonX-config
  PYTHON_CONFIG_NAME=${PYTHON_CONFIG_EXE##*/}
  PYTHON_NAME=${PYTHON_CONFIG_NAME%%-*}
  AC_PATH_PROGS([PYTHON_EXE], ["$PYTHON_NAME"], [""], ["$PYTHON_PREFIX/bin"])
 
dnl Error if python version < 2.6
  AM_PYTHON_CHECK_VERSION([$PYTHON_EXE], [2.6], [PYTHON_VERSION3="no"], [AC_MSG_ERROR([Python version must be >= 2.6])])

dnl Set python3 flag if version >= 3.5
  AM_PYTHON_CHECK_VERSION([$PYTHON_EXE], [3.5], [PYTHON_VERSION3="yes"])

dnl Error if python version is < 3.5 but > 3.0
  AS_IF([test "$PYTHON_VERSION3" = "no"],
    [AM_PYTHON_CHECK_VERSION([$PYTHON_EXE], [3.0],
        [AC_MSG_ERROR([Python3 version must be >= 3.5])])])

dnl Python3.8+ doesn't link to lpython by default
  AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
        [AM_PYTHON_CHECK_VERSION([$PYTHON_EXE], [3.8], [PYTHON_LIBS=`$PYTHON_CONFIG_EXE --libs --embed`], [])])
  
  AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
        [AM_PYTHON_CHECK_VERSION([$PYTHON_EXE], [3.8], [PYTHON_LDFLAGS=`$PYTHON_CONFIG_EXE --ldflags --embed`], [])])

dnl report Python version
  PYTHON_VERSION=`$PYTHON_EXE -V 2>&1`
  AC_MSG_NOTICE([Python version is $PYTHON_VERSION])
  
dnl check for Python.h
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
  AC_SUBST([PYTHON_EXE])
  AC_SUBST([PYTHON_VERSION3])
  AC_SUBST([PYTHON_VERSION])

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
