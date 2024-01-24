AC_DEFUN([SST_CHECK_CURSES],
[
  sst_check_curses_happy="yes"

  AC_ARG_WITH([curses],
    [AS_HELP_STRING([--with-ncurses@<:@=DIR@:>@],
      [Use ncurses library found in DIR])])

  AS_IF([test "$with_curses" = "no"], [sst_check_curses_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  dnl Use user-defined curses library
  AS_IF([test "$sst_check_curses_happy" = "yes"], [
    AS_IF([test ! -z "$with_curses" -a "$with_curses" != "yes"],
      [CURSES_CPPFLAGS="-I$with_curses/include"
       CPPFLAGS="$CURSES_CPPFLAGS $CPPFLAGS"
       CURSES_LDFLAGS="-L$with_curses/lib"
       CURSES_LIBS="-lncurses",
       LDFLAGS="$CURSES_LDFLAGS $LDFLAGS"],
      [CURSES_CPPFLAGS=
       CURSES_CPPFLAGS_LDFLAGS=
       CURSES_LIBS=])])
  
dnl Check for curses.h header
  AC_LANG_PUSH([C++])
  AC_CHECK_HEADER([curses.h], [], [sst_check_curses_happy="no"])
  AC_LANG_POP([C++])

dnl Check that library is usable
AS_IF([test "$sst_check_curses_happy" != "no"], 
  [AC_CHECK_LIB([ncursesw], [wprintw], [CURSES_LIBS="-lncursesw"],
    [AC_CHECK_LIB([ncurses], [wprintw], [CURSES_LIBS="-lncurses"],
      [AC_CHECK_LIB([curses], [wprintw], [CURSES_LIBS="-lcurses"], [sst_check_curses_happy = "no"])])]) 
  ])

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
