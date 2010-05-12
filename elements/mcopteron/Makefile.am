# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(top_srcdir)/elements/include

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmcopteron.la
libmcopteron_la_SOURCES = cpu.h cpu.cc FunctionalUnit.cc InstructionQueue.cc McOpteron.cc \
          Token.cc InstructionInfo.cc Random.cc LoadStoreUnit.cc MemoryModel.cc \
          ReorderBuffer.cc ConfigVars.cc mersenne.h FunctionalUnit.h InstructionQueue.h \
          McOpteron.h Token.h InstructionInfo.h LoadStoreUnit.h MemoryModel.h \
	OffCpuIF.h OpteronDefs.h ReorderBuffer.h  ConfigVars.h

libmcopteron_la_LDFLAGS = -module -avoid-version
