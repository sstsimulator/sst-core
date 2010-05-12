# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(top_srcdir)/elements/include

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmcniagara.la
libmcniagara_la_SOURCES = cpu.h cpu.cc Dependency.cc McNiagara-mem.cc MemoryModel.cc \
          CycleTracker.cc McNiagara.cc McNiagara-sim.cc Dependency.h McNiagara.h \
          MemoryModel.h OffCpuIF.h CycleTracker.h McSimDefs.h mersenne.h 

libmcniagara_la_LDFLAGS = -module -avoid-version
