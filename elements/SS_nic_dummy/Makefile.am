# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(top_srcdir)/elements/include

compdir = $(pkglibdir)
comp_LTLIBRARIES = libSS_nic_dummy.la
libSS_nic_dummy_la_SOURCES = libAlloc.cc dummy.cc  RtrIF.h

libSS_nic_dummy_la_LDFLAGS = -module -avoid-version
