AC_DEFUN([SST_CHECK_BOOST_POLYMORPHIC_TYPE],
[
# check if polymorphic archives support type
AC_CACHE_CHECK([whether polymorphic archive supports ]$1,
  [AS_TR_SH([sst_cv_polymorphic_$1])],
  [AC_LANG_PUSH([C++])
CPPFLAGS_save="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <boost/archive/polymorphic_iarchive.hpp>
#include <boost/archive/polymorphic_oarchive.hpp>
#include <boost/serialization/export.hpp>
class Test {
    $1 tst;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & tst;
    }
};
BOOST_CLASS_EXPORT(Test)
]],[[]])],
  [AS_TR_SH([sst_cv_polymorphic_$1])="yes"],
  [AS_TR_SH([sst_cv_polymorphic_$1])="no"])
CPPFLAGS="$CPPFLAGS_save"
AC_LANG_POP([C++])])
AS_IF([test "$AS_TR_SH([sst_cv_polymorphic_$1])" = "yes"], [$2], [$3])
])
