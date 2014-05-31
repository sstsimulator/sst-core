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
	debug.h \
	element.h \
	elemLoader.h \
	event.h \
	exit.h \
	factory.h \
	graph.h \
        heartbeat.h \
	initQueue.h \
	introspectAction.h \
	introspector.h \
	introspectedComponent.h \
	link.h \
	linkMap.h \
	linkPair.h \
	mempool.h \
        memuse.h \
	module.h \
	output.h \
	params.h \
	pollingLinkQueue.h \
	simulation.h \
	serialization.h \
	sst_types.h \
	sstinfo.h \
	stopAction.h \
	sync.h \
	syncQueue.h \
	timeConverter.h \
	timeLord.h \
	timeVortex.h \
	math/sqrt.h \
	uninitializedQueue.h \
	unitAlgebra.h \
	serialization/core.h \
	serialization/types.h \
	serialization/element.h \
	interfaces/TestEvent.h \
	interfaces/stringEvent.h \
	interfaces/simpleMem.h \
	rng/sstrand.h \
	rng/marsaglia.h \
	rng/mersenne.h \
	rng/distrib.h \
	rng/gaussian.h \
	rng/expon.h \
	rng/constant.h \
	stats/basestats.h \
	stats/statfuncs.h \
	stats/accumulator.h \
	stats/histo/histo.h \
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
	debug.cc \
	elemLoader.cc \
	event.cc \
	exit.cc \
	export.cc \
	factory.cc \
        heartbeat.cc \
	initQueue.cc \
	introspectAction.cc \
	introspector.cc \
	introspectedComponent.cc \
	link.cc \
        memuse.cc \
	output.cc \
	params.cc \
	pollingLinkQueue.cc \
	simulation.cc \
	sync.cc \
	syncQueue.cc \
	timeLord.cc \
	timeVortex.cc \
	uninitializedQueue.cc \
	unitAlgebra.cc \
	interfaces/stringEvent.cc \
	interfaces/TestEvent.cc \
	interfaces/simpleMem.cc \
	rng/sstrand.cc \
	rng/marsaglia.cc \
	rng/mersenne.cc \
	rng/distrib.cc \
	rng/gaussian.cc \
	rng/expon.cc \
	rng/constant.cc \
	cputimer.cc

bin_PROGRAMS = sst sstinfo
libexec_PROGRAMS = sstsim.x sstinfo.x

sstinfo_SOURCES = \
	bootsstinfo.cc \
	bootshared.cc \
	bootshared.h

sst_SOURCES = \
	bootsst.cc \
	bootshared.cc \
	bootshared.h

sstsim_x_SOURCES = \
	main.cc \
	$(sst_core_sources)

sstinfo_x_SOURCES = \
	sstinfo.cc \
	$(sst_core_sources)

sstsim_x_LDADD = \
	$(LIBLTDL) \
	$(BOOST_SERIALIZATION_LIB) \
	$(BOOST_MPI_LIB) \
	$(BOOST_PROGRAM_OPTIONS_LIB) \
	$(ZOLTAN_LIB) \
	$(PARMETIS_LIB) \
	$(MPILIBS) \
	$(TCMALLOC_LIB) \
	-lm

sstsim_x_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	$(PARMETIS_LDFLAGS) \
	$(ZOLTAN_LDFLAGS) \
	$(BOOST_MPI_LIB) \
	$(TCMALLOC_LDFLAGS) \
        $(PYTHON_LDFLAGS) \
	-export-dynamic \
	$(SST_LTLIBS_ELEMLIBS)

sstinfo_x_LDADD = \
	$(LIBLTDL) \
	$(BOOST_SERIALIZATION_LIB) \
	$(BOOST_MPI_LIB) \
	$(BOOST_PROGRAM_OPTIONS_LIB) \
	$(ZOLTAN_LIB) \
	$(PARMETIS_LIB) \
	$(MPILIBS) \
	$(TCMALLOC_LIB) \
	-lm
	
sstinfo_x_LDFLAGS = \
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
