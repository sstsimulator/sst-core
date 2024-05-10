# -*- Makefile -*-
#
#

#AM_CPPFLAGS = $(MPI_CPPFLAGS)

compdir = $(pkglibdir)

comp_LTLIBRARIES = libcoreTestElement.la

libcoreTestElement_la_SOURCES = \
	testElements/coreTest_Component.h \
	testElements/coreTest_Component.cc \
	testElements/coreTest_ComponentEvent.h \
	testElements/coreTest_ClockerComponent.h \
	testElements/coreTest_ClockerComponent.cc \
	testElements/coreTest_DistribComponent.h \
	testElements/coreTest_DistribComponent.cc \
	testElements/coreTest_RNGComponent.h \
	testElements/coreTest_RNGComponent.cc \
	testElements/coreTest_StatisticsComponent.h \
	testElements/coreTest_StatisticsComponent.cc \
	testElements/coreTest_Links.h \
	testElements/coreTest_Links.cc \
	testElements/coreTest_Message.h \
	testElements/coreTest_MessageGeneratorComponent.h \
	testElements/coreTest_MessageGeneratorComponent.cc \
	testElements/coreTest_Serialization.h \
	testElements/coreTest_Serialization.cc \
	testElements/coreTest_SharedObjectComponent.h \
	testElements/coreTest_SharedObjectComponent.cc \
	testElements/coreTest_SubComponent.h \
	testElements/coreTest_SubComponent.cc \
	testElements/coreTest_Module.h \
	testElements/coreTest_Module.cc \
	testElements/coreTest_ParamComponent.h \
	testElements/coreTest_ParamComponent.cc \
	testElements/coreTest_PerfComponent.h \
	testElements/coreTest_PerfComponent.cc \
	testElements/coreTest_MemPoolTest.h \
	testElements/coreTest_MemPoolTest.cc \
	testElements/coreTest_Checkpoint.h \
	testElements/coreTest_Checkpoint.cc \
        testElements/message_mesh/messageEvent.h \
	testElements/message_mesh/enclosingComponent.h \
	testElements/message_mesh/enclosingComponent.cc

deprecated_libcoreTestElement_sources =

if !SST_ENABLE_PREVIEW_BUILD
libcoreTestElement_la_SOURCES += $(deprecated_libcoreTestElement_sources)
endif

EXTRA_DIST += \
    testElements/README

deprecated_EXTRA_DIST =

if !SST_ENABLE_PREVIEW_BUILD
EXTRA_DIST += $(deprecated_EXTRA_DIST)
endif

libcoreTestElement_la_LDFLAGS = -module -avoid-version

##########################################################################
##########################################################################
##########################################################################


