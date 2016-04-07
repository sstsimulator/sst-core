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
	componentInfo.h \
	config.h \
	configGraph.h \
	configGraphOutput.h \
	cfgoutput/pythonConfigOutput.h \
	cfgoutput/dotConfigOutput.h \
	cfgoutput/xmlConfigOutput.h \
	cfgoutput/jsonConfigOutput.h \
	element.h \
	elemLoader.h \
	event.h \
	exit.h \
	factory.h \
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
	iouse.h \
	module.h \
	objectComms.h \
	oneshot.h \
	output.h \
	params.h \
	pollingLinkQueue.h \
	profile.h \
	rankInfo.h \
	rankSync.h \
	rankSyncParallelSkip.h \
	rankSyncSerialSkip.h \
	simulation.h \
	serialization.h \
	sparseVectorMap.h \
	sst_types.h \
	sstinfo.h \
	stopAction.h \
	stringize.h \
	subcomponent.h \
	sync.h \
	syncBase.h \
	syncManager.h \
	syncQueue.h \
	threadSync.h \
	threadSyncSimpleSkip.h \
	threadSyncQueue.h \
	sharedRegion.h \
	sharedRegionImpl.h \
	timeConverter.h \
	timeLord.h \
	timeVortex.h \
	math/sqrt.h \
	uninitializedQueue.h \
	unitAlgebra.h \
	serialization/core.h \
	serialization/types.h \
	serialization/element.h \
	serialization/ser_ptr_type.h \
	serialization/serializable.h \
	serialization/serializable_fwd.h \
	serialization/serialize.h \
	serialization/serialize_array.h \
	serialization/serialize_buffer_accessor.h \
	serialization/serialize_list.h \
	serialization/serialize_map.h \
	serialization/serialize_packer.h \
	serialization/serialize_serializable.h \
	serialization/serialize_set.h \
	serialization/serialize_sizer.h \
	serialization/serialize_string.h \
	serialization/serialize_unpacker.h \
	serialization/serialize_vector.h \
	serialization/serializer.h \
	serialization/serializer_fwd.h \
	serialization/statics.h \
	interfaces/TestEvent.h \
	interfaces/stringEvent.h \
	interfaces/simpleMem.h \
	interfaces/simpleNetwork.h \
	interprocess/circularBuffer.h \
	interprocess/ipctunnel.h \
	rng/sstrng.h \
	rng/marsaglia.h \
	rng/poisson.h \
	rng/mersenne.h \
	rng/xorshift.h \
	rng/distrib.h \
	rng/discrete.h \
	rng/gaussian.h \
	rng/expon.h \
	rng/constant.h \
	rng/uniform.h \
	stats/basestats.h \
	stats/statfuncs.h \
	stats/accumulator.h \
	stats/histo/histo.h \
	statapi/statengine.h \
	statapi/statoutput.h \
	statapi/statfieldinfo.h \
	statapi/statoutputconsole.h \
	statapi/statuniquecount.h \
	statapi/statoutputtxt.h \
	statapi/statoutputtxtgz.h \
	statapi/statoutputcsv.h \
	statapi/statoutputcsvgz.h \
	statapi/statbase.h \
	statapi/stathistogram.h \
	statapi/stataccumulator.h \
	statapi/statnull.h \
	statapi/componentregisterstat_impl.h \
	threadsafe.h \
	cputimer.h \
	sst_config.h

nobase_nodist_sst_HEADERS = \
	build_info.h

sst_core_sources = \
	sstconfigreader.cc \
	action.cc \
	archive.cc \
	clock.cc \
	component.cc \
	componentInfo.cc \
	config.cc \
	configGraph.cc \
	cfgoutput/pythonConfigOutput.cc \
	cfgoutput/dotConfigOutput.cc \
	cfgoutput/xmlConfigOutput.cc \
	cfgoutput/jsonConfigOutput.cc \
	boostExports.cc \
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
	oneshot.cc \
	output.cc \
	params.cc \
	pollingLinkQueue.cc \
	rankInfo.cc \
	rankSync.cc \
	rankSyncParallelSkip.cc \
	rankSyncSerialSkip.cc \
	simulation.cc \
	subcomponent.cc \
	sync.cc \
	syncBase.cc \
	syncManager.cc \
	syncQueue.cc \
	threadSync.cc \
	threadSyncSimpleSkip.cc \
	sharedRegion.cc \
	timeLord.cc \
	timeVortex.cc \
	uninitializedQueue.cc \
	unitAlgebra.cc \
	serialization/serializable.cc \
	serialization/serialize_serializable.cc \
	serialization/serializer.cc \
	serialization/statics.cc \
	interfaces/stringEvent.cc \
	interfaces/TestEvent.cc \
	interfaces/simpleMem.cc \
	interfaces/simpleNetwork.cc \
	interfaces/simpleNetworkExports.cc \
	rng/sstrng.cc \
	rng/marsaglia.cc \
	rng/mersenne.cc \
	rng/xorshift.cc \
	statapi/statengine.cc \
	statapi/statoutput.cc \
	statapi/statfieldinfo.cc \
	statapi/statoutputconsole.cc \
	statapi/statoutputtxt.cc \
	statapi/statoutputtxtgz.cc \
	statapi/statoutputcsv.cc \
	statapi/statoutputcsvgz.cc \
	statapi/statbase.cc \
	cputimer.cc \
	iouse.cc

bin_PROGRAMS = sst sst-info sst-config sst-register
libexec_PROGRAMS = sstsim.x sstinfo.x

sst_info_SOURCES = \
	bootsstinfo.cc \
	bootshared.cc \
	bootshared.h \
	sstconfigreader.cc \
	sstconfigreader.h

sst_SOURCES = \
	bootsst.cc \
	bootshared.cc \
	bootshared.h \
	sstconfigreader.cc \
	sstconfigreader.h

sst_config_SOURCES = \
	sstconfigtool.cc \
	sstconfigreader.cc \
	sstconfigreader.h

sst_register_SOURCES = \
	sstregistertool.cc \
	sstconfigreader.cc \
	sstconfigreader.h

sstsim_x_SOURCES = \
	main.cc \
	$(sst_core_sources)

sstinfo_x_SOURCES = \
	sstinfo.cc \
	$(sst_core_sources) \
	$(sst_xml_sources)

sstsim_x_LDADD = \
	$(LIBLTDL) \
	$(ZOLTAN_LIB) \
	$(PARMETIS_LIB) \
	$(BOOST_LIBS) \
	$(MPILIBS) \
	$(TCMALLOC_LIB) \
	-lm

sstsim_x_LDFLAGS = \
	$(PARMETIS_LDFLAGS) \
	$(ZOLTAN_LDFLAGS) \
	$(TCMALLOC_LDFLAGS) \
	$(BOOST_LDFLAGS) \
        $(PYTHON_LDFLAGS) \
	-export-dynamic \
	$(SST_LTLIBS_ELEMLIBS)

sstinfo_x_LDADD = \
	$(LIBLTDL) \
	$(ZOLTAN_LIB) \
	$(PARMETIS_LIB) \
	$(BOOST_LIBS) \
	$(MPILIBS) \
	$(TCMALLOC_LIB) \
	-lm

sstinfo_x_LDFLAGS = \
	$(PARMETIS_LDFLAGS) \
	$(ZOLTAN_LDFLAGS) \
	$(TCMALLOC_LDFLAGS) \
	$(BOOST_LDFLAGS) \
        $(PYTHON_LDFLAGS) \
	-export-dynamic \
	$(SST_LTLIBS_ELEMLIBS)

include tinyxml/Makefile.inc
include part/Makefile.inc
include model/Makefile.inc

if USE_LIBZ
sstsim_x_LDADD += $(LIBZ_LIBS)
sstinfo_x_LDADD += $(LIBZ_LIBS)
endif
