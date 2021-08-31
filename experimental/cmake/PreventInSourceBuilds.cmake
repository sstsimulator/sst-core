# From cpp-best-practices/cpp_starter_project
# This code is in the public domain
# This function will prevent in-source builds
function(AssureOutOfSourceBuilds)
  # make sure the user doesn't play dirty with symlinks
  get_filename_component(srcdir "${SST_TOP_SRC_DIR}" REALPATH)
  get_filename_component(bindir "${CMAKE_BINARY_DIR}" REALPATH)

  # disallow in-source builds
  if("${srcdir}" STREQUAL "${bindir}")
    message(FATAL_ERROR "In-source builds are not allowed.
      This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
      Please delete them.")
  endif()

  if(EXISTS ${SST_TOP_SRC_DIR}/src/sst/core/sst_config.h OR EXISTS ${SST_TOP_SRC_DIR}/src/sst/core/build_info.h)
    message(FATAL_ERROR "Cannot run the cmake build if the autotools build was run in-source.")
  endif()
endfunction()

assureoutofsourcebuilds()
