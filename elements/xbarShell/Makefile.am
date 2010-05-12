# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(top_srcdir)/components/xbar

compdir = $(pkglibdir)
comp_LTLIBRARIES = libxbarShell.la
libxbarShell_la_SOURCES = dummy.h dummy.cc xbarShell.h xbarShell.cc
libxbarShell_la_LDFLAGS = -module -avoid-version
