# -*- Makefile -*-
#
#

CC = $(MPICC)
CXX = $(MPICXX)

AM_CPPFLAGS =  \
	-DTIXML_USE_STL \
	$(BOOST_CPPFLAGS) \
	$(ZOLTAN_CPPFLAGS) \
	$(LTDLINCL)

DIST_SUBDIRS = libltdl .
SUBDIRS = $(LIBLTDL_DIR) .
EXTRA_DIST =

sstdir = $(includedir)/sst/core
nobase_dist_sst_HEADERS = \
	activityQueue.h \
	action.h \
	activity.h \
	archive.h \
	boost.h \
	clockEvent.h \
	clockHandler.h \
	compEvent.h \
	component.h \
	config.h \
	configGraph.h \
	cpunicEvent.h \
	debug.h \
	element.h \
	event.h \
	eventFunctor.h \
	eventHandler1Arg.h \
	eventQueue.h \
	exit.h \
	factory.h \
	graph.h \
	introspector.h \
	link.h \
	linkMap.h \
	linkPair.h \
	log.h \
	memEvent.h \
	pollingLinkQueue.h \
	sdl.h \
	simulation.h \
	sst.h \
	stopEvent.h \
	sync.h \
	syncQueue.h \
	timeConverter.h \
	timeLord.h \
	timeVortex.h \
	zolt.h \
	serialization/archive.h \
	serialization/core.h \
	serialization/types.h \
	serialization/element.h
nobase_nodist_sst_HEADERS = \
	build_info.h

bin_PROGRAMS = sst.x
sst_x_SOURCES = \
	action.cc \
	archive.cc \
	clockEvent.cc \
	component.cc \
	config.cc \
	configGraph.cc \
	cpunicEvent.cc \
	debug.cc \
	event.cc \
	exit.cc \
	export.cc \
	factory.cc \
	introspector.cc \
	link.cc \
	main.cc \
	memEvent.cc \
	pollingLinkQueue.cc \
	simulation.cc \
	sync.cc \
	syncQueue.cc \
	timeLord.cc \
	timeVortex.cc \
	xml.cc \
	zolt.cc
sst_x_LDADD = \
	$(LIBLTDL) \
	$(BOOST_SERIALIZATION_LIB) \
	$(BOOST_MPI_LIB) \
	$(BOOST_PROGRAM_OPTIONS_LIB) \
	$(ZOLTAN_LIB) \
	$(PARMETIS_LIB) \
	$(MPILIBS)
sst_x_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	$(PARMETIS_LDFLAGS) \
	$(ZOLTAN_LDFLAGS) \
	-export-dynamic \
	$(SST_LTLIBS_COMPONENTS) \
	$(SST_LTLIBS_INTROSPECTORS)

include tinyxml/Makefile.inc
