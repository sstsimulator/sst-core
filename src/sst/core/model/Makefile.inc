

# -*- Makefile -*-
#
#


sst_core_sources += \
	model/sstmodel.h \
	model/sstmodel.cc \
	model/element_python.h \
	model/element_python.cc

sst_core_python_headers = \
  model/python/pymacros.h

sst_core_python_sources = \
  model/python/pymacros.h \
	model/python/pymodel.h \
	model/python/pymodel.cc \
	model/python/pymodel_link.h \
	model/python/pymodel_link.cc \
	model/python/pymodel_comp.h \
	model/python/pymodel_comp.cc \
	model/python/pymodel_unitalgebra.h \
	model/python/pymodel_unitalgebra.cc \
	model/python/pymodel_stat.h \
	model/python/pymodel_stat.cc \
	model/python/pymodel_statgroup.h \
	model/python/pymodel_statgroup.cc

libexec_SCRIPTS = model/xmlToPython.py
EXTRA_DIST += model/xmlToPython.py
