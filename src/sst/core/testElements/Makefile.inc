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
	testElements/coreTest_Message.h \
	testElements/coreTest_MessageGeneratorComponent.h \
	testElements/coreTest_MessageGeneratorComponent.cc \
	testElements/coreTest_LookupTableComponent.h \
	testElements/coreTest_LookupTableComponent.cc \
	testElements/coreTest_SubComponent.h \
	testElements/coreTest_SubComponent.cc \
	testElements/coreTest_Module.h \
	testElements/coreTest_Module.cc \
	testElements/coreTest_ParamComponent.h \
	testElements/coreTest_ParamComponent.cc

deprecated_libcoreTestElement_sources = \
	testElements/coreTest_SubComponentLegacy.h \
	testElements/coreTest_SubComponentLegacy.cc

if !SST_ENABLE_PREVIEW_BUILD
libcoreTestElement_la_SOURCES += $(deprecated_libcoreTestElement_sources)
endif

EXTRA_DIST += \
    README \
    tests/test_ParamComponent.py \
    tests/test_Component.py \
    tests/test_ClockerComponent.py \
    tests/test_DistribComponent_discrete.py \
    tests/test_DistribComponent_expon.py \
    tests/test_DistribComponent_gaussian.py \
    tests/test_DistribComponent_poisson.py \
    tests/test_LookupTable.py \
    tests/test_RNGComponent_mersenne.py \
    tests/test_RNGComponent_marsaglia.py \
    tests/test_RNGComponent_xorshift.py \
    tests/test_StatisticsComponent.py \
    tests/test_MessageGeneratorComponent.py \
    tests/test_SubComponent.py \
    tests/test_SubComponent_2.py \
    tests/subcomponent_tests/test_sc_2a.py \
    tests/subcomponent_tests/test_sc_2u2u.py \
    tests/subcomponent_tests/test_sc_2u.py \
    tests/subcomponent_tests/test_sc_a.py \
    tests/subcomponent_tests/test_sc_u2u.py \
    tests/subcomponent_tests/test_sc_u.py \
    tests/subcomponent_tests/test_sc_2u2a.py \
    tests/subcomponent_tests/test_sc_2ua.py \
    tests/subcomponent_tests/test_sc_2uu.py \
    tests/subcomponent_tests/test_sc_u2a.py \
    tests/subcomponent_tests/test_sc_ua.py \
    tests/subcomponent_tests/test_sc_uu.py


deprecated_EXTRA_DIST = \
    tests/subcomponent_tests/legacy/test_sc_legacy_2nl.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_n2l.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_n.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_2l.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_2nn.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_n2n.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_2n2l.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_2n.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_nl.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_2n2n.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_l.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_nn.py

if !SST_ENABLE_PREVIEW_BUILD
EXTRA_DIST += $(deprecated_EXTRA_DIST)
endif

libcoreTestElement_la_LDFLAGS = -module -avoid-version

##########################################################################
##########################################################################
##########################################################################

