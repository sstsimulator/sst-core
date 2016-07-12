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
		 BOOST_LIBS=""
	         LDFLAGS="$BOOST_LDFLAGS $LDFLAGS"],
		[BOOST_CPPFLAGS=
		 BOOST_LDFLAGS=
		 BOOST_LIBDIR=
		 BOOST_LIBS=
  		])

	WANT_BOOST_VERSION=105600

	AC_MSG_CHECKING([Boost version is acceptable for SST])

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
		 sst_check_boost_happy="no"
		 AC_MSG_ERROR([Version of Boost supplied is too old or did not compile correctly.], [1])
		]
		)

	AC_MSG_CHECKING([Boost program options library can be successfully used])

dnl	Check whether the program options library can be compiled successfully
	LIBS="$LIBS -lboost_program_options"
	AC_LINK_IFELSE(
		[AC_LANG_PROGRAM([[@%:@include <boost/program_options.hpp>
			namespace po = boost::program_options;]],
			[[
				po::options_description example("Example Options");
        			example.add_options()
            			("help", "Compile Example")
        			;
			]])
		],
		[AC_MSG_RESULT([yes])
		 BOOST_LIBS="$BOOST_LIBS -lboost_program_options"],
                [AC_MSG_RESULT([no])
		 LIBS="$LIBS_saved -lboost_program_options-mt"
		 AC_MSG_CHECKING([Boost program options (multithreaded) library can be successfully used])
		 AC_LINK_IFELSE(
			[AC_LANG_PROGRAM([[@%:@include <boost/program_options.hpp>
				namespace po = boost::program_options;]],
				[[
					po::options_description example("Example Options");
        				example.add_options()
            				("help", "Compile Example")
        				;
				]])
			],
			[AC_MSG_RESULT([yes])
		 	 BOOST_LIBS="$BOOST_LIBS -lboost_program_options-mt"],
                	[AC_MSG_RESULT([no])
                 	 sst_check_boost_happy="no"
		 	 AC_MSG_ERROR([Boost Program Options cannot be successfully compiled.], [1])
			]
                	)
		]
                )

dnl     Check for boost_serialization library
	AC_MSG_CHECKING([Boost serialization library can be successfully used])
	LIBS="$LIBS -lboost_serialization"
	AC_LINK_IFELSE(
		[AC_LANG_PROGRAM([[@%:@include <fstream>
                                   @%:@include <boost/archive/text_oarchive.hpp>
                                   @%:@include <boost/archive/text_iarchive.hpp>]],
                                 [[std::ofstream ofs("filename");
                                        boost::archive::text_oarchive oa(ofs);
                                        return 0;]])
                ],
		[AC_MSG_RESULT([yes])
		 BOOST_LIBS="$BOOST_LIBS -lboost_serialization"],
                [AC_MSG_RESULT([no])
		 LIBS="$LIBS_saved -lboost_serialization-mt"
		 AC_MSG_CHECKING([Boost serialization (multithreaded) library can be successfully used])
		 AC_LINK_IFELSE(
		        [AC_LANG_PROGRAM([[@%:@include <fstream>
                                   @%:@include <boost/archive/text_oarchive.hpp>
                                   @%:@include <boost/archive/text_iarchive.hpp>]],
                                 [[std::ofstream ofs("filename");
                                        boost::archive::text_oarchive oa(ofs);
                                        return 0;]])
                        ],
			[AC_MSG_RESULT([yes])
		 	 BOOST_LIBS="$BOOST_LIBS -lboost_serialization-mt"],
                	[AC_MSG_RESULT([no])
                 	 sst_check_boost_happy="no"
		 	 AC_MSG_ERROR([Boost Serialization cannot be successfully compiled.], [1])
			]
                	)
		]
                )

	LIBS="$LIBS_saved"
	AC_LANG_POP(C++)

	CPPFLAGS="$CPPFLAGS_saved"
  	LDFLAGS="$LDFLAGS_saved"
  	LIBS="$LIBS_saved"

	AC_SUBST([BOOST_CPPFLAGS])
	AC_SUBST([BOOST_LDFLAGS])
	AC_SUBST([BOOST_LIBDIR])
	AC_SUBST([BOOST_LIBS])

	AS_IF([test "$sst_check_boost_happy" = "yes"],
		[AC_DEFINE([HAVE_BOOST], [1], [Set to 1 Boost was found])])
	AS_IF([test "$sst_check_boost_happy" = "yes"],
		[AC_DEFINE_UNQUOTED([BOOST_LIBDIR], ["$BOOST_LIBDIR"], [Library directory where Boost can be found.])])
	AS_IF([test "$sst_check_boost_happy" = "yes"], [$1], [$2])

])
