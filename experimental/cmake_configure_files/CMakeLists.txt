configure_file(sst_config.h.in ${sst-core_BINARY_DIR}/src/sst/core/sst_config.h)

# THIS should also cover build_info.h
add_library(sst-config-headers INTERFACE)
target_include_directories(sst-config-headers
                           INTERFACE ${sst-core_BINARY_DIR}/src/sst/core/)

install(FILES ${sst-core_BINARY_DIR}/src/sst/core/sst_config.h
        DESTINATION include/sst/core)

# Tell sst-elements (via sst-config DEBUG_SYMBOLS) whether full debug symbols are used
if(CMAKE_BUILD_TYPE MATCHES "^(Debug|RelWithDebInfo)$")
  set(SST_DEBUG_SYMBOLS_STR "yes")
else()
  set(SST_DEBUG_SYMBOLS_STR "no")
endif()

configure_file(sstsimulator.conf.in
               ${sst-core_BINARY_DIR}/src/sst/sstsimulator.conf)
install(FILES ${sst-core_BINARY_DIR}/src/sst/sstsimulator.conf
        DESTINATION "etc/sst")

configure_file(SST_version.pc.in
               ${sst-core_BINARY_DIR}/src/sst/SST-${VERSION}.pc)
install(FILES ${sst-core_BINARY_DIR}/src/sst/SST-${VERSION}.pc
        DESTINATION "lib/pkgconfg")

configure_file(build_info.h.in ${sst-core_BINARY_DIR}/src/sst/core/build_info.h)
# TODO figure out how build_info.h needs to be installed

# I included a libcoreTestElement.la.in, but we are not going to configure ibt
# since *.la is really a relic from the past
# configure_file(libcoreTestElement.la.in ...
