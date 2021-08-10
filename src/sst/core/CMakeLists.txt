# SST-CORE src/sst/core CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

#-- override the default flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSST_BUILDING_CORE=1 ${Python_CPPFLAGS}")

configure_file(sst_config-cmake.h.in sst_config.h)

add_subdirectory(impl)
add_subdirectory(model)
add_subdirectory(shared)
add_subdirectory(sync)
add_subdirectory(testingframework)
add_subdirectory(testElements)

add_library(sst-core-lib OBJECT
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
)

add_library(sst-boot-lib OBJECT
  bootsstinfo.cc
  bootshared.cc
  )
add_library(sst-env-lib OBJECT
  env/envquery.cc
  env/envconfig.cc
  )

target_link_libraries(sst-core-lib 
  PUBLIC 
    libjson libpartitioner libtimeVortex libmodelcore 
    libmodelpython libsync ${SST_LIB})

add_executable(sst main.cc)
target_link_libraries(sst PRIVATE sst-core-lib)

add_executable(sst-info sstinfo.cc)
target_link_libraries(sst-info PRIVATE sst-core-lib libtinyxml)

add_executable(sst-config sstconfigtool.cc)
target_link_libraries(sst-info PRIVATE sst-env-lib)

add_executable(sst-register sstregistertool.cc)
target_link_libraries(sst-register PRIVATE sst-env-lib)

set_target_properties(sst PROPERTIES LINK_FLAGS ${SST_SIM_LDFLAGS})
set_target_properties(sst-info PROPERTIES LINK_FLAGS ${SST_INFO_LDFLAGS})

install(TARGETS sst) 
install(TARGETS sst-info)
install(TARGETS sst-config)
install(TARGETS sst-register)

# Don't build this it's an old thing that nots needed by cmake
# install(FILES testElements/libcoreTestElement.la DESTINATION lib/sstcore)

install(DIRECTORY "."
        DESTINATION "include/sst/core"
        FILES_MATCHING
        PATTERN "*.h")

# Unused
#set(SSTHeaders
#activityQueue.h
#action.h
#activity.h
#clock.h
#baseComponent.h
#component.h
#componentExtension.h
#componentInfo.h
#config.h
#configGraph.h
#configGraphOutput.h
#cfgoutput/pythonConfigOutput.h
#cfgoutput/dotConfigOutput.h
#cfgoutput/xmlConfigOutput.h
#cfgoutput/jsonConfigOutput.h
#decimal_fixedpoint.h
#env/envquery.h
#env/envconfig.h
#elemLoader.h
#event.h
#exit.h
#factory.h
#from_string.h
#heartbeat.h
#initQueue.h
#link.h
#linkMap.h
#linkPair.h
#mempool.h
#memuse.h
#iouse.h
#module.h
#objectComms.h
#oneshot.h
#output.h
#params.h
#pollingLinkQueue.h
#profile.h
#rankInfo.h
#simulation.h
#sparseVectorMap.h
#sst_types.h
#sstinfo.h
#sstpart.h
#stopAction.h
#stringize.h
#subcomponent.h
#sharedRegion.h
#sharedRegionImpl.h
#timeConverter.h
#timeLord.h
#timeVortex.h
#math/sqrt.h
#uninitializedQueue.h
#unitAlgebra.h
#part/sstpart.h
#eli/elementinfo.h
#eli/elibase.h
#eli/categoryInfo.h
#eli/defaultInfo.h
#eli/elementbuilder.h
#eli/interfaceInfo.h
#eli/paramsInfo.h
#eli/portsInfo.h
#eli/statsInfo.h
#eli/subcompSlotInfo.h
#serialization/ser_ptr_type.h
#serialization/serializable.h
#serialization/serializable_fwd.h
#serialization/serialize.h
#serialization/serialize_array.h
#serialization/serialize_buffer_accessor.h
#serialization/serialize_deque.h
#serialization/serialize_list.h
#serialization/serialize_map.h
#serialization/serialize_packer.h
#serialization/serialize_serializable.h
#serialization/serialize_set.h
#serialization/serialize_sizer.h
#serialization/serialize_string.h
#serialization/serialize_unpacker.h
#serialization/serialize_vector.h
#serialization/serializer.h
#serialization/serializer_fwd.h
#serialization/statics.h
#interfaces/TestEvent.h
#interfaces/stringEvent.h
#interfaces/simpleMem.h
#interfaces/stdMem.h
#interfaces/simpleNetwork.h
#interprocess/tunneldef.h
#interprocess/mmapparent.h
#interprocess/mmapchild_pin3.h
#interprocess/shmchild.h
#interprocess/shmparent.h
#interprocess/circularBuffer.h
#interprocess/sstmutex.h
#interprocess/ipctunnel.h
#rng/sstrng.h
#rng/rng.h
#rng/marsaglia.h
#rng/poisson.h
#rng/mersenne.h
#rng/xorshift.h
#rng/distrib.h
#rng/discrete.h
#rng/gaussian.h
#rng/expon.h
#rng/constant.h
#rng/uniform.h
#statapi/statengine.h
#statapi/statgroup.h
#statapi/statoutput.h
#statapi/statfieldinfo.h
#statapi/statoutputconsole.h
#statapi/statuniquecount.h
#statapi/statoutputtxt.h
#statapi/statoutputcsv.h
#statapi/statoutputjson.h
#statapi/statoutputhdf5.h
#statapi/statbase.h
#statapi/stathistogram.h
#statapi/stataccumulator.h
#statapi/statnull.h
#threadsafe.h
#cputimer.h
#warnmacros.h
#model/element_python.h
#)

# unused 
#-- ./part
#set(SSTPartSrcs
#part/sstpart.h
#)

# Unused 
#set(SST_Dist_Hdrs
#build_info.h
#sst_config.h
#${SSTHeaders}
#${SSTModelHdrs}
#${SSTCorePythonHdrs}
#)

#set(SSTCorePythonHdrs
#model/python/pymacros.h
#)

#set(SSTModelHdrs
#shared/sharedObject.h
#shared/sharedArray.h
#shared/sharedMap.h
#shared/sharedSet.h
#)


# EOF
