# -*- coding: utf-8 -*-

import os
import filecmp

from sst_unittest import *
from sst_unittest_support import *

################################################################################

class testcase_RNGComponent(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_RNG_Mersenne(self):
        self.RNG_test_template("mersenne")

    def test_RNG_Marsaglia(self):
        self.RNG_test_template("marsaglia")

    def test_RNG_xorshift(self):
        self.RNG_test_template("xorshift")

#####

    def RNG_test_template(self, testtype):
        testsuitedir = self.get_testsuite_dir()
        outdir = get_test_output_run_dir()

        sdlfile = "{0}/test_RNGComponent_{1}.py".format(testsuitedir, testtype)
        reffile = "{0}/refFiles/test_RNGComponent_{1}.out".format(testsuitedir, testtype)
        outfile = "{0}/test_RNGComponent_{1}.out".format(outdir, testtype)
        tmpfile = "{0}/test_RNGComponent_{1}.tmp".format(outdir, testtype)
        cmpfile = "{0}/test_RNGComponent_{1}.cmp".format(outdir, testtype)

        self.run_sst(sdlfile, outfile)

        # Post processing of the output data to scrub it into a format to compare
        os.system("grep Random {0} > {1}".format(outfile, tmpfile))
        os.system("tail -5 {0} > {1}".format(tmpfile, cmpfile))

        # Perform the test
        testresult = filecmp.cmp(cmpfile, reffile)
        testerror = "Output/Compare file {0} does not match Reference File {1}".format(cmpfile, reffile)
        self.assertTrue(testresult, testerror)
