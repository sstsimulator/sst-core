# -*- Makefile -*-
#
#

EXTRA_DIST = mase_gcc.trc

compdir = $(pkglibdir)
comp_LTLIBRARIES = libDRAMSimTrace.la
libDRAMSimTrace_la_SOURCES = \
	DRAMSimTrace.cpp \
	DRAMSimTrace.h
libDRAMSimTrace_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(DRAMSIM_CPPFLAGS) \
			-I$(top_srcdir)/components/include
libDRAMSimTrace_la_LDFLAGS = -module -avoid-version $(DRAMSIM_LDFLAGS)
libDRAMSimTrace_la_LIBADD = $(DRAMSIM_LIB)
