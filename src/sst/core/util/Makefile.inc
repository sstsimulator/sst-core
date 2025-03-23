

# -*- Makefile -*-
#
#


sst_core_sources += \
	util/smartTextFormatter.cc \
	util/filesystem.cc

nobase_dist_sst_HEADERS += \
	util/smartTextFormatter.h \
	util/filesystem.h

sst_info_SOURCES += \
	util/smartTextFormatter.cc

sst_SOURCES += \
	util/smartTextFormatter.cc
