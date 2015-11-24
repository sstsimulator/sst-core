#!/bin/bash

LIBTOOL=libtool
LIBTOOLIZE=libtoolize

# Delete the old libtool output
rm -rf libltdl src/sst/core/libltdl

$LIBTOOLIZE --automake --copy --ltdl

if test -d libltdl; then
	echo "Moving libltdl to src .."
	mv libltdl ./src/sst/core
fi

if test ! -d src/sst/core/libltdl ; then
    echo "libltdl doesn't exist.  Aborting."
    exit 1
fi

aclocal -I config
autoheader
autoconf
automake --foreign --add-missing --include-deps

