

# -*- Makefile -*-
#
#


sst_core_sources += \
	model/sstmodel.h \
	model/sstmodel.cc \
	model/configComponent.cc \
	model/configGraph.cc \
	model/configLink.cc \
	model/configStatistic.cc \
	model/element_python.h \
	model/element_python.cc \
	model/restart/sstcptmodel.h \
	model/restart/sstcptmodel.cc \
	model/cfgoutput/pythonConfigOutput.h \
	model/cfgoutput/pythonConfigOutput.cc \
	model/cfgoutput/dotConfigOutput.h \
	model/cfgoutput/dotConfigOutput.cc \
	model/cfgoutput/xmlConfigOutput.h \
	model/cfgoutput/xmlConfigOutput.cc \
	model/cfgoutput/jsonConfigOutput.h \
	model/cfgoutput/jsonConfigOutput.cc


nobase_dist_sst_HEADERS += \
	model/configComponent.h \
	model/configGraph.h \
	model/configLink.h \
	model/configStatistic.h \
	model/element_python.h


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
	model/python/pymodel_statgroup.cc \
	model/python/pymodel_portmodule.h \
	model/python/pymodel_portmodule.cc

sst_core_json_sources = \
  model/json/jsonmodel.h \
  model/json/jsonmodel.cc

libexec_SCRIPTS = model/xmlToPython.py
EXTRA_DIST += model/xmlToPython.py
