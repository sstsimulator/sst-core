
# -*- Makefile -*-
#
#

sst_core_sources += \
	impl/oneshotManager.h \
	impl/oneshotManager.cc


include impl/interactive/Makefile.inc
include impl/timevortex/Makefile.inc
include impl/partitioners/Makefile.inc
include impl/portmodules/Makefile.inc
