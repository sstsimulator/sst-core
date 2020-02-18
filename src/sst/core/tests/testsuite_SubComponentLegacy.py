# -*- coding: utf-8 -*-

import os
import sys
import filecmp

import test_globals
from test_support import *

################################################################################

def setUpModule():
    pass

def tearDownModule():
    pass

################################################################################

class testsuite_SubComponentLegacy(SSTUnitTest):

    def setUp(self):
        pass

    def tearDown(self):
        pass

#############################################

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


################################################################################

    def subcomponentlegacy_test_template(self, testtype):
        # Set the various file paths
        sdlfile = "{0}/subcomponent_tests/legacy/test_{1}.py".format(self.get_testsuite_dir(), testtype)
        reffile = "{0}/subcomponent_tests/legacy/refFiles/test_{1}.out".format(self.get_testsuite_dir(), testtype)
        outfile = "{0}/test_SubComponentlegacy_{1}.out".format(get_test_output_run_dir(), testtype)

        # TODO: Destroy any outfiles
        # TODO: Validate SST is an executable file

        # Build the launch command
        oscmd = "sst {0}".format(sdlfile)
        rtn = OSCommand(oscmd, outfile).run()
        self.assertFalse(rtn.timeout(), "SST Timed-Out while running {0}".format(oscmd))
        self.assertEqual(rtn.result(), 0, "SST returned {0}; while running {1}".format(rtn.result(), oscmd))

        # Perform the test
        cmp_result = self.compare_sorted(outfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

###

    def compare_sorted(self, outfile, reffile):
       sorted_outfile = "{0}/coreTestSubComponentLegacy_sorted_outfile".format(get_test_output_tmp_dir())
       sorted_reffile = "{0}/coreTestSubComponentLegacy_sorted_reffile".format(get_test_output_tmp_dir())

       os.system("sort -o {0} {1}".format(sorted_outfile, outfile))
       os.system("sort -o {0} {1}".format(sorted_reffile, reffile))

       return filecmp.cmp(sorted_outfile, sorted_reffile)

