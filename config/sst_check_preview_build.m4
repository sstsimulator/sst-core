AC_DEFUN([SST_CHECK_PREVIEW_BUILD],
[
  AC_ARG_ENABLE([preview-build],
    [AS_HELP_STRING([--enable-preview-build],
      [Enable or disable preview build (Default: disabled)])],
    [
      enable_sst_preview_build=$enableval
    ], [
      enable_sst_preview_build=no
    ]
  )

AM_CONDITIONAL([SST_ENABLE_PREVIEW_BUILD], [test "x$enable_sst_preview_build" = "xyes"])
AS_IF([test "x$enable_sst_preview_build" = "xyes"], [AC_DEFINE([SST_ENABLE_PREVIEW_BUILD],[1],[Defines whether preview build is enabled])])

AH_TEMPLATE([SST_ENABLE_PREVIEW_BUILD],[Define to enable preview build])
if test "x$enable_sst_preview_build" = "xyes"; then
  AC_DEFINE_UNQUOTED([SST_ENABLE_PREVIEW_BUILD], 1, [Whether to enable preview build])
fi

])

