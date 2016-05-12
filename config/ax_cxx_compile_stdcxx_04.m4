# ============================================================================
#  http://www.gnu.org/software/autoconf-archive/ax_cxx_compile_stdcxx_0x.html
# ============================================================================
#
# SYNOPSIS
#
#   AX_CXX_COMPILE_STDCXX_0X
#
# DESCRIPTION
#
#   Check for baseline language coverage in the compiler for the C++0x
#   standard.
#
# LICENSE
#
#   Copyright (c) 2008 Benjamin Kosnik <bkoz@redhat.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 7

AU_ALIAS([AC_CXX_COMPILE_STDCXX_0X], [AX_CXX_COMPILE_STDCXX_0X])

AC_DEFUN([AX_CXX_COMPILE_STDCXX_0X], [
  SST_CXX0X_FLAGS=""

  AC_CACHE_CHECK(if C++ supports C++0x features without additional flags,
  ax_cv_cxx_compile_cxx0x_native,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([
  template <typename T>
    struct check
    {
      static_assert(sizeof(int) <= sizeof(T), "not big enough");
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c;
    check_type&& cr = static_cast<check_type&&>(c);],,
  ax_cv_cxx_compile_cxx0x_native=yes, ax_cv_cxx_compile_cxx0x_native=no)
  AC_LANG_RESTORE
  ])

  SST_CXX0X_FLAGS=""

  AC_CACHE_CHECK(if C++ supports C++0x features with -std=c++11,
  ax_cv_cxx_compile_cxx11_cxx,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=c++11 -D__STDC_FORMAT_MACROS"
  AC_TRY_COMPILE([
  template <typename T>
    struct check
    {
      static_assert(sizeof(int) <= sizeof(T), "not big enough");
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c;
    check_type&& cr = static_cast<check_type&&>(c);],,
  ax_cv_cxx_compile_cxx11_cxx=yes, ax_cv_cxx_compile_cxx11_cxx=no)
  CXXFLAGS=$ac_save_CXXFLAGS
  AS_IF([test "$ax_cv_cxx_compile_cxx11_cxx" = "yes" ], [SST_CXX0X_FLAGS="-std=c++11 -D__STDC_FORMAT_MACROS"])

  AC_LANG_RESTORE
  ])

  AS_IF([test "$ax_cv_cxx_compile_cxx11_cxx" != "yes"], [

  AC_CACHE_CHECK(if C++ supports C++0x features with -std=c++0x,
  ax_cv_cxx_compile_cxx0x_cxx,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=c++0x -D__STDC_FORMAT_MACROS"
  AC_TRY_COMPILE([
  template <typename T>
    struct check
    {
      static_assert(sizeof(int) <= sizeof(T), "not big enough");
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c;
    check_type&& cr = static_cast<check_type&&>(c);],,
  ax_cv_cxx_compile_cxx0x_cxx=yes, ax_cv_cxx_compile_cxx0x_cxx=no)
  CXXFLAGS=$ac_save_CXXFLAGS
  AS_IF([test "$ax_cv_cxx_compile_cxx0x_cxx" = "yes" ], [SST_CXX0X_FLAGS="-std=c++0x -D__STDC_FORMAT_MACROS"])
  AC_LANG_RESTORE

  ])
  ])

  AS_IF( [test "$ax_cv_cxx_compile_cxx0x_native" = "yes" -o "$ax_cv_cxx_compile_cxx0x_cxx" = "yes" -o "$ax_cv_cxx_compile_cxx11_cxx" = "yes" ], 
	[found_cxx0x="yes"], [found_cxx0x="no"] )

  AS_IF( [test "x$found_cxx1y" = "xno" -a "x$found_cxx0x" = "xno"],
	[AC_MSG_ERROR([Could not found C++14 or C++11 support but this is required for SST to successfully build.], [1]) ])
  AC_SUBST([SST_CXX0X_FLAGS])
])
