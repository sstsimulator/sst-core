# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(top_srcdir)/elements/include

compdir = $(pkglibdir)
comp_LTLIBRARIES = libtrig_nic.la
#libtrig_nic_la_SOURCES = libAlloc.cc trig_nic.cc RtrIF.h
libtrig_nic_la_SOURCES = libAlloc.cc trig_nic.cc trig_nic.h ../SS_router/RtrIF.h

libtrig_nic_la_LDFLAGS = -module -avoid-version
