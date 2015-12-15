

AC_INIT([SSTCore], [6.0.0], [sst-users@sst-simulator.org])

AC_PREREQ([2.59])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE([1.9.6 foreign dist-bzip2 subdir-objects no-define tar-ustar])

# If Automake supports silent rules, enable them.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_pattern_allow([LT_LIBEXT])

AC_CONFIG_HEADER([src/sst/core/sst_config.h])

# Lets check for the standard compilers and basic options
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_C99
AC_C_INLINE
AC_PROG_MAKE_SET

AC_PROG_CXX
AC_CACHE_SAVE

AC_PROG_CPP
AC_CANONICAL_HOST

AC_CACHE_SAVE
AC_HEADER_TIME

LT_CONFIG_LTDL_DIR([src/sst/core/libltdl])
LT_INIT([shared disable-static dlopen])
LTDL_INIT([recursive])

if test "x$with_included_ltdl" != "xyes"; then
   save_CFLAGS="$CFLAGS"
   save_LDFLAGS="$LDFLAGS"
   save_LIBS="$LIBS"
   CFLAGS="$CFLAGS $LTDLINCL"
   LDFLAGS="$LDFLAGS $LIBLTDL"
   AC_CHECK_LIB([ltdl], [lt_dladvise_init],
                 [],
                 [AC_MSG_ERROR([installed libltdl is too old])])
   LIBS="$save_LIBS"
   LDFLAGS="$save_LDFLAGS"
   CFLAGS="$save_CFLAGS"
fi

AC_SUBST([LTDLINCL])
AC_SUBST([LIBLTDL])

dnl Work out the C++ standard which we are going to use
AX_CXX_COMPILE_STDCXX_1Y
AX_CXX_COMPILE_STDCXX_0X

AS_IF([test "x$found_cxx1y" = "xyes"],
        [CXXFLAGS="$CXXFLAGS $SST_CXX1Y_FLAGS"],
        [CXXFLAGS="$CXXFLAGS $SST_CXX0X_FLAGS"])

dnl Fix flags - seems to be only way to make libltdl play nice
CPPFLAGS='-I$(top_builddir) -I$(top_srcdir) -I$(top_srcdir)/src'" $CPPFLAGS"

AC_HEADER_TIME
AC_CHECK_HEADERS([c_asm.h dlfcn.h intrinsics.h mach/mach_time.h sys/time.h sys/stat.h sys/types.h unistd.h])

AC_CACHE_SAVE

SST_CHECK_MPI([], [AC_MSG_ERROR([Could not find MPI package])])
SST_CHECK_BOOST([], [AC_MSG_ERROR([Could not find Boost, this is required for SST to build])])

SST_CHECK_BOOST_POLYMORPHIC_TYPE([uint64_t], [want_polymorphic=1], [want_polymorphic=0])
SST_CHECK_BOOST_POLYMORPHIC_TYPE([long long], [], [want_polymorphic=0])
AC_DEFINE_UNQUOTED([SST_WANT_POLYMORPHIC_ARCHIVE], [$want_polymorphic],
  [defined to 1 if want boost polymorphic serialization archives])

SST_CHECK_PYTHON([], [AC_MSG_ERROR([Could not find Python, this is required for SST to build])])
SST_CHECK_LIBZ()
SST_CHECK_MEM_POOL()

SST_CHECK_ZOLTAN([have_zoltan=1],[have_zoltan=0],[AC_MSG_ERROR([Zoltan requested but not found])])
AS_IF([test "x$have_zoltan" = "x1"], [AC_DEFINE_UNQUOTED([HAVE_ZOLTAN], [1],
	[Define if you have the Zoltan library.])])
AM_CONDITIONAL([HAVE_ZOLTAN], [test "$have_zoltan" = 1])

AC_DEFINE_UNQUOTED([SST_INSTALL_PREFIX], ["$prefix"], [Defines the location SST will be installed in])

AC_DEFINE_UNQUOTED([SST_CPPFLAGS], ["$CPPFLAGS"], [Defines the CPPFLAGS used to build SST])
AC_DEFINE_UNQUOTED([SST_CFLAGS], ["$CFLAGS"], [Defines the CFLAGS used to build SST])
AC_DEFINE_UNQUOTED([SST_CXXFLAGS], ["$CXXFLAGS"], [Defines the CXXFLAGS used to build SST])
AC_DEFINE_UNQUOTED([SST_LDFLAGS], ["$LDFLAGS"], [Defines the LDFLAGS used to build SST])

AC_DEFINE_UNQUOTED([SST_CC], ["$CC"], [Defines the C compiler used to build SST])
AC_DEFINE_UNQUOTED([SST_CXX], ["$CXX"], [Defines the C++ compiler used to build SST])
AC_DEFINE_UNQUOTED([SST_LD], ["$LD"], [Defines the linker used to build SST])
AC_DEFINE_UNQUOTED([SST_MPICC], ["$MPICC"], [Defines the MPI C compiler used to build SST])
AC_DEFINE_UNQUOTED([SST_MPICXX], ["$MPICXX"], [Defines the MPI C++ compilers used to build SST])

AC_DEFINE_UNQUOTED([SST_CPP], ["$CPP"], [Defines the C preprocessor used to build SST])
AC_DEFINE_UNQUOTED([SST_CXXCPP], ["$CXXCPP"], [Defines the C++ preprocessor used to build SST])

AC_DEFINE_UNQUOTED([SST_PYTHON_CPPFLAGS], ["$PYTHON_CPPFLAGS"],
	[Defines the CPPFLAGS needed to compile Python into SST])
AC_DEFINE_UNQUOTED([SST_PYTHON_LDFLAGS], ["$PYTHON_LDFLAGS"],
	[Defines the LDFLAGS needed to compile Python into SST])

AC_DEFINE_UNQUOTED([SST_BOOST_CPPFLAGS], ["$BOOST_CPPFLAGS"],
	[Defines the CPPFLAGS needed to compile Boost into SST])
AC_DEFINE_UNQUOTED([SST_BOOST_LDFLAGS], ["$BOOST_LDFLAGS"],
	[Defines the LDFLAGS needed to compile Boost into SST])
AC_DEFINE_UNQUOTED([SST_BOOST_LIBS], ["$BOOST_LIBS"],
	[Defines the LIBS needed to compile Boost into SST])
AC_DEFINE_UNQUOTED([SST_BOOST_LIBDIR], ["$BOOST_LIBDIR"],
	[Defines the LIBDIR needed to load dynamic libraries for Boost])

AC_CACHE_SAVE

AC_CONFIG_FILES([
  Makefile
  src/sst/SST-${PACKAGE_VERSION}.conf:src/sst/sst.conf
  src/Makefile
  src/sst/Makefile
  src/sst/SST-${PACKAGE_VERSION}.pc:src/sst/sst.pc.in
  src/sst/core/Makefile
  src/sst/core/libltdl/Makefile
  src/sst/core/build_info.h:src/sst/core/build_info.h.in
])
AC_OUTPUT

