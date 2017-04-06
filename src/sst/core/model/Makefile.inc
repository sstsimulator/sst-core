

# -*- Makefile -*-
#
#

sst_core_sources += \
	model/sstmodel.h \
	model/sstmodel.cc \
	model/pymodel.h \
	model/pymodel.cc \
	model/pymodel_link.h \
	model/pymodel_link.cc \
	model/pymodel_comp.h \
	model/pymodel_comp.cc \
	model/pymodel_statgroup.h \
	model/pymodel_statgroup.cc \
	model/element_python.h \
	model/element_python.cc

libexec_SCRIPTS = model/xmlToPython.py
EXTRA_DIST += model/xmlToPython.py
