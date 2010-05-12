# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(top_srcdir)/components/include

compdir = $(pkglibdir)
comp_LTLIBRARIES = libtrig_cpu.la
libtrig_cpu_la_SOURCES = trig_cpu.cc trig_cpu.h ../trig_nic/trig_nic_event.h

libtrig_cpu_la_LDFLAGS = -module -avoid-version

bin_PROGRAMS = trig_cpu_sdlgen
trig_cpu_sdlgen_SOURCES = trig_cpu_sdlgen.c
