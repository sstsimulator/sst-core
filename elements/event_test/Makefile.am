# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(top_srcdir)/components/include

compdir = $(pkglibdir)
comp_LTLIBRARIES = libevent_test.la

libevent_test_la_SOURCES = \
	event_test.cc \
	event_test.h

libevent_test_la_LDFLAGS = -module -avoid-version
