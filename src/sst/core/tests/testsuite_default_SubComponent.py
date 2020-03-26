# -*- coding: utf-8 -*-

import os
import sys
import filecmp

from sst_unittest import *
from sst_unittest_support import *

################################################################################

class testcase_SubComponent(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_SubComponent_sc_2a(self):
        self.subcomponent_test_template("sc_2a")

    def test_SubComponent_sc_2u2u(self):
        self.subcomponent_test_template("sc_2u2u")

    def test_SubComponent_sc_2u(self):
        self.subcomponent_test_template("sc_2u")

    def test_SubComponent_sc_a(self):
        self.subcomponent_test_template("sc_a")

    def test_SubComponent_sc_u2u(self):
        self.subcomponent_test_template("sc_u2u")

    def test_SubComponent_sc_u(self):
        self.subcomponent_test_template("sc_u")

    def test_SubComponent_sc_2u2a(self):
        self.subcomponent_test_template("sc_2u2a")

    def test_SubComponent_sc_2ua(self):
        self.subcomponent_test_template("sc_2ua")

    def test_SubComponent_sc_2uu(self):
        self.subcomponent_test_template("sc_2uu")

    def test_SubComponent_sc_u2a(self):
        self.subcomponent_test_template("sc_u2a")

    def test_SubComponent_sc_ua(self):
        self.subcomponent_test_template("sc_ua")

    def test_SubComponent_sc_uu(self):
        self.subcomponent_test_template("sc_uu")

#####

    def subcomponent_test_template(self, testtype):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()

        # Set the various file paths
        sdlfile = "{0}/subcomponent_tests/test_{1}.py".format(test_path, testtype)
        reffile = "{0}/subcomponent_tests/refFiles/test_{1}.out".format(test_path, testtype)
        outfile = "{0}/test_SubComponent_{1}.out".format(get_test_output_run_dir(), testtype)

        self.run_sst(sdlfile, outfile)

        # Perform the test
        cmp_result = compare_sorted(testtype, outfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

