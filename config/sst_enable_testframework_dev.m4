AC_DEFUN([SST_ENABLE_TESTFRAMEWORK_DEV], [

  AC_ARG_ENABLE([testframework-dev],
    [AS_HELP_STRING([--enable-testframework-dev],
      [Enables sym-link of test frameworks components rather than copy when installing sst-core (Allows development of test framework components).])])

  AM_CONDITIONAL([SST_TESTFRAMEWORK_DEV], [test "x$enable_testframework_dev" = "xyes"])
  AS_IF([test "x$enable_testframework_dev" = "xyes" ],
	[AC_DEFINE([SST_TESTFRAMEWORK_DEV], [1], [Test Frameworks will be sym-linked instead of copied on install])])

])
