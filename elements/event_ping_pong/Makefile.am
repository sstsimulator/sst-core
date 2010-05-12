# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libevent_ping_pong.la
libevent_ping_pong_la_SOURCES = cycle.h event_ping_pong.h event_ping_pong.cc
libevent_ping_pong_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = ping_pong.xml
