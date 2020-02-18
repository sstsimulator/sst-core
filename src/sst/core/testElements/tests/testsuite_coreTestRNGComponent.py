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

############

class testsuite_simpleRNGComponent(SSTUnitTest):

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_RNG_Mersenne(self):
        self.RNG_test_template("mersenne")
        log("\n=======================================================")

        log("")
        log("=== ls cmd")
        os_ls()

        log("")
        log("=== cat VERSION file")
        os_cat("VERSION")

        log("")
        log("=== Run tail and force a timeout")
        cmd = "tail".format()
        rtn = OSCommand(cmd).run(timeout=5)
        log("Tail (forced Timeout) Rtn = {0}".format(rtn))

        log("\n=======================================================")


    def test_RNG_Marsaglia(self):
        self.RNG_test_template("marsaglia")


    def test_RNG_xorshift(self):
        self.RNG_test_template("xorshift")

###

    def RNG_test_template(self, testcase):
        # Set the various file paths
        sdlfile = "{0}/test_simpleRNGComponent_{1}.py".format(self.get_testsuite_dir(), testcase)
        reffile = "{0}/refFiles/test_simpleRNGComponent_{1}.out".format(self.get_testsuite_dir(), testcase)
        outfile = "{0}/test_simpleRNGComponent_{1}.out".format(get_test_output_run_dir(), testcase)
        tmpfile = "{0}/test_simpleRNGComponent_{1}.tmp".format(get_test_output_run_dir(), testcase)
        cmpfile = "{0}/test_simpleRNGComponent_{1}.cmp".format(get_test_output_run_dir(), testcase)

        # TODO: Destroy any outfiles
        # TODO: Validate SST is an executable file

        # Build the launch command
        oscmd = "sst {0}".format(sdlfile)
        rtn = OSCommand(oscmd, outfile).run()
        self.assertFalse(rtn.timeout(), "SST Timed-Out while running {0}".format(oscmd))
        self.assertEqual(rtn.result(), 0, "SST returned {0}; while running {1}".format(rtn.result(), oscmd))

        # Post processing of the output data to scrub it into a format to compare
        os.system("grep Random {0} > {1}".format(outfile, tmpfile))
        os.system("tail -5 {0} > {1}".format(tmpfile, cmpfile))

        # Perform the test
        testresult = filecmp.cmp(cmpfile, reffile)
        testerror = "Output/Compare file {0} does not match Reference File {1}".format(cmpfile, reffile)
        self.assertTrue(testresult, testerror)
