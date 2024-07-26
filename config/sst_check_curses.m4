dnl -*- mode: autoconf; -*-

AC_DEFUN([SST_CHECK_CURSES],
[
  sst_check_curses_happy="yes"

  AC_ARG_WITH([ncurses],
    [AS_HELP_STRING([--with-ncurses@<:@=DIR or EXEC@:>@],
      [Use ncurses library found in DIR or associated with the ncursesN-config utility specified by EXEC])])

  AS_IF([test "$with_ncurses" = "no"], [sst_check_curses_happy="no"])

  NCURSES_CONFIG_EXE="no"

  dnl check if user provided a specific ncursesN-config
  AS_IF([test ! -d "$with_ncurses"],
    [AS_IF([test -x "$with_ncurses"],
        [NCURSES_CONFIG_EXE=$with_ncurses])])

  dnl test ncursesN-config
  AS_IF([test $NCURSES_CONFIG_EXE = "no"],
    [AS_IF([test -n "$with_ncurses"],
        [AC_PATH_PROGS([NCURSES_CONFIG_EXE],
                       ["ncurses6-config" "ncursesw6-config" "ncurses5.4-config" "ncurses5-config"],
                       ["no"], ["$with_ncurses/bin"])],
        [AC_PATH_PROGS([NCURSES_CONFIG_EXE],
                       ["ncurses6-config" "ncursesw6-config" "ncurses5.4-config" "ncurses5-config"],
                       ["no"])])])

  AC_MSG_CHECKING([ncurses config binary exists])
  AC_MSG_RESULT([$NCURSES_CONFIG_EXE])

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

            dnl Check that the specific header exists and that plausible lib
            dnl locations are correct.
            AC_LANG_PUSH([C++])
            AC_CHECK_HEADER([ncurses.h], [], [sst_check_curses_happy="no"])
            dnl We cannot check that the config-provided lib names are
            dnl correct, since those are not available at macro expansion
            dnl time.  This is only necessary for platforms where libs are
            dnl reported but are broken or don't actually exist.
            dnl
            dnl If nothing is specified for `action-if-found`, the library
            dnl will be added to LIBS, which is not correct for our use case.
            curses_check_lib_tmp=""
            AS_IF([test "$sst_check_curses_happy" != "no"],
              [AC_CHECK_LIB([ncursesw], [initscr], [curses_check_lib_tmp="ncursesw is valid"],
                [AC_CHECK_LIB([ncurses], [initscr], [curses_check_lib_tmp="ncurses is valid"],
                  [AC_CHECK_LIB([curses], [initscr], [curses_check_lib_tmp="curses is valid"],
                    [sst_check_curses_happy="no"
                     CURSES_CPPFLAGS=""
                     CURSES_LIBS=""
                    ])])])
               ])
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

  AS_IF([test "$sst_check_curses_happy" = "no" -a ! -z "$with_ncurses" -a "$with_ncurses" != "no"], [$3])
  AS_IF([test "$sst_check_curses_happy" = "yes"], [$1], [$2])
])
