# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(top_srcdir)/elements/include

compdir = $(pkglibdir)
comp_LTLIBRARIES = libSS_router.la
noinst_HEADERS = \
				 SS_router.h \
				 SS_network.h \
				 SS_router-inline.h
libSS_router_la_SOURCES = libAlloc.cc \
						  SS_network.cpp \
						  SS_router.cpp

EXTRA_DIST = SS_routerInternals.cpp SS_routerEvent.cpp

libSS_router_la_LDFLAGS = -module -avoid-version
