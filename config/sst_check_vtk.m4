AC_DEFUN([SST_CHECK_VTK], [

  SAVE_CPPFLAGS="$CPPFLAGS"
  SAVE_LDFLAGS="$LDFLAGS"

  AC_ARG_WITH(vtk,
    [AS_HELP_STRING(
      [--with-vtk],
      [Give path to VTK installation],
      )],
    [ vtk_path=$withval ],
    [ vtk_path="" ]
  )


  AC_ARG_ENABLE(vtk,
    [AS_HELP_STRING(
      [--(dis|en)able-vtk],
      [Give VTK version number to enable VTK visualization],
      )],
    [ vtk_version=$enableval ],
    [ vtk_version=no ]
  )
  AH_TEMPLATE([VTK_ENABLED], [Whether VTK is available and VTK stats should be built])
  if test "$vtk_version" != "no"; then
  if test "$vtk_version" = "yes"; then
  AC_MSG_ERROR([Must give VTK version --enable-vtk=VERSION])
  fi
  if test -z "$vtk_path"; then
  AC_MSG_ERROR([VTK can only be enabled by giving the install prefix --with-vtk=PREFIX, even for system default path])
  fi
  fi

  if test "$vtk_version" = "no"; then
  if test -z "$vtk_path"; then
  ignore="ignore"
  else
  AC_MSG_ERROR([VTK path given as --with-vtk=PREFIX, but need version specified as --enable-vtk=VERSION])
  fi
  fi

  if test "X$vtk_version" = "Xno"; then
  AM_CONDITIONAL([HAVE_VTK], [false])
  else
  AM_CONDITIONAL([HAVE_VTK], [true])
  AC_DEFINE([HAVE_VTK],[1],[Defines whether we have the vtk library])
  AC_DEFINE_UNQUOTED([VTK_ENABLED], 1, [VTK is enabled and installed])
  VTK_CPPFLAGS="-I${vtk_path}/include/vtk-${vtk_version}"
  VTK_LIBS="-lvtkCommonCore-${vtk_version} -lvtkCommonDataModel-${vtk_version} -lvtkIOXML-${vtk_version} -lvtkIOExodus-${vtk_version}"
  VTK_LIBS="${VTK_LIBS} -lvtkCommonExecutionModel-${vtk_version}"
  VTK_LDFLAGS="-L${vtk_path}/lib -rpath ${vtk_path}/lib"
  AC_SUBST([VTK_CPPFLAGS])
  AC_SUBST([VTK_LIBS])
  AC_SUBST([VTK_LDFLAGS])
  fi

])

