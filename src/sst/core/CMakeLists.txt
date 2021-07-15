# SST-CORE src/sst/core CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

#-- override the default flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIXML_USE_STL -DSST_BUILDING_CORE=1 ${Python_CPPFLAGS}")

set(SSTHeaders
activityQueue.h
action.h
activity.h
clock.h
baseComponent.h
component.h
componentExtension.h
componentInfo.h
config.h
configGraph.h
configGraphOutput.h
cfgoutput/pythonConfigOutput.h
cfgoutput/dotConfigOutput.h
cfgoutput/xmlConfigOutput.h
cfgoutput/jsonConfigOutput.h
decimal_fixedpoint.h
env/envquery.h
env/envconfig.h
elemLoader.h
event.h
exit.h
factory.h
from_string.h
heartbeat.h
initQueue.h
link.h
linkMap.h
linkPair.h
mempool.h
memuse.h
iouse.h
module.h
objectComms.h
oneshot.h
output.h
params.h
pollingLinkQueue.h
profile.h
rankInfo.h
simulation.h
sparseVectorMap.h
sst_types.h
sstinfo.h
sstpart.h
stopAction.h
stringize.h
subcomponent.h
sharedRegion.h
sharedRegionImpl.h
timeConverter.h
timeLord.h
timeVortex.h
math/sqrt.h
uninitializedQueue.h
unitAlgebra.h
part/sstpart.h
eli/elementinfo.h
eli/elibase.h
eli/categoryInfo.h
eli/defaultInfo.h
eli/elementbuilder.h
eli/interfaceInfo.h
eli/paramsInfo.h
eli/portsInfo.h
eli/statsInfo.h
eli/subcompSlotInfo.h
serialization/ser_ptr_type.h
serialization/serializable.h
serialization/serializable_fwd.h
serialization/serialize.h
serialization/serialize_array.h
serialization/serialize_buffer_accessor.h
serialization/serialize_deque.h
serialization/serialize_list.h
serialization/serialize_map.h
serialization/serialize_packer.h
serialization/serialize_serializable.h
serialization/serialize_set.h
serialization/serialize_sizer.h
serialization/serialize_string.h
serialization/serialize_unpacker.h
serialization/serialize_vector.h
serialization/serializer.h
serialization/serializer_fwd.h
serialization/statics.h
interfaces/TestEvent.h
interfaces/stringEvent.h
interfaces/simpleMem.h
interfaces/stdMem.h
interfaces/simpleNetwork.h
interprocess/tunneldef.h
interprocess/mmapparent.h
interprocess/mmapchild_pin3.h
interprocess/shmchild.h
interprocess/shmparent.h
interprocess/circularBuffer.h
interprocess/sstmutex.h
interprocess/ipctunnel.h
rng/sstrng.h
rng/rng.h
rng/marsaglia.h
rng/poisson.h
rng/mersenne.h
rng/xorshift.h
rng/distrib.h
rng/discrete.h
rng/gaussian.h
rng/expon.h
rng/constant.h
rng/uniform.h
statapi/statengine.h
statapi/statgroup.h
statapi/statoutput.h
statapi/statfieldinfo.h
statapi/statoutputconsole.h
statapi/statuniquecount.h
statapi/statoutputtxt.h
statapi/statoutputcsv.h
statapi/statoutputjson.h
statapi/statoutputhdf5.h
statapi/statbase.h
statapi/stathistogram.h
statapi/stataccumulator.h
statapi/statnull.h
threadsafe.h
cputimer.h
warnmacros.h
model/element_python.h
)

#-- ./part
set(SSTPartSrcs
part/sstpart.h
)

#-- ./model
set(SSTModelSrcs
model/sstmodel.h
model/sstmodel.cc
model/element_python.h
model/element_python.cc
)

set(SSTCorePythonHdrs
model/python/pymacros.h
)

set(SSTCorePythonSrcs
model/python/pymacros.h
model/python/pymodel.h
model/python/pymodel.cc
model/python/pymodel_link.h
model/python/pymodel_link.cc
model/python/pymodel_comp.h
model/python/pymodel_comp.cc
model/python/pymodel_unitalgebra.h
model/python/pymodel_unitalgebra.cc
model/python/pymodel_stat.h
model/python/pymodel_stat.cc
model/python/pymodel_statgroup.h
model/python/pymodel_statgroup.cc
)

#-- ./shared
set(SSTSharedSrcs
shared/sharedObject.cc
)

set(SSTModelHdrs
shared/sharedObject.h
shared/sharedArray.h
shared/sharedMap.h
shared/sharedSet.h
)

#-- impl/timevortex
set(SSTImplTimeVortexSrcs
impl/timevortex/timeVortexPQ.cc
impl/timevortex/timeVortexPQ.h
)

#-- impl/partitioners
set(SSTImplPartitionersSrcs
impl/partitioners/linpart.cc
impl/partitioners/linpart.h
impl/partitioners/rrobin.cc
impl/partitioners/rrobin.h
impl/partitioners/selfpart.h
impl/partitioners/selfpart.cc
impl/partitioners/simplepart.cc
impl/partitioners/simplepart.h
impl/partitioners/singlepart.cc
impl/partitioners/singlepart.h
)

#-- sync
set(SSTSyncSrcs
sync/rankSyncParallelSkip.h
sync/rankSyncParallelSkip.cc
sync/rankSyncSerialSkip.h
sync/rankSyncSerialSkip.cc
sync/syncManager.h
sync/syncManager.cc
sync/syncQueue.h
sync/syncQueue.cc
sync/threadSyncSimpleSkip.h
sync/threadSyncSimpleSkip.cc
sync/threadSyncQueue.h
)

#-- testElements
set(SSTTestElements
testElements/coreTest_Component.h
testElements/coreTest_Component.cc
testElements/coreTest_ComponentEvent.h
testElements/coreTest_ClockerComponent.h
testElements/coreTest_ClockerComponent.cc
testElements/coreTest_DistribComponent.h
testElements/coreTest_DistribComponent.cc
testElements/coreTest_RNGComponent.h
testElements/coreTest_RNGComponent.cc
testElements/coreTest_StatisticsComponent.h
testElements/coreTest_StatisticsComponent.cc
testElements/coreTest_Links.h
testElements/coreTest_Links.cc
testElements/coreTest_Message.h
testElements/coreTest_MessageGeneratorComponent.h
testElements/coreTest_MessageGeneratorComponent.cc
testElements/coreTest_SharedObjectComponent.h
testElements/coreTest_SharedObjectComponent.cc
testElements/coreTest_SubComponent.h
testElements/coreTest_SubComponent.cc
testElements/coreTest_Module.h
testElements/coreTest_Module.cc
testElements/coreTest_ParamComponent.h
testElements/coreTest_ParamComponent.cc
)

set(SSTCoreSrcs
action.cc
clock.cc
baseComponent.cc
component.cc
componentExtension.cc
componentInfo.cc
config.cc
configGraph.cc
cfgoutput/pythonConfigOutput.cc
cfgoutput/dotConfigOutput.cc
cfgoutput/xmlConfigOutput.cc
cfgoutput/jsonConfigOutput.cc
env/envquery.cc
env/envconfig.cc
eli/elibase.cc
eli/elementinfo.cc
elemLoader.cc
event.cc
exit.cc
factory.cc
heartbeat.cc
initQueue.cc
link.cc
memuse.cc
oneshot.cc
output.cc
params.cc
pollingLinkQueue.cc
simulation.cc
subcomponent.cc
sharedRegion.cc
timeLord.cc
uninitializedQueue.cc
unitAlgebra.cc
module.cc
sstpart.cc
timeVortex.cc
serialization/serializable.cc
serialization/serialize_serializable.cc
serialization/serializer.cc
serialization/statics.cc
interfaces/TestEvent.cc
interfaces/simpleMem.cc
interfaces/stdMem.cc
interfaces/simpleNetwork.cc
interprocess/ipctunnel.cc
interprocess/mmapparent.cc
interprocess/shmparent.cc
rng/marsaglia.cc
rng/mersenne.cc
rng/xorshift.cc
statapi/statengine.cc
statapi/statgroup.cc
statapi/statoutput.cc
statapi/statfieldinfo.cc
statapi/statoutputconsole.cc
statapi/statoutputtxt.cc
statapi/statoutputcsv.cc
statapi/statoutputjson.cc
statapi/statbase.cc
cputimer.cc
iouse.cc
simulation_impl.h
${SSTPartSrcs}
${SSTModelSrcs}
${SSTSharedSrcs}
${SSTCorePythonSrcs}
${SSTImplTimeVortexSrcs}
${SSTImplPartitionersSrcs}
${SSTSyncSrcs}
)

set(SSTInfoSrcs
bootsstinfo.cc
bootshared.cc
bootshared.h
env/envquery.h
env/envconfig.h
env/envquery.cc
env/envconfig.cc
)

set(SSTSrcs
bootsst.cc
bootshared.cc
bootshared.h
env/envquery.h
env/envconfig.h
env/envquery.cc
env/envconfig.cc
)

set(SSTConfigSrcs
sstconfigtool.cc
env/envquery.h
env/envconfig.h
env/envquery.cc
env/envconfig.cc
)

set(SSTRegisterSrcs
sstregistertool.cc
env/envquery.h
env/envconfig.h
env/envquery.cc
env/envconfig.cc
)

set(SSTXMLSrcs
tinyxml/tinystr.cpp
tinyxml/tinyxml.cpp
tinyxml/tinyxmlerror.cpp
tinyxml/tinyxmlparser.cpp
tinyxml/tinystr.h
tinyxml/tinyxml.h
)

set(SST_Dist_Hdrs
build_info.h
sst_config.h
${SSTHeaders}
${SSTModelHdrs}
${SSTCorePythonHdrs}
)

set(SST_X_Srcs
main.cc
${SSTCoreSrcs}
)

set(SSTInfo_X_Srcs
sstinfo.cc
${SSTCoreSrcs}
${SSTXMLSrcs}
)

set(SSTLibexec
testingframework/test_engine.py
testingframework/sst_unittest_parameterized.py
testingframework/sst_unittest.py
testingframework/test_engine_globals.py
testingframework/test_engine_junit.py
testingframework/test_engine.py
testingframework/test_engine_support.py
testingframework/test_engine_unittest.py
model/xmlToPython.py
)

set(SSTPythonBin
testingframework/sst_test_engine_loader.py
testingframework/sst-test-core
testingframework/sst-test-elements
)


add_executable(sst ${SST_X_Srcs})
add_executable(sst-info ${SSTInfo_X_Srcs})
add_executable(sst-config ${SSTConfigSrcs})
add_executable(sst-register ${SSTRegisterSrcs})

add_library(coreTestElement MODULE ${SSTTestElements})

set_target_properties(sst PROPERTIES LINK_FLAGS ${SST_SIM_LDFLAGS})
set_target_properties(sst-info PROPERTIES LINK_FLAGS ${SST_INFO_LDFLAGS})
target_link_libraries(sst "${SST_LIB}")
target_link_libraries(sst-info "${SST_LIB}")

include_directories(${SST_INCLUDE_DIRS})
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(${HDF5_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS sst DESTINATION bin)
install(TARGETS sst-info DESTINATION bin)
install(TARGETS sst-config DESTINATION bin)
install(TARGETS sst-register DESTINATION bin)
install(TARGETS coreTestElement DESTINATION lib/sstcore)
install(FILES ${SSTLibexec} DESTINATION libexec)
install(FILES ${SSTPythonBin} DESTINATION bin)
install(FILES testElements/libcoreTestElement.la DESTINATION lib/sstcore)

install(DIRECTORY "."
        DESTINATION "include/sst/core"
        FILES_MATCHING
        PATTERN "*.h")


# EOF
