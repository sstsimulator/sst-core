
AC_DEFUN([SST_ENABLE_CORE_PROFILE], [

  AC_ARG_ENABLE([profile],
    [AS_HELP_STRING([--enable-profile],
      [Enables performance profiling of core features.  Slight performance penalties incurred.])])

  AS_IF([test "x$enable_profile" = "xyes" ],
	[AC_DEFINE([__SST_ENABLE_PROFILE__], [1], [Defines if core should have profiling enabled.])])

])
