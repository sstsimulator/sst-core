# -*- Makefile -*-
#
#

sst_core_sources += \
	part/simplepart.cc \
	part/simplepart.h \
	part/rrobin.cc \
	part/rrobin.h \
	part/sstpart.cc \
	part/sstpart.h \
	part/linpart.cc \
	part/linpart.h	

if HAVE_ZOLTAN
sst_core_sources += \
	part/zoltpart.h \
	part/zoltpart.cc
endif
