# -*- Makefile -*-
#
#

CC = $(MPICC)
CXX = $(MPICXX)

AM_CPPFLAGS =  \
	-DTIXML_USE_STL \
	-DSST_BUILDING_CORE=1 \
	$(PYTHON_CPPFLAGS) \
	-I$(top_srcdir)/external \
	$(LTDLINCL)

EXTRA_DIST = mainpage.dox eli/README.md

sstdir = $(includedir)/sst/core
nobase_dist_sst_HEADERS = \
	activityQueue.h \
	action.h \
	activity.h \
	clock.h \
	baseComponent.h \
	checkpointAction.h \
	component.h \
	componentExtension.h \
	componentInfo.h \
	config.h \
	configBase.h \
	configShared.h \
	configGraph.h \
	configGraphOutput.h \
	cfgoutput/pythonConfigOutput.h \
	cfgoutput/dotConfigOutput.h \
	cfgoutput/xmlConfigOutput.h \
	cfgoutput/jsonConfigOutput.h \
	decimal_fixedpoint.h \
	env/envquery.h \
	env/envconfig.h \
	elemLoader.h \
	event.h \
	exit.h \
	factory.h \
	from_string.h \
	heartbeat.h \
	initQueue.h \
	link.h \
	mempool.h \
	memuse.h \
	iouse.h \
	module.h \
	namecheck.h \
	oneshot.h \
	output.h \
	params.h \
	pollingLinkQueue.h \
	profile.h \
	profile/profiletool.h \
	profile/clockHandlerProfileTool.h \
	profile/eventHandlerProfileTool.h \
	profile/syncProfileTool.h \
	profile/componentProfileTool.h \
	rankInfo.h \
	sparseVectorMap.h \
	sst_types.h \
	sstpart.h \
	ssthandler.h \
	stopAction.h \
	stringize.h \
	subcomponent.h \
	timeConverter.h \
	timeLord.h \
	timeVortex.h \
	math/sqrt.h \
	uninitializedQueue.h \
	unitAlgebra.h \
	eli/elementinfo.h \
	eli/elibase.h \
	eli/attributeInfo.h \
	eli/categoryInfo.h \
	eli/defaultInfo.h \
	eli/elementbuilder.h \
	eli/interfaceInfo.h \
	eli/paramsInfo.h \
	eli/portsInfo.h \
	eli/profilePointInfo.h \
	eli/simpleInfo.h \
	eli/statsInfo.h \
	eli/subcompSlotInfo.h \
	serialization/serializable.h \
	serialization/serializable_base.h \
	serialization/serializable_fwd.h \
	serialization/serialize.h \
	serialization/serialize_impl_fwd.h \
	serialization/serialize_array.h \
	serialization/serialize_atomic.h \
	serialization/serialize_buffer_accessor.h \
	serialization/serialize_deque.h \
	serialization/serialize_list.h \
	serialization/serialize_map.h \
	serialization/serialize_multiset.h \
	serialization/serialize_output.h \
	serialization/serialize_packer.h \
	serialization/serialize_priority_queue.h \
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
	interfaces/stdMem.h \
	interfaces/simpleNetwork.h \
	interprocess/tunneldef.h \
	interprocess/mmapparent.h \
	interprocess/mmapchild_pin3.h \
	interprocess/shmchild.h \
	interprocess/shmparent.h \
	interprocess/circularBuffer.h \
	interprocess/sstmutex.h \
	interprocess/ipctunnel.h \
	rng/rng.h \
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
	statapi/statengine.h \
	statapi/statgroup.h \
	statapi/statoutput.h \
	statapi/statfieldinfo.h \
	statapi/statuniquecount.h \
	statapi/statoutputtxt.h \
	statapi/statoutputcsv.h \
	statapi/statoutputjson.h \
	statapi/statoutputhdf5.h \
	statapi/statbase.h \
	statapi/stathistogram.h \
	statapi/stataccumulator.h \
	statapi/statnull.h \
	threadsafe.h \
	cputimer.h \
	warnmacros.h \
	model/element_python.h


deprecated_nobase_dist_sst_headers =


nobase_nodist_sst_HEADERS = \
	build_info.h \
	sst_config.h

sst_core_sources = \
	action.cc \
	clock.cc \
	baseComponent.cc \
	checkpointAction.cc \
	component.cc \
	componentExtension.cc \
	componentInfo.cc \
	config.cc \
	configBase.cc \
	configShared.cc \
	configGraph.cc \
	cfgoutput/pythonConfigOutput.cc \
	cfgoutput/dotConfigOutput.cc \
	cfgoutput/xmlConfigOutput.cc \
	cfgoutput/jsonConfigOutput.cc \
	env/envquery.cc \
	env/envconfig.cc \
	eli/elibase.cc \
	eli/elementinfo.cc \
	elemLoader.cc \
	event.cc \
	exit.cc \
	factory.cc \
	heartbeat.cc \
	initQueue.cc \
	link.cc \
	linkMap.h \
	linkPair.h \
	memuse.cc \
	mempool.cc \
	mempoolAccessor.h \
	namecheck.cc \
	oneshot.cc \
	output.cc \
	params.cc \
	pollingLinkQueue.cc \
	profile/profiletool.cc \
	profile/clockHandlerProfileTool.cc \
	profile/eventHandlerProfileTool.cc \
	profile/syncProfileTool.cc \
	profile/componentProfileTool.cc \
	simulation.cc \
	stringize.cc \
	subcomponent.cc \
	timeLord.cc \
	uninitializedQueue.cc \
	unitAlgebra.cc \
	module.cc \
	ssthandler.cc \
	sstpart.cc \
	timeVortex.cc \
	serialization/serializable_base.cc \
	serialization/serialize_serializable.cc \
	serialization/serializer.cc \
	serialization/statics.cc \
	sstinfo.h \
	interfaces/TestEvent.cc \
	interfaces/stdMem.cc \
	interfaces/simpleNetwork.cc \
	interprocess/ipctunnel.cc \
	interprocess/mmapparent.cc \
	interprocess/shmparent.cc \
	rng/marsaglia.cc \
	rng/mersenne.cc \
	rng/xorshift.cc \
	statapi/statengine.cc \
	statapi/statgroup.cc \
	statapi/statoutput.cc \
	statapi/statfieldinfo.cc \
	statapi/statoutputtxt.cc \
	statapi/statoutputcsv.cc \
	statapi/statoutputjson.cc \
	statapi/statbase.cc \
	cputimer.cc \
	iouse.cc \
	objectComms.h \
	objectSerialization.h \
	simulation_impl.h

bin_PROGRAMS = sst sst-info sst-config sst-register
libexec_PROGRAMS = sstsim.x sstinfo.x

sst_info_SOURCES = \
	bootsstinfo.cc \
	bootshared.cc \
	bootshared.h \
	configBase.h \
	configBase.cc \
	configShared.h \
	configShared.cc \
	env/envquery.h \
	env/envconfig.h \
	env/envquery.cc \
	env/envconfig.cc

sst_SOURCES = \
	bootsst.cc \
	bootshared.cc \
	bootshared.h \
	configBase.h \
	configBase.cc \
	configShared.h \
	configShared.cc \
	env/envquery.h \
	env/envconfig.h \
	env/envquery.cc \
	env/envconfig.cc


sst_config_SOURCES = \
	sstconfigtool.cc \
	env/envquery.h \
	env/envconfig.h \
	env/envquery.cc \
	env/envconfig.cc

sst_register_SOURCES = \
	sstregistertool.cc \
	env/envquery.h \
	env/envconfig.h \
	env/envquery.cc \
	env/envconfig.cc

sstsim_x_SOURCES = \
	main.cc \
	$(sst_core_sources)

sstinfo_x_SOURCES = \
	sstinfo.cc \
	$(sst_core_sources) \
	$(sst_xml_sources)

sstsim_x_LDADD = \
	$(PYTHON_LIBS) \
	$(MPILIBS) \
	$(TCMALLOC_LIB) \
	-lm

sstsim_x_LDFLAGS = \
	$(TCMALLOC_LDFLAGS) \
	$(PYTHON_LDFLAGS) \
	-export-dynamic \
	$(SST_LTLIBS_ELEMLIBS)

sstinfo_x_LDADD = \
	$(MPILIBS) \
	$(TCMALLOC_LIB) \
	-lm

sstinfo_x_LDFLAGS = \
	$(TCMALLOC_LDFLAGS) \
	$(PYTHON_LDFLAGS) \
	-export-dynamic \
	$(SST_LTLIBS_ELEMLIBS)

include ../../../external/tinyxml/Makefile.inc
include ../../../external/nlohmann/Makefile.inc
include model/Makefile.inc
include shared/Makefile.inc
include impl/Makefile.inc
include sync/Makefile.inc
include testingframework/Makefile.inc
include testElements/Makefile.inc

if !SST_ENABLE_PREVIEW_BUILD
nobase_dist_sst_HEADERS += $(deprecated_nobase_dist_sst_headers)
sst_core_sources += $(deprecated_sst_core_sources)
endif

sst_core_sources += $(sst_core_python_sources)
nobase_dist_sst_HEADERS += $(sst_core_python_headers)

sst_core_sources += $(sst_core_json_sources)

if !SST_COMPILE_OSX
sstsim_x_LDADD += -lrt
sstinfo_x_LDADD += -lrt
endif

if USE_LIBZ
sstsim_x_LDADD += $(LIBZ_LIBS)
sstinfo_x_LDADD += $(LIBZ_LIBS)
endif

if USE_CURSES
AM_CPPFLAGS += $(CURSES_CPPFLAGS)
sstinfo_x_LDADD += $(CURSES_LIBS)
endif

if USE_HDF5
AM_CPPFLAGS += $(HDF5_CFLAGS)
sstsim_x_SOURCES += statapi/statoutputhdf5.cc
sstinfo_x_SOURCES += statapi/statoutputhdf5.cc
sstsim_x_LDADD += $(HDF5_LDFLAGS) $(HDF5_LIBS)
sstinfo_x_LDADD += $(HDF5_LDFLAGS) $(HDF5_LIBS)
endif
