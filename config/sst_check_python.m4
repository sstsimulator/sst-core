AC_DEFUN([SST_CHECK_PYTHON], [
  AC_ARG_WITH([python],
    [AS_HELP_STRING([--with-python@<:@=DIR or EXEC@:>@],
      [Use Python package installed in optionally specified DIR or associated with the python-config utility specified by EXEC])])

  sst_check_python_happy="yes"

  PYTHON_CONFIG_EXE="no"
  PYTHON_VERSION3="no"

dnl check if user provided a specific python-config
  AS_IF([test ! -d "$with_python"],
    [AS_IF([test -x "$with_python"],
        [PYTHON_CONFIG_EXE=$with_python])])

dnl search python3-config
  AS_IF([test $PYTHON_CONFIG_EXE = "no"],
    [AS_IF([test -n "$with_python"],
        [AC_PATH_PROGS([PYTHON_CONFIG_EXE], ["python3-config" "python3.11-config" "python3.10-config" "python3.9-config" "python3.8-config" "python3.7-config" "python3.6-config"], ["no"], ["$with_python/bin"])],
        [AC_PATH_PROGS([PYTHON_CONFIG_EXE], ["python3-config" "python3.11-config" "python3.10-config" "python3.9-config" "python3.8-config" "python3.7-config" "python3.6-config"], ["no"])])])

dnl search python-config
  AS_IF([test $PYTHON_CONFIG_EXE = "no"],
    [AS_IF([test -n "$with_python"], 
        [AC_PATH_PROGS([PYTHON_CONFIG_EXE], ["python-config"], ["no"], ["$with_python/bin"])],
        [AC_PATH_PROGS([PYTHON_CONFIG_EXE], ["python-config"], ["no"])])])

dnl error if pythonX-config can't be found

  AC_MSG_CHECKING([python-config exists])
  AC_MSG_RESULT([$PYTHON_CONFIG_EXE])
  AS_IF([test $PYTHON_CONFIG_EXE = "no"],
        [AC_MSG_ERROR([Unable to locate Python configure utility, check that devel package is installed])])

  PYTHON_CPPFLAGS=`$PYTHON_CONFIG_EXE --includes`
  PYTHON_LDFLAGS=`$PYTHON_CONFIG_EXE --ldflags`
  PYTHON_LIBS=`$PYTHON_CONFIG_EXE --libs`
  PYTHON_PREFIX=`$PYTHON_CONFIG_EXE --prefix`
  
dnl Determine python version using the interpreter
dnl Assume a consistent naming convention for pythonX and pythonX-config
  PYTHON_CONFIG_NAME=${PYTHON_CONFIG_EXE##*/}
  PYTHON_NAME=${PYTHON_CONFIG_NAME%%-*}
  AC_PATH_PROGS([PYTHON_EXE], ["$PYTHON_NAME"], [""], ["$PYTHON_PREFIX/bin"])


dnl Error if python version < 3.6
  AM_PYTHON_CHECK_VERSION([$PYTHON_EXE], [3.6], [PYTHON_VERSION3="yes"], [AC_MSG_ERROR([Python version must be >= 3.6])])

dnl Python3.8+ doesn't link to lpython by default
  AM_PYTHON_CHECK_VERSION([$PYTHON_EXE], [3.8], [PYTHON_LIBS=`$PYTHON_CONFIG_EXE --libs --embed`], [])
  
  AM_PYTHON_CHECK_VERSION([$PYTHON_EXE], [3.8], [PYTHON_LDFLAGS=`$PYTHON_CONFIG_EXE --ldflags --embed`], [])

dnl Figure out the name of the python library
  PYLIBX=`sed 's/.*python/python/g' <<< "$PYTHON_LIBS"`
  PYLIB=${PYLIBX%% *}

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

dnl Sometimes python-config doesn't give the library path correctly
dnl Also, autoconf caches the result of AC_CHECK_LIB and won't recheck
dnl even though LDFLAGS is updated
  PYLIB_US=${PYLIB/./_}

m4_if(m4_defn([AC_AUTOCONF_VERSION]), 2.71, [PYCACHEVAR="ac_cv_lib_${PYLIB_US}_Py_Initialize"], 
	m4_defn([AC_AUTOCONF_VERSION]), 2.70, [PYCACHEVAR="ac_cv_lib_${PYLIB_US}_Py_Initialize"], 
		[PYCACHEVAR="ac_cv_lib_${PYLIB_US}___Py_Initialize"])

  AC_CHECK_LIB([$PYLIB], [Py_Initialize], [PYLIB_OK="yes"], [PYLIB_OK="no"])
  AS_UNSET([$PYCACHEVAR]) 

  AS_IF([test "$PYLIB_OK" = "no"],
        [LDFLAGS="$LDFLAGS -L$PYTHON_PREFIX/lib"])
  
  AS_IF([test "$PYLIB_OK" = "no"],
        [AC_CHECK_LIB([$PYLIB], 
                      [Py_Initialize], 
                      [PYLIB_OK="yes"
                       PYTHON_LDFLAGS="$PYTHON_LDFLAGS -L$PYTHON_PREFIX/lib"], 
                      [PYLIB_OK="no"])])
  
  AS_UNSET([$PYCACHEVAR]) 
  
  AS_IF([test "$PYLIB_OK" = "no"],
        [LDFLAGS="$LDFLAGS -L$PYTHON_PREFIX/lib64"])
  
  AS_IF([test "$PYLIB_OK" = "no"],
        [AC_CHECK_LIB([$PYLIB], 
                      [Py_Initialize], 
                      [PYLIB_OK="yes"
                       PYTHON_LDFLAGS="$PYTHON_LDFLAGS -L$PYTHON_PREFIX/lib64"],
                      [PYLIB_OK="no"])])
  
  AC_MSG_CHECKING([python libraries])
  AC_MSG_RESULT([$PYLIB_OK])
  AS_IF([test $PYLIB_OK = "no"],
        [AC_MSG_ERROR([Unable to locate Python library $PYLIB in $LDFLAGS])])

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
