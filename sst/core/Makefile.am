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
	$(PYTHON_CPPFLAGS) \
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
	simulation.h \
	sst_types.h \
	sstinfo.h \
	stopAction.h \
	sync.h \
	syncQueue.h \
	timeConverter.h \
	timeLord.h \
	timeVortex.h \
	uninitializedQueue.h \
	serialization/core.h \
	serialization/types.h \
	serialization/element.h \
	serialization.h \
	interfaces/TestEvent.h \
	interfaces/memEvent.h \
	interfaces/stringEvent.h \
	rng/sstrand.h \
	rng/marsaglia.h \
	rng/mersenne.h \
	rng/distrib.h \
	rng/gaussian.h \
	rng/expon.h \
	cputimer.h

nobase_nodist_sst_HEADERS = \
	build_info.h

sst_core_sources = \
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
	simulation.cc \
	sync.cc \
	syncQueue.cc \
	timeLord.cc \
	timeVortex.cc \
	uninitializedQueue.cc \
	interfaces/memEvent.cc \
	interfaces/stringEvent.cc \
	interfaces/TestEvent.cc \
	rng/sstrand.cc \
	rng/marsaglia.cc \
	rng/mersenne.cc \
	rng/distrib.cc \
	rng/gaussian.cc \
	rng/expon.cc \
	cputimer.cc

bin_PROGRAMS = sst.x sstinfo
sst_x_SOURCES = \
	main.cc \
	$(sst_core_sources)

sstinfo_SOURCES = \
	sstinfo.cc \
	$(sst_core_sources)

sst_x_LDADD = \
	$(LIBLTDL) \
	$(BOOST_SERIALIZATION_LIB) \
	$(BOOST_MPI_LIB) \
	$(BOOST_PROGRAM_OPTIONS_LIB) \
	$(ZOLTAN_LIB) \
	$(PARMETIS_LIB) \
	$(MPILIBS) \
	$(TCMALLOC_LIB) \
	-lm

sst_x_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	$(PARMETIS_LDFLAGS) \
	$(ZOLTAN_LDFLAGS) \
	$(BOOST_MPI_LIB) \
	$(TCMALLOC_LDFLAGS) \
    $(PYTHON_LDFLAGS) \
	-export-dynamic \
	$(SST_LTLIBS_ELEMLIBS)

sstinfo_LDADD = \
	$(LIBLTDL) \
	$(BOOST_SERIALIZATION_LIB) \
	$(BOOST_MPI_LIB) \
	$(BOOST_PROGRAM_OPTIONS_LIB) \
	$(ZOLTAN_LIB) \
	$(PARMETIS_LIB) \
	$(MPILIBS) \
	$(TCMALLOC_LIB) \
	-lm
	
sstinfo_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	$(PARMETIS_LDFLAGS) \
	$(ZOLTAN_LDFLAGS) \
	$(BOOST_MPI_LIB) \
	$(TCMALLOC_LDFLAGS) \
    $(PYTHON_LDFLAGS) \
	-export-dynamic \
	$(SST_LTLIBS_ELEMLIBS)

include tinyxml/Makefile.inc
include part/Makefile.inc
include model/Makefile.inc
