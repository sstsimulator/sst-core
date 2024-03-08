AC_DEFUN([SST_CHECK_CURSES],
[
  sst_check_curses_happy="yes"

  AC_ARG_WITH([curses],
    [AS_HELP_STRING([--with-ncurses@<:@=DIR or EXEC@:>@],
      [Use ncurses library found in DIR or associated with the ncursesN-config utility specified by EXEC])])

  AS_IF([test "$with_curses" = "no"], [sst_check_curses_happy="no"])

  NCURSES_CONFIG_EXE="no"

  dnl check if user provided a specific ncursesN-config
  AS_IF([test ! -d "$with_curses"],
    [AS_IF([test -x "$with_curses"],
        [NCURSES_CONFIG_EXE=$with_curses])])

  dnl test ncursesN-config
  AS_IF([test $NCURSES_CONFIG_EXE = "no"],
    [AS_IF([test -n "$with_curses"],
        [AC_PATH_PROGS([NCURSES_CONFIG_EXE], ["ncurses6-config" "ncurses5.4-config" "ncurses5-config"], ["no"], ["$with_curses/bin"])],
        [AC_PATH_PROGS([NCURSES_CONFIG_EXE], ["ncurses6-config" "ncurses5.4-config" "ncurses5-config"], ["no"])])])

  dnl don't continue if ncursesN-config can't be found rather than look for the
  dnl specific libraries
  AS_IF([test $NCURSES_CONFIG_EXE = "no"],
        [
            CURSES_CPPFLAGS=
            CURSES_LIBS=
            sst_check_curses_happy="no"
        ],
        [
            dnl Older versions only have --libs, not --libs-only-l and --libs-only-L,
            dnl which combines the two.  Ideally, CURSES_LDFLAGS (sstinfo_x_LDFLAGS)
            dnl contains --libs-only-L and CURSES_LIBS (sstinfo_x_LDADD) contains
            dnl --libs-only-l, but rather than complicated logic testing the above,
            dnl combining everything into LDADD seems acceptable..
            CURSES_CPPFLAGS=`$NCURSES_CONFIG_EXE --cflags`
            CURSES_LIBS=`$NCURSES_CONFIG_EXE --libs`

            CPPFLAGS_saved="$CPPFLAGS"
            LDFLAGS_saved="$LDFLAGS"

            CPPFLAGS="$CPPFLAGS $CURSES_CPPFLAGS"
            LDFLAGS="$LDFLAGS $CURSES_LIBS"

            dnl Check for specific header
            AC_LANG_PUSH([C++])
            AC_CHECK_HEADER([ncurses.h], [], [sst_check_curses_happy="no"])
            AC_LANG_POP([C++])

            CPPFLAGS="$CPPFLAGS_saved"
            LDFLAGS="$LDFLAGS_saved"
        ]
   )

  AC_SUBST([CURSES_CPPFLAGS])
  AC_SUBST([CURSES_LIBS])
  AS_IF([test "x$sst_check_curses_happy" = "xyes"], [AC_DEFINE([HAVE_CURSES], [1], [Defines whether we have the curses library])])
  AM_CONDITIONAL([USE_CURSES], [test "x$sst_check_curses_happy" = "xyes"])

  AC_MSG_CHECKING([for curses library])
  AC_MSG_RESULT([$sst_check_curses_happy])

  AS_IF([test "$sst_check_curses_happy" = "no" -a ! -z "$with_curses" -a "$with_curses" != "no"], [$3])
  AS_IF([test "$sst_check_curses_happy" = "yes"], [$1], [$2])
])
