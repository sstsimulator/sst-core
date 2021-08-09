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

echo "Running ${LIBTOOLIZE}..."
$LIBTOOLIZE --automake --copy

aclocal -I config
autoheader
autoconf
automake --foreign --add-missing --include-deps
autoreconf --force --install

