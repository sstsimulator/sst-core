# -*- Makefile -*-
#
#

ACLOCAL_AMFLAGS = -I config

EXTRA_DIST = autogen.sh

SUBDIRS = \
	sst \
	doc \
	user_includes \
	user_libs \
	user_tests 

dist-hook:
	@ if test $(SST_DISTABLE_LIBTOOL) -eq 0 ; then \
	  echo "*** WARNING: Your version of Libtool is too old for 'make dist'" ; \
	  echo "*** Libtool version 2.2.0 or higher is required." ; \
	  echo "*** Your libtool reports:" ; \
	  ./libtool --version ; \
	  echo "*** Can not continue!" ; \
	  exit 1 ; \
	fi
