# -*- Makefile -*-
#
#

sst_core_sources += \
	impl/partitioners/linpart.cc \
	impl/partitioners/linpart.h \
	impl/partitioners/rrobin.cc \
	impl/partitioners/rrobin.h \
	impl/partitioners/selfpart.h \
	impl/partitioners/selfpart.cc \
	impl/partitioners/simplepart.cc \
	impl/partitioners/simplepart.h \
	impl/partitioners/singlepart.cc \
	impl/partitioners/singlepart.h 

if HAVE_ZOLTAN
sst_core_sources += \
	impl/partitioners/zoltpart.h \
	impl/partitioners/zoltpart.cc
endif
