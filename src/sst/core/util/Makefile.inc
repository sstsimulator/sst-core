

# -*- Makefile -*-
#
#


sst_core_sources += \
	util/basicPerf.cc \
	util/basicPerf.h \
	util/smartTextFormatter.cc \
	util/filesystem.cc \
	util/perfReporter.cc \
	util/perfReporter.h

nobase_dist_sst_HEADERS += \
	util/bit_util.h \
	util/smartTextFormatter.h \
	util/filesystem.h

sst_info_SOURCES += \
	util/smartTextFormatter.cc

sst_SOURCES += \
	util/smartTextFormatter.cc
