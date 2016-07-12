
AU_ALIAS([AC_CXX_COMPILE_STDCXX_1Y], [AX_CXX_COMPILE_STDCXX_1Y])

AC_DEFUN([AX_CXX_COMPILE_STDCXX_1Y], [
  SST_CXX1Y_FLAGS=""

  AC_CACHE_CHECK(if C++ supports C++1y features without additional flags,
  ax_cv_cxx_compile_cxx1y_native,
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
  ax_cv_cxx_compile_cxx1y_native=yes, ax_cv_cxx_compile_cxx1y_native=no)
  AC_LANG_RESTORE
  ])

  SST_CXX1Y_FLAGS=""

  AC_CACHE_CHECK(if C++ supports C++1y features with -std=c++14,
  ax_cv_cxx_compile_cxx14_cxx,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=c++14 -D__STDC_FORMAT_MACROS"
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
  ax_cv_cxx_compile_cxx_14_cxx=yes, ax_cv_cxx_compile_cxx_14_cxx=no)
  CXXFLAGS=$ac_save_CXXFLAGS
  AS_IF([test "$ax_cv_cxx_compile_cxx_14_cxx" = "yes" ], [SST_CXX1Y_FLAGS="-std=c++14 -D__STDC_FORMAT_MACROS"])

  AC_LANG_RESTORE
  ])

  AS_IF([test "$ax_cv_cxx_compile_cxx14_cxx" != "yes"], [

  AC_CACHE_CHECK(if C++ supports C++1y features with -std=c++1y,
  ax_cv_cxx_compile_cxx1y_cxx,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=c++1y -D__STDC_FORMAT_MACROS"
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
  ax_cv_cxx_compile_cxx1y_cxx=yes, ax_cv_cxx_compile_cxx1y_cxx=no)
  CXXFLAGS=$ac_save_CXXFLAGS
  AS_IF([test "$ax_cv_cxx_compile_cxx1y_cxx" = "yes" ], [SST_CXX1Y_FLAGS="-std=c++1y -D__STDC_FORMAT_MACROS"])
  AC_LANG_RESTORE

  ])
  ])

  AS_IF( [test "$ax_cv_cxx_compile_cxx1y_native" = "yes" -o "$ax_cv_cxx_compile_cxx1y_cxx" = "yes" -o "$ax_cv_cxx_compile_cxx14_cxx" = "yes" ], 
	[found_cxx1y="yes"], [found_cxx1y="no"] )

  AC_DEFINE(HAVE_STDCXX_1Y, [1], [Define if C++ supports C++14 features.])
  AC_SUBST([SST_CXX1Y_FLAGS])
  AM_CONDITIONAL([HAVE_STDCXX_1Y], [test "x$found_cxx1y" = "xyes"])
])

