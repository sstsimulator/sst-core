#!/bin/bash

if [ -z $LIBTOOLIZE ] ; then
    LIBTOOLIZE=$(type -P libtoolize)
    if [ -z $LIBTOOLIZE ] ; then
        LIBTOOLIZE=$(type -P glibtoolize)
    fi
fi
if [ -z $LIBTOOL ] ; then
    LIBTOOL=$(type -P "${LIBTOOLIZE%???}")
fi

if [ -z $LIBTOOL ] || [ -z $LIBTOOLIZE ] ; then
    echo "Unable to find working libtool. [$LIBTOOL][$LIBTOOLIZE]"
    exit 1
fi


# Delete the old libtool output
rm -rf libltdl src/sst/core/libltdl

echo "Running ${LIBTOOLIZE}..."
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
autoreconf --force --install

