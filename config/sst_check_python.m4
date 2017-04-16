
AC_DEFUN([SST_CHECK_PYTHON], [
  AC_ARG_WITH([python],
    [AS_HELP_STRING([--with-python@<:@=DIR@:>@],
      [Use Python package installed in optionally specified DIR])])

  sst_check_python_happy="yes"

  FOUND_PYTHON="no"
  PYTHON_CONFIG_EXE=""

  AC_PATH_PROG([PYTHON_CONFIG_EXE], ["python-config"], 
		["NOTFOUND"])

  AS_IF([test -n "$with_python"],
        [PYTHON_CPPFLAGS="-I$with_python/include"],
        [AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
                [PYTHON_CPPFLAGS=`$PYTHON_CONFIG_EXE --includes`])])

  AS_IF([test -n "$with_python"],
        [PYTHON_LDFLAGS="-L$with_python/lib"],
        [AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
                [PYTHON_LDFLAGS=`$PYTHON_CONFIG_EXE --ldflags`])])

  AS_IF([test -n "$with_python"],
        [PYTHON_LIBS=""],
        [AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
                [PYTHON_LIBS=`$PYTHON_CONFIG_EXE --libs`])])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  CPPFLAGS="$PYTHON_CPPFLAGS $CPPFLAGS"
  LDFLAGS="$PYTHON_LDFLAGS $LDFLAGS"
  LIBS="$PYTHON_LIBS $LIBS"

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([Python.h], [sst_check_python_happy="yes"], [sst_check_python_happy="no"])

  AC_MSG_CHECKING([python libraries])
  AS_IF([test -n "$with_python" -a -z "$PYTHON_LIBS" ],
	[AC_CHECK_LIB([python2.6], [Py_Initialize], [PYTHON_LIBS="-lpython2.6"], [PYTHON_LIBS=""])])
  AS_IF([test -n "$with_python" -a -z "$PYTHON_LIBS" ],
	[AC_CHECK_LIB([python2.7], [Py_Initialize], [PYTHON_LIBS="-lpython2.7"], [PYTHON_LIBS=""])])
  
  AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
	[PYTHON_PREFIX=`$PYTHON_CONFIG_EXE --prefix`], [PYTHON_PREFIX=""])

  AS_IF([test -n "$PYTHON_PREFIX" -a -z "$PYTHON_LIBS"],
	[LDFLAGS="$LDFLAGS -L$PYTHON_PREFIX/lib64"
	 PYTHON_LDFLAGS="$PYTHON_LDFLAGS -L$PYTHON_PREFIX/lib64"])

  AS_IF([test -n "$with_python" -a -z "$PYTHON_LIBS" ],
	[AC_CHECK_LIB([python2.6], [Py_Initialize], [PYTHON_LIBS="-lpython2.6"], [PYTHON_LIBS=""])])
  AS_IF([test -n "$with_python" -a -z "$PYTHON_LIBS" ],
	[AC_CHECK_LIB([python2.7], [Py_Initialize], [PYTHON_LIBS="-lpython2.7"], [PYTHON_LIBS=""])])

  AC_MSG_RESULT([$PYTHON_LIBS])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([PYTHON_LIBS])
  AC_SUBST([PYTHON_CPPFLAGS])
  AC_SUBST([PYTHON_LDFLAGS])
  AC_SUBST([PYTHON_CONFIG_EXE])

  AM_CONDITIONAL([SST_CONFIG_HAVE_PYTHON], [test "$sst_check_python_happy" = "yes"])

  AS_IF([test "$sst_check_python_happy" = "yes"],
        [AC_DEFINE([SST_CONFIG_HAVE_PYTHON], [1], [Set to 1 if Python was found])])
  AS_IF([test "$sst_check_python_happy" != "yes"],
	[AC_MSG_ERROR([Python was not successfully located but this is a prerequisite to build SST])])
  AS_IF([test "$sst_check_python_happy" = "yes"], [$1], [$2])
])
