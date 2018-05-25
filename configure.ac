

AC_INIT([SSTCore], [8.0.0], [sst@sandia.gov])

AC_PREREQ([2.59])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AC_PREFIX_DEFAULT([/usr/local])
AM_INIT_AUTOMAKE([1.9.6 foreign dist-bzip2 subdir-objects no-define tar-pax])

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

SST_CHECK_PICKY
AS_IF([test "x$use_picky" = "xyes"],
      [WARNFLAGS="-Wall -Wextra"],
      [WARNFLAGS=""])
CFLAGS="$CFLAGS $WARNFLAGS"
CXXFLAGS="$CXXFLAGS $WARNFLAGS"

dnl Fix flags - seems to be only way to make libltdl play nice
CPPFLAGS='-I$(top_builddir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(top_builddir)/src'" $CPPFLAGS"

AC_HEADER_TIME
AC_CHECK_HEADERS([c_asm.h dlfcn.h intrinsics.h mach/mach_time.h sys/time.h sys/stat.h sys/types.h unistd.h])

AC_CACHE_SAVE

AC_CHECK_PROG([DOXYGEN], [doxygen], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test "$DOXYGEN" = "doxygen"])

SST_CHECK_OSX()
SST_CHECK_MPI([], [AC_MSG_ERROR([Could not find MPI package])])
SST_CHECK_BOOST([], [])

SST_CHECK_PYTHON([], [AC_MSG_ERROR([Could not find Python, this is required for SST to build])])
SST_CHECK_LIBZ([have_zlib="yes"],[have_zlib="no"],[AC_MSG_ERROR([zlib was requested but could not be found.])])
SST_CHECK_HDF5()
SST_CHECK_MEM_POOL()

SST_ENABLE_DEBUG_OUTPUT()
SST_ENABLE_DEBUG_EVENT_TRACKING()
SST_ENABLE_CORE_PROFILE()

SST_CHECK_FPIC()

SST_CHECK_ZOLTAN([have_zoltan=1],[have_zoltan=0],[AC_MSG_ERROR([Zoltan requested but not found])])
AS_IF([test "x$have_zoltan" = "x1"], [AC_DEFINE_UNQUOTED([HAVE_ZOLTAN], [1],
	[Define if you have the Zoltan library.])])
AM_CONDITIONAL([HAVE_ZOLTAN], [test "$have_zoltan" = 1])

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

AC_DEFINE([__STDC_FORMAT_MACROS], [1], [Defines that standard PRI macros should be enabled])

AC_MSG_CHECKING([for SST-Core Git Branch, Head SHA and Commit Count])
if test -d ".git" ; then
    SSTCORE_GIT_BRANCH=`git rev-parse --abbrev-ref HEAD`
    AC_MSG_RESULT([${SSTCORE_GIT_BRANCH}])
    SSTCORE_GIT_HEADSHA=`git rev-parse HEAD`
    AC_MSG_RESULT([${SSTCORE_GIT_HEADSHA}])
	SSTCORE_GIT_COMMITCOUNT=`git rev-list HEAD | wc -l | sed -e 's/^ *//g'`
	AC_MSG_RESULT([${SSTCORE_GIT_COMMITCOUNT}])
else
    SSTCORE_GIT_BRANCH="N/A"
    AC_MSG_RESULT([${SSTCORE_GIT_BRANCH}])
    SSTCORE_GIT_HEADSHA="${PACKAGE_VERSION}"
    AC_MSG_RESULT([${SSTCORE_GIT_HEADSHA}])
	SSTCORE_GIT_COMMITCOUNT="0"
	AC_MSG_RESULT([${SSTCORE_GIT_COMMITCOUNT}])
fi

AC_SUBST(SSTCORE_GIT_BRANCH)
AC_DEFINE_UNQUOTED([SSTCORE_GIT_BRANCH], ["$SSTCORE_GIT_BRANCH"], [SST-Core Git Branch])

AC_SUBST(SSTCORE_GIT_HEADSHA)
AC_DEFINE_UNQUOTED([SSTCORE_GIT_HEADSHA], ["$SSTCORE_GIT_HEADSHA"], [SST-Core Git Head SHA])

AC_SUBST(SSTCORE_GIT_COMMITCOUNT)
AC_DEFINE_UNQUOTED([SSTCORE_GIT_COMMITCOUNT], ["$SSTCORE_GIT_COMMITCOUNT"], [SST-Core Git Commit Count])

CC_VERSION=`$CC --version | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n/%/g' | awk -F'%' '{print $1}'`
AC_SUBST(CC_VERSION)

MPICC_VERSION=`$MPICC --version | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n/%/g' | awk -F'%' '{print $1}'`
AC_SUBST(MPICC_VERSION)

AC_CACHE_SAVE

AC_DEFINE_UNQUOTED([SST_INSTALL_PREFIX], ["$prefix"], [Defines the location SST will be installed in])

AC_CACHE_SAVE

dnl Remove flags like -g, -O, and -W, that are not needed by other elements
SST_EXPORT_CXXFLAGS=`echo "$CXXFLAGS" | sed -E -e 's/ *-(g|O.|W@<:@^\s@:>@+)//g'`
SST_EXPORT_CFLAGS=`echo "$CFLAGS" | sed -E -e 's/ *-(g|O.|W@<:@^\s@:>@+)//g'`
AC_SUBST(SST_EXPORT_CXXFLAGS)
AC_SUBST(SST_EXPORT_CFLAGS)

AC_CONFIG_FILES([
  Makefile
  doc/Makefile
  doc/Doxyfile
  src/sst/sstsimulator.conf:src/sst/sst.conf
  src/Makefile
  src/sst/Makefile
  src/sst/SST-${PACKAGE_VERSION}.pc:src/sst/sst.pc.in
  src/sst/core/Makefile
  src/sst/core/libltdl/Makefile
  src/sst/core/build_info.h:src/sst/core/build_info.h.in
])
AC_OUTPUT

echo ""
echo "-------------------------------------------------------"
echo "SST Core Configuration completed."
echo "-------------------------------------------------------"
echo "Build Environment:"
echo ""
printf "%38s : %s\n" "SST-Core Version" "$PACKAGE_VERSION"
if test "x$SSTCORE_GIT_HEADSHA" != "x$PACKAGE_VERSION"; then
printf "%38s : %s\n" "Git Branch" "$SSTCORE_GIT_BRANCH"
printf "%38s : %s\n" "Git HEAD SHA" "$SSTCORE_GIT_HEADSHA"
printf "%38s : %s\n" "Branch Commit Count" "$SSTCORE_GIT_COMMITCOUNT"
else 
printf "%38s : %s\n" "Version" "SST-Core $PACKAGE_VERSION"
fi
printf "%38s : %s\n" "Prefix" "$prefix"
printf "%38s : %s\n" "Preprocessor" "$CPP"
printf "%38s : %s\n" "C++ Compiler" "$CXX"
printf "%38s : %s\n" "C Compiler" "$CC"
printf "%38s : %s\n" "Compiler Version" "$CC_VERSION"
printf "%38s : %s\n" "MPI C Compiler" "$MPICC"
printf "%38s : %s\n" "MPI C++ Compiler" "$MPICXX"
printf "%38s : %s\n" "MPI Compiler Version" "$MPICC_VERSION"
printf "%38s : %s\n" "Linker" "$LD"
printf "%38s : %s\n" "Preprocessor Options" "$CPPFLAGS"
printf "%38s : %s\n" "C Compiler Options" "$CFLAGS"
printf "%38s : %s\n" "C++ Compiler Options" "$CXXFLAGS"

if test "x$found_cxx1y" = "xyes"; then
printf "%38s : %s\n" "C++ Standard Compliance" "C++1Y/14"
else
printf "%38s : %s\n" "C++ Standard Compliance" "C++11"
fi

printf "%38s : %s\n" "Linker Options" "$LDFLAGS"
echo "-------------------------------------------------------"
echo "Configuration Information (Dependencies):"
echo ""

if test "x$sst_check_boost_happy" = "xyes" ; then
    printf "%38s : YES\n" "Boost Library"
    printf "%38s : %s\n" "Boost Library Preprocessor" "$BOOST_CPPFLAGS"
    printf "%38s : %s\n" "Boost Library Linker Options" "$BOOST_LDFLAGS"
else
    printf "%38s : No\n" "Boost Library"
fi

if test "x$sst_check_zoltan_happy" = "xyes" ; then
    printf "%38s : YES\n" "Zoltan Partitioner"
else
    printf "%38s : No\n" "Zoltan Partitioner"
fi
if test "x$sst_check_hdf5_happy" = "xyes" ; then
    printf "%38s : YES\n" "HDF5 Support"
else
    printf "%38s : No\n" "HDF5 Support"
fi
if test "x$sst_check_libz_happy" = "xyes" ; then
    printf "%38s : YES\n" "libz compression library"
else
    printf "%38s : No\n" "libz compression library"
fi

echo "-------------------------------------------------------" 

