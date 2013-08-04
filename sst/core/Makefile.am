# -*- Makefile -*-
#
#

CC = $(MPICC)
CXX = $(MPICXX)

AM_CPPFLAGS =  \
	-DTIXML_USE_STL \
	-DSST_BUILDING_CORE=1 \
	$(BOOST_CPPFLAGS) \
	$(ZOLTAN_CPPFLAGS) \
	$(SST_CXX0X_FLAGS) \
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
	module.h \
	output.h \
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
	serialization/core.h \
	serialization/types.h \
	serialization/element.h \
	serialization.h \
	interfaces/TestEvent.h \
	interfaces/memEvent.h \
	interfaces/stringEvent.h \
	interfaces/registerComponentOnNetworkEvent.h \
	rng/sstrand.h \
	rng/marsaglia.h \
	rng/mersenne.h

nobase_nodist_sst_HEADERS = \
	build_info.h

noinst_LTLIBRARIES = libsst.la
libsst_la_SOURCES = \
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
	output.cc \
	pollingLinkQueue.cc \
	sdl.cc \
	simulation.cc \
	sync.cc \
	syncQueue.cc \
	timeLord.cc \
	timeVortex.cc \
	uninitializedQueue.cc \
	interfaces/memEvent.cc \
	interfaces/stringEvent.cc \
	interfaces/registerComponentOnNetworkEvent.cc \
	interfaces/TestEvent.cc \
	rng/sstrand.cc \
	rng/marsaglia.cc \
	rng/mersenne.cc
if HAVE_ZOLTAN
libsst_la_SOURCES += zolt.cc
endif
libsst_la_LIBADD = \
	$(LIBLTDL) \
	$(BOOST_SERIALIZATION_LIB) \
	$(BOOST_MPI_LIB) \
	$(BOOST_PROGRAM_OPTIONS_LIB) \
	$(ZOLTAN_LIB) \
	$(PARMETIS_LIB) \
	$(MPILIBS) \
	$(TCMALLOC_LIB) \
	-lm
libsst_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	$(PARMETIS_LDFLAGS) \
	$(ZOLTAN_LDFLAGS) \
	$(BOOST_MPI_LIB) \
	$(TCMALLOC_LDFLAGS)

# Note: SST_LTLIBS_ELEMLIBS is the list of 
bin_PROGRAMS = sst.x
sst_x_SOURCES = \
	main.cc
sst_x_LDFLAGS = \
	-export-dynamic
sst_x_LDADD = libsst.la \
	$(SST_LTLIBS_ELEMLIBS)

include tinyxml/Makefile.inc
include part/Makefile.inc

