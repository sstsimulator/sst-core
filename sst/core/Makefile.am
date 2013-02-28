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
SUBDIRS = libltdl .
EXTRA_DIST =

sstdir = $(includedir)/sst/core
nobase_dist_sst_HEADERS = \
	activityQueue.h \
	action.h \
	activity.h \
	archive.h \
	clock.h \
	component.h \
	config.h \
	configGraph.h \
	cpunicEvent.h \
	debug.h \
	element.h \
	event.h \
	exit.h \
	factory.h \
	graph.h \
	initQueue.h \
	introspectAction.h \
	introspector.h \
	introspectedComponent.h \
	link.h \
	linkMap.h \
	linkPair.h \
	log.h \
	memEvent.h \
	params.h \
	pollingLinkQueue.h \
	sdl.h \
	simulation.h \
	sst_types.h \
	stopAction.h \
	sync.h \
	syncQueue.h \
	timeConverter.h \
	timeLord.h \
	timeVortex.h \
	uninitializedQueue.h \
	zolt.h \
	serialization/archive.h \
	serialization/core.h \
	serialization/types.h \
	serialization/element.h \
	interfaces/TestEvent.h \
	interfaces/memEvent.h \
	rng/sstrand.h

nobase_nodist_sst_HEADERS = \
	build_info.h

bin_PROGRAMS = sst.x
sst_x_SOURCES = \
	action.cc \
	archive.cc \
	clock.cc \
	component.cc \
	config.cc \
	configGraph.cc \
	cpunicEvent.cc \
	debug.cc \
	event.cc \
	exit.cc \
	export.cc \
	factory.cc \
	initQueue.cc \
	introspectAction.cc \
	introspector.cc \
	introspectedComponent.cc \
	link.cc \
	main.cc \
	memEvent.cc \
	pollingLinkQueue.cc \
	sdl.cc \
	simulation.cc \
	sync.cc \
	syncQueue.cc \
	timeLord.cc \
	timeVortex.cc \
	uninitializedQueue.cc \
	interfaces/memEvent.cc \
	interfaces/TestEvent.cc \
	rng/sstrand.cc

if HAVE_ZOLTAN
sst_x_SOURCES += zolt.cc
endif

sst_x_LDADD = \
	$(LIBLTDL) \
	$(BOOST_SERIALIZATION_LIB) \
	$(BOOST_MPI_LIB) \
	$(BOOST_PROGRAM_OPTIONS_LIB) \
	$(ZOLTAN_LIB) \
	$(PARMETIS_LIB) \
	$(MPILIBS) \
	-lm
sst_x_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	$(PARMETIS_LDFLAGS) \
	$(ZOLTAN_LDFLAGS) \
	$(BOOST_MPI_LIB) \
	-export-dynamic \
	$(SST_LTLIBS_ELEMLIBS)

include tinyxml/Makefile.inc
include part/Makefile.inc

