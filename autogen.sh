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

# do elemlib magic
echo "Finding element libraries ..."
ignored_list=
added_make_list=
added_m4_list=
elemlib_list=
elemlib_m4_list=
for elemlib_dir in sst/elements/* ; do
  if test -d "$elemlib_dir" -a -r "$elemlib_dir/Makefile.am" ; then
    elemlib=`basename "$elemlib_dir"`
    if test -f "$elemlib_dir/.ignore" -a ! -f "$elemlib_dir/.unignore" ; then
#      echo " !- ignoring element library $elemlib"
      ignored_list="$ignored_list $elemlib"
    elif test -f "$elemlib_dir/.ignore" && \
         test -s $elemlib_dir/.unignore && \
         test -z "`grep $USER $elemlib_dir/.unignore`" ; then
#      echo " !- ignoring element library $elemlib"
      ignored_list="$ignored_list $elemlib"
    else
      if test -z "$elemlib_list" ; then
        elemlib_list="$elemlib"
      else
        elemlib_list="$elemlib_list, $elemlib"
      fi
      if test -f "$elemlib_dir/configure.m4" ; then
#        echo "  - including: $elemlib_dir/configure.m4"
        added_m4_list="$added_m4_list $elemlib/config.m4"
        elemlib_m4_list="$elemlib_m4_list $elemlib_dir/configure.m4"
      else 
        added_make_list="$added_make_list $elemlib"
#        echo "  - including element library: $elemlib_dir"
      fi
    fi
  fi
done

# Output our findings
echo
echo "Included Element Libraries in sst/elements/"
echo "========================================================="
for i in $added_make_list ; do echo "    $i" ; done
echo
echo "Included config.m4 files in sst/elements/"
echo "========================================================="
for i in $added_m4_list ; do echo "    $i" ; done
echo
echo "Ignored Element Libraries in sst/elements/"
echo "========================================================="
for i in $ignored_list ; do echo "    $i" ; done
echo
echo "========================================================="
echo "========================================================="

# Build the configuration file
cat > "config/sst_m4_config_include.m4" <<EOF
dnl -*- Autoconf -*-

dnl This file is automatically created by autogen.sh; it should not
dnl be edited by hand!!

EOF

echo "m4_define([sst_elemlib_list], [$elemlib_list])" >> config/sst_m4_config_include.m4

for elemlib in $elemlib_m4_list ; do
  echo "m4_include($elemlib)" >> config/sst_m4_config_include.m4
done

OPSYS=`uname`
OSSYSVER=`uname -r`

echo "Generating configure files..."
if test "$OPSYS" = "Darwin" -a "$OSSYSVER" = "12.3.0"; then
  echo " - Using Libtool generation for Mountain Lion"

  rm -rf libltdl sst/core/libltdl
  ${libtool}ize --automake --copy --ltdl
  if test -d libltdl; then
    echo " - Moving libltdl to sst/core/"
    mv libltdl sst/core
  fi
  if test ! -d sst/core/libltdl ; then
    echo "libltdl doesn't exist.  Aborting."
    exit 1
  fi
  aclocal -I config
  autoheader
  autoconf
  automake --foreign --add-missing --include-deps

elif test $major_version -lt 2 ; then
  echo " - Using Libtool pre-2.0"

  rm -rf libltdl sst/core/libltdl
  ${libtool}ize --automake --copy --ltdl
  if test -d libltdl; then
    echo " - Moving libltdl to sst/core/"
    mv libltdl sst/core
  fi
  if test ! -d sst/core/libltdl ; then
    echo "libltdl doesn't exist.  Aborting."
    exit 1
  fi
  aclocal -I config
  autoheader
  autoconf
  automake --foreign --add-missing --include-deps

else
  echo " - Using Libtool 2.0 or higher"
  autoreconf --install --force --symlink --warnings=gnu,obsolete,override,portability,no-obsolete
  if test $? != 0 ; then
    echo "It appears that configure file generation failed.  Sorry :(."
    exit 1
  fi
fi
