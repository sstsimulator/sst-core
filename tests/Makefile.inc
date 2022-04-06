# -*- Makefile -*-
#
#

EXTRA_DIST += \
    tests/testsuite_default_Component.py \
    tests/testsuite_default_PerfComponent.py \
    tests/testsuite_default_RNGComponent.py \
    tests/testsuite_default_Links.py \
    tests/testsuite_default_ParamComponent.py \
    tests/testsuite_default_SharedObject.py \
    tests/testsuite_default_SubComponent.py \
    tests/testsuite_default_UnitAlgebra.py \
    tests/testsuite_default_config_input_output.py \
    tests/testsuite_default_partitioner.py \
    tests/testsuite_default_Serialization.py \
    tests/testsuite_testengine_testing.py \
    tests/test_Component.py \
    tests/test_ClockerComponent.py \
    tests/test_DistribComponent_discrete.py \
    tests/test_DistribComponent_expon.py \
    tests/test_DistribComponent_gaussian.py \
    tests/test_DistribComponent_poisson.py \
    tests/test_LookupTable.py \
    tests/test_LookupTable2.py \
    tests/test_MessageMesh.py \
    tests/test_ParamComponent.py \
    tests/test_ParallelLoad.py \
    tests/test_RNGComponent.py \
    tests/test_RNGComponent_mersenne.py \
    tests/test_RNGComponent_marsaglia.py \
    tests/test_RNGComponent_xorshift.py \
    tests/test_Serialization.py \
    tests/test_SharedObject.py \
    tests/test_StatisticsComponent.py \
    tests/test_Links.py \
    tests/test_MessageGeneratorComponent.py \
    tests/test_SubComponent.py \
    tests/test_SubComponent_2.py \
    tests/test_UnitAlgebra.py \
    tests/test_PerfComponent.py \
    tests/refFiles/test_Component.out \
    tests/refFiles/test_PerfComponent.out \
    tests/refFiles/test_DistribComponent_discrete.out \
    tests/refFiles/test_DistribComponent_expon.out \
    tests/refFiles/test_DistribComponent_gaussian.out \
    tests/refFiles/test_LookupTableComponent.out \
    tests/refFiles/test_ParamComponent.out \
    tests/refFiles/test_MessageGeneratorComponent.out \
    tests/refFiles/test_RNGComponent_marsaglia.out \
    tests/refFiles/test_RNGComponent_mersenne.out \
    tests/refFiles/test_RNGComponent_xorshift.out \
    tests/refFiles/test_StatisticsComponent.out \
    tests/refFiles/test_Links_basic.out \
    tests/refFiles/test_Links_dangling.out \
    tests/refFiles/test_Links_wrong_port.out \
    tests/refFiles/test_Serialization.out \
    tests/refFiles/test_SubComponent_2.out \
    tests/refFiles/test_SubComponent.out \
    tests/refFiles/test_UnitAlgebra.out \
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
    tests/subcomponent_tests/test_sc_uu.py \
    tests/subcomponent_tests/refFiles/test_sc_2a.out \
    tests/subcomponent_tests/refFiles/test_sc_2u.out \
    tests/subcomponent_tests/refFiles/test_sc_2u2a.out \
    tests/subcomponent_tests/refFiles/test_sc_2u2u.out \
    tests/subcomponent_tests/refFiles/test_sc_2ua.out \
    tests/subcomponent_tests/refFiles/test_sc_2uu.out \
    tests/subcomponent_tests/refFiles/test_sc_a.out \
    tests/subcomponent_tests/refFiles/test_sc_u.out \
    tests/subcomponent_tests/refFiles/test_sc_u2a.out \
    tests/subcomponent_tests/refFiles/test_sc_u2u.out \
    tests/subcomponent_tests/refFiles/test_sc_ua.out \
    tests/subcomponent_tests/refFiles/test_sc_uu.out

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
    tests/subcomponent_tests/legacy/test_sc_legacy_nn.py \
    tests/subcomponent_tests/legacy/refFiles/test_sc_legacy_2l.out \
    tests/subcomponent_tests/legacy/refFiles/test_sc_legacy_2n.out \
    tests/subcomponent_tests/legacy/refFiles/test_sc_legacy_2n2l.out \
    tests/subcomponent_tests/legacy/refFiles/test_sc_legacy_2n2n.out \
    tests/subcomponent_tests/legacy/refFiles/test_sc_legacy_2nl.out \
    tests/subcomponent_tests/legacy/refFiles/test_sc_legacy_2nn.out \
    tests/subcomponent_tests/legacy/refFiles/test_sc_legacy_l.out \
    tests/subcomponent_tests/legacy/refFiles/test_sc_legacy_n.out \
    tests/subcomponent_tests/legacy/refFiles/test_sc_legacy_n2l.out \
    tests/subcomponent_tests/legacy/refFiles/test_sc_legacy_n2n.out \
    tests/subcomponent_tests/legacy/refFiles/test_sc_legacy_nl.out \
    tests/subcomponent_tests/legacy/refFiles/test_sc_legacy_nn.out


if !SST_ENABLE_PREVIEW_BUILD
EXTRA_DIST += $(deprecated_EXTRA_DIST)
endif

##########################################################################
##########################################################################
##########################################################################


