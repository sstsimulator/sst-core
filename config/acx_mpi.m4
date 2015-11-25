# ===========================================================================
#                http://autoconf-archive.cryp.to/acx_mpi.html
# ===========================================================================
#
# SYNOPSIS
#
#   ACX_MPI([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
#
# DESCRIPTION
#
#   This macro tries to find out how to compile programs that use MPI
#   (Message Passing Interface), a standard API for parallel process
#   communication (see http://www-unix.mcs.anl.gov/mpi/)
#
#   On success, it sets the MPICC, MPICXX, MPIF77, or MPIFC output variable
#   to the name of the MPI compiler, depending upon the current language.
#   (This may just be $CC/$CXX/$F77/$FC, but is more often something like
#   mpicc/mpiCC/mpif77/mpif90.) It also sets MPILIBS to any libraries that
#   are needed for linking MPI (e.g. -lmpi or -lfmpi, if a special
#   MPICC/MPICXX/MPIF77/MPIFC was not found).
#
#   If you want to compile everything with MPI, you should set:
#
#       CC="MPICC" #OR# CXX="MPICXX" #OR# F77="MPIF77" #OR# FC="MPIFC"
#       LIBS="$MPILIBS $LIBS"
#
#   NOTE: The above assumes that you will use $CC (or whatever) for linking
#   as well as for compiling. (This is the default for automake and most
#   Makefiles.)
#
#   The user can force a particular library/compiler by setting the
#   MPICC/MPICXX/MPIF77/MPIFC and/or MPILIBS environment variables.
#
#   ACTION-IF-FOUND is a list of shell commands to run if an MPI library is
#   found, and ACTION-IF-NOT-FOUND is a list of commands to run if it is not
#   found. If ACTION-IF-FOUND is not specified, the default action will
#   define HAVE_MPI.
#
# LICENSE
#
#   Copyright (c) 2008 Steven G. Johnson <stevenj@alum.mit.edu>
#   Copyright (c) 2008 Julian C. Cummings <cummings@cacr.caltech.edu>
#
#   This program is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation, either version 3 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <http://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

AC_DEFUN([AX_PROG_PREPROC_WORKS_IFELSE],
[ax_preproc_ok=false
_AC_PREPROC_IFELSE([AC_LANG_SOURCE([[@%:@ifdef __STDC__
@%:@ include <limits.h>
@%:@else
@%:@ include <assert.h>
@%:@endif
           Syntax error]])],
           [],
           [# Broken: fails on valid input.
continue])

  # OK, works on sane cases. Now check whether nonexistent headers
  # can be detected and how.
_AC_PREPROC_IFELSE([AC_LANG_SOURCE([[@%:@include <ax_nonexistent.h>]])],
           [# Broken: success on invalid input.
continue],
           [# Passes both tests.
ac_preproc_ok=true
break])

# Because of `break', _AC_PREPROC_IFELSE's cleaning code was skipped.
rm -f conftest.i conftest.err conftest.$ac_ext
AS_IF([test "x$ac_preproc_ok" = xtrue], [$1], [$2])
])

AC_DEFUN([AX_PROG_MPICXXCPP],
[
dnl This is heavily stolen from autoconf's AC_PROG_CPP
    AC_ARG_VAR(MPICXXCPP,[MPI C++ preprocessor command])
    AC_LANG_PUSH([C++])
    AC_MSG_CHECKING([how to run the MPI C++ preprocessor])
    AS_IF([test -n "$MPICXXCPP" && test -d "$MPICXXCPP"],
          [MPICXXCPP=])
    AS_IF([test "x$MPICXXCPP" = "x"],
          [AC_CACHE_VAL([ax_cv_prog_MPICXXCPP],
           [dnl
             CXXCPP_save="$CXXCPP"
             # Double quotes because CPP needs to be expanded
             for CXXCPP in "$MPICXX -E" "$MPICXX -E -traditional-cpp" "$MPICC -E" "$MPICC -E -traditional-cpp" "/lib/cpp"
             do
               AX_PROG_PREPROC_WORKS_IFELSE([break])
             done
             ax_cv_prog_MPICXXCPP="$CXXCPP"
             CXXCPP="$CXXCPP_save"
             ])dnl
             MPICXXCPP="$ax_cv_prog_MPICXXCPP"],
          [ac_cv_prog_MPICXXCPP="$CXXCPP"])
    AC_MSG_RESULT([$MPICXXCPP])
    AX_PROG_PREPROC_WORKS_IFELSE([],
           [AC_MSG_FAILURE([MPI C++ preprocessor "$MPICXXCPP" fails sanity check])])
    AC_SUBST(MPICXXCPP)
    AC_LANG_POP([C++])
])

AC_DEFUN([AX_PROG_MPICPP],
[
dnl This is heavily stolen from autoconf's AC_PROG_CPP
    AC_ARG_VAR(MPICPP,[MPI C preprocessor command])
    AC_LANG_PUSH(C)
    AC_MSG_CHECKING([how to run the MPI C preprocessor])
    AS_IF([test -n "$MPICPP" && test -d "$MPICPP"],
          [MPICPP=])
    AS_IF([test "x$MPICPP" = "x"],
          [AC_CACHE_VAL([ax_cv_prog_MPICPP],
           [dnl
             CPP_save="$CPP"
             # Double quotes because CPP needs to be expanded
             for CPP in "$MPICC -E" "$MPICC -E -traditional-cpp" "/lib/cpp"
             do
               AX_PROG_PREPROC_WORKS_IFELSE([break])
             done
             ax_cv_prog_MPICPP="$CPP"
             CPP="$CPP_save"
             ])dnl
             MPICPP="$ax_cv_prog_MPICPP"],
          [ac_cv_prog_MPICPP="$CPP"])
    AC_MSG_RESULT([$MPICPP])
    AX_PROG_PREPROC_WORKS_IFELSE([],
           [AC_MSG_FAILURE([MPI C preprocessor "$MPICPP" fails sanity check])])
    AC_SUBST(MPICPP)
    AC_LANG_POP(C)
])

AC_DEFUN([ACX_MPI], [
AC_PREREQ(2.50) dnl for AC_LANG_CASE

AC_LANG_CASE([C], [
    AC_REQUIRE([AC_PROG_CC])
    AC_ARG_VAR(MPICC,[MPI C compiler command])
    AC_CHECK_PROGS(MPICC, mpicc hcc mpxlc_r mpxlc mpcc cmpicc, $CC)
    acx_mpi_save_CC="$CC"
    CC="$MPICC"
    AX_PROG_MPICPP()
    AC_SUBST(MPICC)
],
[C++], [
    AC_REQUIRE([AC_PROG_CXX])
    AC_ARG_VAR(MPICXX,[MPI C++ compiler command])
    AC_CHECK_PROGS(MPICXX, mpic++ mpicxx mpiCC hcp mpxlC_r mpxlC mpCC cmpic++, $CXX)
    acx_mpi_save_CXX="$CXX"
    CXX="$MPICXX"
	AX_PROG_MPICXXCPP()
    AC_SUBST(MPICXX)
])

if test x = x"$MPILIBS"; then
    AC_LANG_CASE([C], [AC_CHECK_FUNC(MPI_Init, [MPILIBS=" "])],
        [C++], [AC_CHECK_FUNC(MPI_Init, [MPILIBS=" "])])
fi
if test x = x"$MPILIBS"; then
    AC_CHECK_LIB(mpi, MPI_Init, [MPILIBS="-lmpi"])
fi
if test x = x"$MPILIBS"; then
    AC_CHECK_LIB(mpich, MPI_Init, [MPILIBS="-lmpich"])
fi

dnl We have to use AC_TRY_COMPILE and not AC_CHECK_HEADER because the
dnl latter uses $CPP, not $CC (which may be mpicc).
AC_LANG_CASE([C], [AS_IF([test x != x"$MPILIBS"],
   [AC_MSG_CHECKING([for C mpi.h])
    AC_TRY_COMPILE([#include <mpi.h>],[],[AC_MSG_RESULT(yes)], [MPILIBS=""
        AC_MSG_RESULT(no)])])],
[C++], [AS_IF([test x != x"$MPILIBS"],
   [AC_MSG_CHECKING([for C++ mpi.h])
    AC_TRY_COMPILE([#include <mpi.h>],[],[AC_MSG_RESULT(yes)], [MPILIBS=""
        AC_MSG_RESULT(no)])])])

AC_LANG_CASE([C], [CC="$acx_mpi_save_CC"],
    [C++], [CXX="$acx_mpi_save_CXX"],
    [Fortran 77], [F77="$acx_mpi_save_F77"],
    [Fortran], [FC="$acx_mpi_save_FC"])

AC_SUBST(MPILIBS)

# try to figure out include directory.  We only need the C API, so only do it once.     
if test "$MPI_CPPFLAGS" = "" ; then
    MPI_CPPFLAGS=

    # Open MPI
    incdir=`${MPICC} -showme:incdirs 2>/dev/null`
    if test $? -eq 0 ; then
        for flag in $incdir ; do
            MPI_CPPFLAGS="$MPI_CPPFLAGS -I${flag}"    
        done
    else
        # MPICH2
        flags=`${MPICC} -compile-info 2>/dev/null`
        if test $? -eq 0 ; then
            for flag in $flags ; do
                if echo $flag | grep -q '^\-I' ; then
                    MPI_CPPFLAGS="$MPI_CPPFLAGS $flag"
                fi
            done
        fi
    fi

    AC_SUBST(MPI_CPPFLAGS)
fi

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x = x"$MPILIBS"; then
        $2
        :
else
        ifelse([$1],,[AC_DEFINE(SST_CONFIG_HAVE_MPI,1,[Define if you have the MPI library.])],[$1])
        :
fi
])dnl ACX_MPI

dnl vim:set expandtab
