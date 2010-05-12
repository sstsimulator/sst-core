# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(top_srcdir)/elements/include

compdir = $(pkglibdir)
comp_LTLIBRARIES = libbus.la
libbus_la_SOURCES = bus.h bus.cc libAlloc.cc
libbus_la_LDFLAGS = -module -avoid-version
