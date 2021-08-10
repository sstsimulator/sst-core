find_package(Doxygen)
option(
  BUILD_DOCUMENTATION
  "Create and install the doxygen-ized API documentation (requires Doxygen)"
  ${DOXYGEN_FOUND})

# ------------------------------------------------------------------------
# -- DOCUMENTATION
# ------------------------------------------------------------------------
if(BUILD_DOCUMENTATION)
  find_package(Git)
  if(NOT Git_FOUND)
    message(FATAL_ERROR "Git is required to tag the source documentation.")
  endif()
  if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen is required to build the documentation.")
  endif()

  # -- set the preprocessor defs
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --match=NeVeRmAtCh --always --abbrev=40
            --dirty
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE SST_SVN_REVISION
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

  # -- preprocess the doxyfile
  message(
    STATUS "SST: PREPROCESSING ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in")

  configure_file(Doxyfile.in Doxyfile.conf)

  doxygen_add_docs(doc Doxyfile.conf)

  #add_custom_target(
  #  doc
  #  COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
  #  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  #  COMMENT "Generating API documentation with Doxygen"
  #  VERBATIM)

  #install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/html DESTINATION share/doc)
endif(BUILD_DOCUMENTATION)
