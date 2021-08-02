AC_DEFUN([SST_CHECK_BACKTRACE], [
sst_check_backtrace_happy="yes"

AC_CHECK_HEADERS([execinfo.h], [], [sst_check_backtrace_happy="no"])
AC_CHECK_FUNCS([backtrace], [], [sst_check_backtrace_happy="no"])

AS_IF([test "x$sst_check_backtrace_happy" = "xyes"], [AC_DEFINE([HAVE_BACKTRACE], [1], [Defines if the backtrace function is avalible])])

AC_MSG_CHECKING([for backtrace function])
AC_MSG_RESULT([$sst_check_backtrace_happy])
])
