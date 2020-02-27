# -*- coding: utf-8 -*-

import os
import sys
import filecmp

from sst_unittest_support import *

################################################################################

def setUpModule():
    test_engine_setup_module()
    # Put Module based setup code here. it is called before any testcases are run

def tearDownModule():
    # Put Module based teardown code here. it is called after all testcases are run
    test_engine_teardown_module()

################################################################################

class testcase_SubComponentLegacy(SSTTestCase):

    @classmethod
    def setUpClass(cls):
        super(cls, cls).setUpClass()
        # Put class based setup code here. it is called once before tests are run

    @classmethod
    def tearDownClass(cls):
        # Put class based teardown code here. it is called once after tests are run
        super(cls, cls).tearDownClass()

#####

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

    # DEMO tests
    def test_success(self):
        self.assertEqual(1, 1)

    def test_fail1(self):
        self.assertEqual(2, 1)

    def test_fail2(self):
        self.assertEqual(2, 1)

    def test_fail3(self):
        self.assertEqual(2, 1)

    def test_error1(self):
        self.assertEqual(1 / 0, 1)

    def test_error2(self):
        self.assertEqual(1 / 0, 1)

    @unittest.skip("Demonstrating Skipping")
    def test_skipping(self):
        self.assertEqual(1 / 0, 1)
#####

    def subcomponentlegacy_test_template(self, testtype):
        # Set the various file paths
        sdlfile = "{0}/subcomponent_tests/legacy/test_{1}.py".format(self.get_testsuite_dir(), testtype)
        reffile = "{0}/subcomponent_tests/legacy/refFiles/test_{1}.out".format(self.get_testsuite_dir(), testtype)
        outfile = "{0}/test_SubComponentlegacy_{1}.out".format(self.get_test_output_run_dir(), testtype)

        self.run_sst(sdlfile, outfile)

        # Perform the test
        cmp_result = self.compare_sorted(outfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

#####

    def compare_sorted(self, outfile, reffile):
       sorted_outfile = "{0}/coreTestSubComponentLegacy_sorted_outfile".format(self.get_test_output_tmp_dir())
       sorted_reffile = "{0}/coreTestSubComponentLegacy_sorted_reffile".format(self.get_test_output_tmp_dir())

       os.system("sort -o {0} {1}".format(sorted_outfile, outfile))
       os.system("sort -o {0} {1}".format(sorted_reffile, reffile))

       return filecmp.cmp(sorted_outfile, sorted_reffile)

