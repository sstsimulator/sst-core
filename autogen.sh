#!/bin/sh

echo "Finding Autotools versions ..."
version=`libtool --version 2>&1`
if test "$?" != 0 ; then
  version=`glibtool --version 2>&1`
  if test "$?" != 0 ; then 
    echo "Could not find libtool version number.  Aborting."
    exit 1
  else
    libtool=glibtool
  fi
else
  libtool=libtool
fi
version=`echo $version | cut -f2 -d')'`
version=`echo $version | cut -f1 -d' ' | sed -e 's/[A-Za-z]//g'`
major_version=`echo $version | cut -f1 -d.`


# do component magic
echo "Finding components ..."
component_list=
component_m4_list=
for component_dir in components/* ; do
  if test -d "$component_dir" ; then
    component=`basename "$component_dir"`
    if test -f "$component_dir/.ignore" -a ! -f "$component_dir/.unignore" ; then
      echo " - ignoring component $component"
    elif test -f "$component_dir/.ignore" && \
         test -s $component_dir/.unignore && \
         test -z "`grep $USER $component_dir/.unignore`" ; then
      echo " - ignoring component $component"
    else
      if test -z "$component_list" ; then
        component_list="$component"
      else
        component_list="$component_list, $component"
      fi
      if test -f "$component_dir/configure.m4" ; then
        component_m4_list="$component_m4_list $component_dir/configure.m4"
      fi
    fi
  fi
done

echo "Finding introspectors ..."
introspector_list=
introspector_m4_list=
for introspector_dir in introspectors/* ; do
  if test -d "$introspector_dir" ; then
    introspector=`basename "$introspector_dir"`
    if test -f "$introspector_dir/.ignore" -a ! -f "$introspector_dir/.unignore" ; then
      echo " - ignoring introspector $introspector"
    elif test -f "$introspector_dir/.ignore" && \
         test -s $introspector_dir/.unignore && \
         test -z "`grep $USER $introspector_dir/.unignore`" ; then
      echo " - ignoring introspector $introspector"
    else
      if test -z "$introspector_list" ; then
        introspector_list="$introspector"
      else
        introspector_list="$introspector_list, $introspector"
      fi
      if test -f "$introspector_dir/configure.m4" ; then
        introspector_m4_list="$introspector_m4_list $introspector_dir/configure.m4"
      fi
    fi
  fi
done


cat > "config/sst_m4_config_include.m4" <<EOF
dnl -*- Autoconf -*-

dnl This file is automatically created by autogen.sh; it should not
dnl be edited by hand!!

EOF

echo "m4_define([sst_component_list], [$component_list])" >> config/sst_m4_config_include.m4

for component in $component_m4_list ; do
  echo "m4_include($component)" >> config/sst_m4_config_include.m4
done

echo "m4_define([sst_introspector_list], [$introspector_list])" >> config/sst_m4_config_include.m4
for introspector in $introspector_m4_list ; do
  echo "m4_include($introspector)" >> config/sst_m4_config_include.m4
done


echo "Generating configure files ..."
if test $major_version -lt 2 ; then
  echo " - Using Libtool pre-2.0"

  rm -rf libltdl sst/libltdl
  ${libtool}ize --automake --copy --ltdl
  if test -d libltdl; then
    echo " - Moving libltdl to sst/"
    mv libltdl sst
  fi
  if test ! -d sst/libltdl ; then
    echo "libltdl doesn't exist.  Aborting."
    exit 1
  fi
  aclocal -I config
  autoheader
  autoconf
  automake --foreign --add-missing --include-deps

else
  echo " - Using Libtool 2.0 or higher"
  autoreconf --install --symlink --warnings=gnu,obsolete,override,portability,no-obsolete
  if test $? != 0 ; then
    echo "It appears that configure file generation failed.  Sorry :(."
    exit 1
  fi
fi
