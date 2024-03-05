AC_DEFUN([SST_CHECK_CURSES],
[
  sst_check_curses_happy="yes"

  AC_ARG_WITH([curses],
    [AS_HELP_STRING([--with-ncurses@<:@=DIR or EXEC@:>@],
      [Use ncurses library found in DIR or associated with the ncurses6-config utility specified by EXEC])])

  AS_IF([test "$with_curses" = "no"], [sst_check_curses_happy="no"])

  NCURSES_CONFIG_EXE="no"

  dnl check if user provided a specific ncurses6-config
  AS_IF([test ! -d "$with_curses"],
    [AS_IF([test -x "$with_curses"],
        [NCURSES_CONFIG_EXE=$with_curses])])

  dnl test ncurses6-config
  AS_IF([test $NCURSES_CONFIG_EXE = "no"],
    [AS_IF([test -n "$with_curses"],
        [AC_PATH_PROGS([NCURSES_CONFIG_EXE], ["ncurses6-config"], ["no"], ["$with_curses/bin"])],
        [AC_PATH_PROGS([NCURSES_CONFIG_EXE], ["ncurses6-config"], ["no"])])])

  dnl error if ncurses6-config can't be found rather than look for the
  dnl specific libraries
  AC_MSG_CHECKING([ncurses6-config exists])
  AC_MSG_RESULT([$NCURSES_CONFIG_EXE])
  AS_IF([test $NCURSES_CONFIG_EXE = "no"],
        [AC_MSG_ERROR([Unable to locate ncurses6-config utility])])

  CURSES_CPPFLAGS=`$NCURSES_CONFIG_EXE --cflags`
  CURSES_LDFLAGS=`$NCURSES_CONFIG_EXE --libs-only-L`
  CURSES_LIBS=`$NCURSES_CONFIG_EXE --libs-only-l`

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

dnl Check for header
  AC_LANG_PUSH([C++])
  AC_CHECK_HEADER([ncurses.h], [], [sst_check_curses_happy="no"])
  AC_LANG_POP([C++])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([CURSES_CPPFLAGS])
  AC_SUBST([CURSES_LDFLAGS])
  AC_SUBST([CURSES_LIBS])
  AS_IF([test "x$sst_check_curses_happy" = "xyes"], [AC_DEFINE([HAVE_CURSES], [1], [Defines whether we have the curses library])])
  AM_CONDITIONAL([USE_CURSES], [test "x$sst_check_curses_happy" = "xyes"])

  AC_MSG_CHECKING([for curses library])
  AC_MSG_RESULT([$sst_check_curses_happy])

  AS_IF([test "$sst_check_curses_happy" = "no" -a ! -z "$with_curses" -a "$with_curses" != "no"], [$3])
  AS_IF([test "$sst_check_curses_happy" = "yes"], [$1], [$2])
])
