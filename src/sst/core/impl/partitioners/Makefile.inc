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

if HAVE_METIS
sst_core_sources += \
	impl/partitioners/util.hpp \
	impl/partitioners/csrmat.hpp \
	impl/partitioners/metispart.h \
	impl/partitioners/metispart.cc
endif