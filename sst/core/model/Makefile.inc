

# -*- Makefile -*-
#
#

sst_core_sources += \
	model/sstmodel.h \
	model/sstmodel.cc \
	model/pymodel.h \
	model/pymodel.cc

libexec_SCRIPTS = model/xmlToPython.py
