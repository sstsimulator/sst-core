

# -*- Makefile -*-
#
#

sst_core_sources += \
	model/sstmodel.h \
	model/sstmodel.cc \
	model/element_python.h \
	model/element_python.cc

sst_core_python2_sources = \
	model/python2/pymodel.h \
	model/python2/pymodel.cc \
	model/python2/pymodel_link.h \
	model/python2/pymodel_link.cc \
	model/python2/pymodel_comp.h \
	model/python2/pymodel_comp.cc \
	model/python2/pymodel_statgroup.h \
	model/python2/pymodel_statgroup.cc

sst_core_python3_sources = \
	model/python3/pymodel.h \
	model/python3/pymodel.cc \
	model/python3/pymodel_link.h \
	model/python3/pymodel_link.cc \
	model/python3/pymodel_comp.h \
	model/python3/pymodel_comp.cc \
	model/python3/pymodel_statgroup.h \
	model/python3/pymodel_statgroup.cc

libexec_SCRIPTS = model/xmlToPython.py
EXTRA_DIST += model/xmlToPython.py
