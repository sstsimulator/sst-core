AC_DEFUN([SST_CHECK_OSX],
[
  sst_check_os_happy="yes"

  AC_MSG_CHECKING([for Mac OSX])
  AS_IF([test `uname` = "Darwin"], [sst_check_os_happy="yes"], [sst_check_os_happy="no"])

  AS_IF([test "$sst_check_os_happy" = "yes"], 
	[AC_DEFINE([SST_COMPILE_MACOSX], [1], [Defines compile for Mac OS-X])])
  AM_CONDITIONAL([SST_COMPILE_OSX], [test "x$sst_check_os_happy" = "xyes"])

  AS_IF([test "$sst_check_os_happy" = "yes"], 
	[AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no])])

  AS_IF([test "$sst_check_os_happy" = "yes"], [$1], [$2])
])
