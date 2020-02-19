# -*- coding: utf-8 -*-

import os
import sys
import filecmp

from sst_unittest_support import *

################################################################################

def setUpModule():
    pass

def tearDownModule():
    pass

################################################################################

class testsuite_SubComponent(SSTUnitTest):

    def setUp(self):
        pass

    def tearDown(self):
        pass

#############################################

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



################################################################################

    def subcomponent_test_template(self, testtype):
        # Set the various file paths
        sdlfile = "{0}/subcomponent_tests/test_{1}.py".format(self.get_testsuite_dir(), testtype)
        reffile = "{0}/subcomponent_tests/refFiles/test_{1}.out".format(self.get_testsuite_dir(), testtype)
        outfile = "{0}/test_SubComponent_{1}.out".format(self.get_test_output_run_dir(), testtype)

        # TODO: Destroy any outfiles
        # TODO: Validate SST is an executable file

        self.run_sst(sdlfile, outfile)

        # Perform the test
        cmp_result = self.compare_sorted(outfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

###

    def compare_sorted(self, outfile, reffile):
       sorted_outfile = "{0}/coreTestSubComponent_sorted_outfile".format(self.get_test_output_tmp_dir())
       sorted_reffile = "{0}/coreTestSubComponent_sorted_reffile".format(self.get_test_output_tmp_dir())

       os.system("sort -o {0} {1}".format(sorted_outfile, outfile))
       os.system("sort -o {0} {1}".format(sorted_reffile, reffile))

       return filecmp.cmp(sorted_outfile, sorted_reffile)

