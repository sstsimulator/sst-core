
AC_INIT([SSTCore], [-dev], [wg-sst@sandia.gov])

AC_PREREQ([2.59])
AC_COPYRIGHT([Copyright National Technology and Engineering Solutions of Sandia (NTESS), 2004-2023])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AC_PREFIX_DEFAULT([/usr/local])
AM_INIT_AUTOMAKE([1.9.6 foreign dist-bzip2 subdir-objects no-define tar-pax])

# If Automake supports silent rules, enable them.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_pattern_allow([LT_LIBEXT])

AC_CONFIG_HEADERS([src/sst/core/sst_config.h])

# Lets check for the standard compilers and basic options
AC_PROG_CC
AM_PROG_CC_C_O
m4_if(m4_defn([AC_AUTOCONF_VERSION]), 2.71,, m4_defn([AC_AUTOCONF_VERSION]), 2.70,, [AC_PROG_CC_C99]) 
AC_C_INLINE
AC_PROG_MAKE_SET

AC_PROG_CXX
AC_CACHE_SAVE

AC_PROG_CPP
AC_CANONICAL_HOST

AC_CACHE_SAVE

LT_INIT([shared disable-static dlopen])

dnl Work out the C++ standard which we are going to use
AX_CXX_COMPILE_STDCXX_17(noext)

CXXFLAGS="$CXXFLAGS -std=c++17"
#AS_IF([test "x$found_cxx1y" = "xyes"],
#        [CXXFLAGS="$CXXFLAGS $SST_CXX1Y_FLAGS"],
#        [CXXFLAGS="$CXXFLAGS $SST_CXX0X_FLAGS"])

# Temporary fix to ensure tests pass with newer Apple clang compiler
case "${host_os}" in
    darwin*)
    CXXFLAGS="$CXXFLAGS -ffp-contract=off"
    ;;
esac

SST_CHECK_PICKY
AS_IF([test "x$use_picky" = "xyes"],
      [WARNFLAGS="-Wall -Wextra"],
      [WARNFLAGS=""])
CFLAGS="$CFLAGS $WARNFLAGS"
CXXFLAGS="$CXXFLAGS $WARNFLAGS"

CPPFLAGS='-I$(top_srcdir)/src -I$(top_builddir)/src'" $CPPFLAGS"

AC_CHECK_HEADERS([c_asm.h dlfcn.h intrinsics.h mach/mach_time.h sys/time.h sys/stat.h sys/types.h unistd.h])

AC_CACHE_SAVE

AC_CHECK_PROG([DOXYGEN], [doxygen], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test "$DOXYGEN" = "doxygen"])

SST_CHECK_PREVIEW_BUILD()
SST_ENABLE_TESTFRAMEWORK_DEV()

SST_CHECK_OSX()
SST_CHECK_MPI([], [AC_MSG_ERROR([Could not find MPI package])])

SST_CHECK_PYTHON([], [AC_MSG_ERROR([Could not find Python, this is required for SST to build])])
SST_CHECK_LIBZ([have_zlib="yes"],[have_zlib="no"],[AC_MSG_ERROR([zlib was requested but could not be found.])])
SST_CHECK_CURSES()
SST_CHECK_BACKTRACE()
SST_CHECK_HDF5()
SST_CHECK_MEM_POOL()

SST_ENABLE_DEBUG_OUTPUT()
SST_ENABLE_DEBUG_EVENT_TRACKING()
SST_ENABLE_PERF_TRACKING()

AS_IF([test "$use_mempool" = "no" -a "$enable_debug_event_tracking" = "yes"],
[AC_MSG_ERROR([Event Tracking cannot be enabled with mem-pools disabled.])])

SST_ENABLE_CORE_PROFILE()

SST_CHECK_FPIC()

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
export_flag_cleanup() {
  echo "$1" | sed -E -e 's/ *-(@<:@gO@:>@@<:@^@<:@:space:@:>@@:>@*|W@<:@^@<:@:space:@:>@@:>@+)//g'
}
SST_EXPORT_CXXFLAGS=`export_flag_cleanup "$CXXFLAGS"`
SST_EXPORT_CFLAGS=`export_flag_cleanup "$CFLAGS"`
AC_SUBST(SST_EXPORT_CXXFLAGS)
AC_SUBST(SST_EXPORT_CFLAGS)

AC_CONFIG_FILES([
  Makefile
  doc/Makefile
  doc/Doxyfile
  share/Makefile
  share/SSTConfig.cmake
  share/SSTConfigVersion.cmake
  src/sst/sstsimulator.conf:src/sst/sst.conf
  src/Makefile
  src/sst/Makefile
  src/sst/SST-${PACKAGE_VERSION}.pc:src/sst/sst.pc.in
  src/sst/core/Makefile
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
printf "%38s : %s\n" "C++ Standard Compliance" "C++17"

printf "%38s : %s\n" "Linker Options" "$LDFLAGS"
echo "-------------------------------------------------------"
echo "Configuration Information (Options):"
echo ""

if test "x$sst_use_mpi" = "x1" ; then
    printf "%38s : Yes\n" "MPI enabled"
else
    printf "%38s : No\n" "MPI enabled"
fi
if test "x$use_mempool" = "xyes" ; then
    printf "%38s : Yes\n" "Mempool enabled"
else
    printf "%38s : No\n" "Mempool enabled"
fi
if test "x$enable_debug" = "xyes" ; then
    printf "%38s : Yes\n" "Debug output enabled"
else
    printf "%38s : No\n" "Debug output enabled"
fi
if test "x$enable_debug_event_tracking" = "xyes" ; then
    printf "%38s : Yes\n" "Event tracking enabled"
else
    printf "%38s : No\n" "Event tracking enabled"
fi
if test "x$enable_profile" = "xyes" ; then
    printf "%38s : Yes\n" "Performance profiling enabled"
else
    printf "%38s : No\n" "Performance profiling enabled"
fi
echo "-------------------------------------------------------"
echo "Configuration Information (Dependencies):"
echo ""

printf "%38s : %s (%s)\n" "Found Python" "$PYTHON_EXE" "$PYTHON_VERSION"

if test "x$sst_check_hdf5_happy" = "xyes" ; then
    printf "%38s : Yes\n" "HDF5 Support"
else
    printf "%38s : No\n" "HDF5 Support"
fi
if test "x$sst_check_libz_happy" = "xyes" ; then
    printf "%38s : Yes\n" "libz compression library"
else
    printf "%38s : No\n" "libz compression library"
fi

if test "x$sst_check_curses_happy" = "xyes" ; then
    printf "%38s : %s %s\n" "curses library" "$CURSES_LIBS"
else
    printf "%38s : Not found\n" "curses library"
fi

echo "-------------------------------------------------------"
