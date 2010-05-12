# -*- Makefile -*-
#
#


compdir = $(pkglibdir)
comp_LTLIBRARIES = libcpu_power.la
libcpu_power_la_SOURCES = \
	cpu_power.h \
	cpu_power.cc \
	../power/power.cc \
	../power/power.h


libcpu_power_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(SIMPANALYZER_CPPFLAGS) $(MCPAT_CPPFLAGS)
libcpu_power_la_LDFLAGS = -module -avoid-version $(SIMPANALYZER_LDFLAGS) $(MCPAT_LDFLAGS)
libcpu_power_la_LIBADD = $(SIMPANALYZER_LIB) $(MCPAT_LIB)

