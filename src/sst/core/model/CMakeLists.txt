add_subdirectory(python)

add_library(libmodelcore OBJECT sstmodel.cc element_python.cc)
install(FILES xmlToPython.py DESTINATION libexec)
