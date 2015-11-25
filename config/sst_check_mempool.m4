
AC_DEFUN([SST_CHECK_MEM_POOL], [
  use_mempool="yes"

  AC_ARG_ENABLE([mem-pools], [AS_HELP_STRING([--disable-mem-pools],
      [Disable the use of memory pools within the SST core.])])

  AS_IF([test "x$enable_mem_pools" = "xno"], [use_mempool="no"])
 
  AS_IF([test "$use_mempool" = "yes"], [AC_DEFINE([USE_MEMPOOL], [1], 
	[Set to 1 to use memory pools in the SST core]) ])
])
