# -*- Makefile -*-
#
#


compdir = $(pkglibdir)
comp_LTLIBRARIES = libcpu_PowerAndData.la
libcpu_PowerAndData_la_SOURCES = \
	cpu_PowerAndData.h \
	cpu_PowerAndData.cc \
	../power/power.cc \
	../power/power.h


libcpu_PowerAndData_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(SIMPANALYZER_CPPFLAGS) $(MCPAT_CPPFLAGS)
libcpu_PowerAndData_la_LDFLAGS = -module -avoid-version $(SIMPANALYZER_LDFLAGS) $(MCPAT_LDFLAGS)
libcpu_PowerAndData_la_LIBADD = $(SIMPANALYZER_LIB) $(MCPAT_LIB)

