# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

################################################################################

class testcase_SubComponentLegacy(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_SubComponentLegacy_sc_2a(self):
        self.subcomponentlegacy_test_template("sc_legacy_2nl")

    def test_SubComponentLegacy_sc_2u2u(self):
        self.subcomponentlegacy_test_template("sc_legacy_n2l")

    def test_SubComponentLegacy_sc_2u(self):
        self.subcomponentlegacy_test_template("sc_legacy_n")

    def test_SubComponentLegacy_sc_a(self):
        self.subcomponentlegacy_test_template("sc_legacy_2l")

    def test_SubComponentLegacy_sc_u2u(self):
        self.subcomponentlegacy_test_template("sc_legacy_2nn")

    def test_SubComponentLegacy_sc_u(self):
        self.subcomponentlegacy_test_template("sc_legacy_n2n")

    def test_SubComponentLegacy_sc_2u2a(self):
        self.subcomponentlegacy_test_template("sc_legacy_2n2l")

    def test_SubComponentLegacy_sc_2ua(self):
        self.subcomponentlegacy_test_template("sc_legacy_2n")

    def test_SubComponentLegacy_sc_2uu(self):
        self.subcomponentlegacy_test_template("sc_legacy_nl")

    def test_SubComponentLegacy_sc_u2a(self):
        self.subcomponentlegacy_test_template("sc_legacy_2n2n")

    def test_SubComponentLegacy_sc_ua(self):
        self.subcomponentlegacy_test_template("sc_legacy_l")

    def test_SubComponentLegacy_sc_uu(self):
        self.subcomponentlegacy_test_template("sc_legacy_nn")

#####

    def subcomponentlegacy_test_template(self, testtype):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()

        # Set the various file paths
        sdlfile = "{0}/subcomponent_tests/legacy/test_{1}.py".format(test_path, testtype)
        reffile = "{0}/subcomponent_tests/legacy/refFiles/test_{1}.out".format(test_path, testtype)
        outfile = "{0}/test_SubComponentlegacy_{1}.out".format(get_test_output_run_dir(), testtype)

        self.run_sst(sdlfile, outfile)

        # Perform the test
        cmp_result = compare_sorted(testtype, outfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))


