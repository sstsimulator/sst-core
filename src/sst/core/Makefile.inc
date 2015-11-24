# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	-I$(top_srcdir)/sst/tinyxml

sst_x_SOURCES += \
	src/clockEvent.cc \
	src/component.cc \
	src/config.cc \
	src/configGraph.cc \
	src/cpunicEvent.cc \
	src/debug.cc \
	src/event.cc \
	src/exit.cc \
	src/export.cc \
	src/factory.cc \
	src/link.cc \
	src/linkMap.cc \
	src/memEvent.cc \
	src/simulation.cc \
	src/sync.cc \
	src/timeLord.cc \
	src/introspector.cc \
	src/xml.cc \
	src/zolt.cc \
	src/main.cc
