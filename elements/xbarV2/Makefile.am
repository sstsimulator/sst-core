# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(top_srcdir)/elements/include

compdir = $(pkglibdir)
comp_LTLIBRARIES = libxbarV2.la
libxbarV2_la_SOURCES = xbarV2.h xbarV2.cc libAlloc.cc
libxbarV2_la_LDFLAGS = -module -avoid-version
