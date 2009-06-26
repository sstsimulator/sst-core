#!/bin/sh

echo "Generating configure files..."

version=`libtool --version 2>&1`
if test "$?" != 0 ; then
  echo "Could not find libtool version number.  Aborting."
  exit 1
fi
version=`echo $version | cut -f2 -d')'`
version=`echo $version | cut -f1 -d' ' | sed -e 's/[A-Za-z]//g'`
major_version=`echo $version | cut -f1 -d.`

if test $major_version -lt 2 ; then
  echo "- Using Libtool pre-2.0"

  rm -rf libltdl sst/libltdl
  libtoolize --automake --copy --ltdl
  if test -d libltdl; then
    echo "- Moving libltdl to sst/"
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
  echo "- Using Libtool 2.0 or higher"
  autoreconf --install --symlink --warnings=gnu,obsolete,override,portability,no-obsolete
  if test $? != 0 ; then
    echo "It appears that configure file generation failed.  Sorry :(."
    exit 1
  fi
fi
