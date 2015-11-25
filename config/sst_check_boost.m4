
AC_DEFUN([SST_CHECK_BOOST], [

	AC_ARG_WITH([boost], [AS_HELP_STRING([--with-boost@<:@=DIR@:>@],
                             [use boost (default is yes) - it is possible to
                              specify the root directory for boost (optional)])])

	sst_check_boost_happy="yes"

	AS_IF([test "$with_boost" = "no"], [sst_check_boost_happy="no"])

	CPPFLAGS_saved="$CPPFLAGS"
  	LDFLAGS_saved="$LDFLAGS"
  	LIBS_saved="$LIBS"

	AS_IF([test ! -z "$with_boost" -a "$with_boost" != "yes"],
		[BOOST_CPPFLAGS="-I$with_boost/include"
		 CPPFLAGS="$BOOST_CPPFLAGS $CPPFLAGS"
	    	 BOOST_LDFLAGS="-L$with_boost/lib"
	         BOOST_LIBDIR="$with_boost/lib"
	         LDFLAGS="$BOOST_LDFLAGS $LDFLAGS"],
		[BOOST_CPPFLAGS=
		 BOOST_LDFLAGS=
		 BOOST_LIBDIR=
  		])

	WANT_BOOST_VERSION=105600

	AC_LANG_PUSH(C++)
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([[@%:@include <boost/version.hpp>]],
                	[[
                               #if BOOST_VERSION >= $WANT_BOOST_VERSION
                               // Everything is okay
                               #else
                               #  error Boost version is too old
                               #endif
                         ]])
		],
                [AC_MSG_RESULT([yes])],
		[AC_MSG_RESULT([no])
		 sst_check_boost_happy="no"]
		)
	AC_LANG_POP(C++)

	CPPFLAGS="$CPPFLAGS_saved"
  	LDFLAGS="$LDFLAGS_saved"
  	LIBS="$LIBS_saved"

	AC_SUBST([BOOST_CPPFLAGS])
	AC_SUBST([BOOST_LDFLAGS])
	AC_SUBST([BOOST_LIBDIR])

	AS_IF([test "$sst_check_boost_happy" = "yes"],
		[AC_DEFINE([HAVE_BOOST], [1], [Set to 1 DRAMSim was found])])
	AS_IF([test "$sst_check_boost_happy" = "yes"], [$1], [$2])

])
