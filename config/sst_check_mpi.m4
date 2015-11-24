AC_DEFUN([SST_CHECK_MPI], [
  AC_ARG_ENABLE([mpi],
     [AC_HELP_STRING([--disable-mpi],
                     [Disable support for multi-node simulations using MPI.])])
  AS_IF([test "$enable_mpi" = "no"], [sst_use_mpi=0], [sst_use_mpi=1])
  SST_CHECK_USING_MPI([
    AC_LANG_PUSH([C])
    ACX_MPI([], [$2])
    AC_LANG_POP([C])
    AC_LANG_PUSH([C++])
    ACX_MPI([], [$2])
    AC_LANG_POP([C++])
    $1],
   [MPICC=$CC
    MPICXX=$CXX])
  AM_CONDITIONAL([SST_USE_MPI], [test $sst_use_mpi -eq 1])
])

AC_DEFUN([SST_CHECK_USING_MPI], [AS_IF([test $sst_use_mpi -eq 1], [$1], [$2])])
