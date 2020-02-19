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

class testsuite_Component(SSTUnitTest):

    def setUp(self):
        pass

    def tearDown(self):
        pass

#############################################

    def test_Component(self):
        self.component_test_template()

################################################################################

    def component_test_template(self):
        # Set the various file paths
        sdlfile = "{0}/test_Component.py".format(self.get_testsuite_dir())
        reffile = "{0}/refFiles/test_Component.out".format(self.get_testsuite_dir())
        outfile = "{0}/test_Component.out".format(self.get_test_output_run_dir())

        # TODO: Destroy any outfiles
        # TODO: Validate SST is an executable file

        self.run_sst(sdlfile, outfile)
#        oscmd = "sst {0}".format(sdlfile)
#        rtn = OSCommand(oscmd, outfile).run()
#        self.assertFalse(rtn.timeout(), "SST Timed-Out while running {0}".format(oscmd))
#        self.assertEqual(rtn.result(), 0, "SST returned {0}; while running {1}".format(rtn.result(), oscmd))

        # Perform the test
        cmp_result = self.compare_sorted(outfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

###

    def compare_sorted(self, outfile, reffile):
       sorted_outfile = "{0}/coreTestComponent_sorted_outfile".format(self.get_test_output_tmp_dir())
       sorted_reffile = "{0}/coreTestComponent_sorted_reffile".format(self.get_test_output_tmp_dir())

       os.system("sort -o {0} {1}".format(sorted_outfile, outfile))
       os.system("sort -o {0} {1}".format(sorted_reffile, reffile))

       return filecmp.cmp(sorted_outfile, sorted_reffile)
