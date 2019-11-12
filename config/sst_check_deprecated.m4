AC_DEFUN([SST_CHECK_DEPRECATED],
[
  sst_check_hdf5_happy="yes"

  AC_ARG_ENABLE([deprecated],
    [AS_HELP_STRING([--(dis|en)able-deprecated],
      [Enable or disable deprecated code (Default: enabled)])],
    [
      enable_sst_deprecated=$enableval
    ], [
      enable_sst_deprecated=yes
    ]
  )

AH_TEMPLATE([SST_ENABLE_DEPRECATED],[Define to enable deprecated code])
if test "x$enable_sst_deprecated" = "xyes"; then
  AC_DEFINE_UNQUOTED([SST_ENABLE_DEPRECATED], 1, [Whether to enable deprecated code])
fi

])

