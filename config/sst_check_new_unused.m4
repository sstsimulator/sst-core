AC_DEFUN([SST_CHECK_NEW_UNUSED], [
  AC_ARG_ENABLE([new-unused], AS_HELP_STRING([--disable-new-unused], [Disable the new UNUSED() macro]))
  new_unused=yes
  AS_IF([test "x$enable_new_unused" != "xno"],
    [ AC_DEFINE([SST_NEW_UNUSED], [1], [Defines that new UNUSED macro should be enabled]) ],
    [ new_unused=no ])
])
