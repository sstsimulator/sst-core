add_subdirectory(python)

add_library(modelCore OBJECT sstmodel.cc element_python.cc)
target_include_directories(modelCore PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(modelCore PRIVATE sst-config-headers Python::Python)

install(FILES xmlToPython.py DESTINATION libexec)
