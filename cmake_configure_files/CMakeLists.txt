configure_file(sst_config.h.in ${CMAKE_BINARY_DIR}/src/sst/core/sst_config.h)

# THIS should also cover build_info.h
add_library(sst-config-headers INTERFACE)
target_include_directories(sst-config-headers
                           INTERFACE ${CMAKE_BINARY_DIR}/src/sst/core/)

install(FILES ${CMAKE_BINARY_DIR}/src/sst/core/sst_config.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/sst/core)

configure_file(sstsimulator.conf.in
               ${CMAKE_BINARY_DIR}/src/sst/sstsimulator.conf)
install(FILES ${CMAKE_BINARY_DIR}/src/sst/sstsimulator.conf
        DESTINATION "etc/sst")

configure_file(SST_version.pc.in ${CMAKE_BINARY_DIR}/src/sst/SST-${VERSION}.pc)
install(FILES ${CMAKE_BINARY_DIR}/src/sst/SST-${VERSION}.pc
        DESTINATION "lib/pkgconfg")

configure_file(build_info.h.in ${CMAKE_BINARY_DIR}/src/sst/core/build_info.h)
# TODO figure out how build_info.h needs to be installed

# I included a libcoreTestElement.la.in, but we are not going to configure ibt
# since *.la is really a relic from the past
# configure_file(libcoreTestElement.la.in ...
