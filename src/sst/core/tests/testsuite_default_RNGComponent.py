# -*- coding: utf-8 -*-

import os
import filecmp

import sst_unittest_support
from sst_unittest_support import *

################################################################################

def setUpModule():
    sst_unittest_support.setUpModule()
    # Put Module based setup code here. it is called before any testcases are run

def tearDownModule():
    # Put Module based teardown code here. it is called after all testcases are run
    sst_unittest_support.tearDownModule()

################################################################################

class testcase_RNGComponent(SSTTestCase):

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

    def test_RNG_Mersenne(self):
        self.RNG_test_template("mersenne")

    def test_RNG_Marsaglia(self):
        self.RNG_test_template("marsaglia")

    def test_RNG_xorshift(self):
        self.RNG_test_template("xorshift")

#####

    def RNG_test_template(self, testcase):
        # Set the various file paths
        parent_module_path = os.path.dirname(__file__)
        testsuitedir = parent_module_path

#        sdlfile = "{0}/test_RNGComponent_{1}.py".format(get_testsuite_dir(), testcase)
#        reffile = "{0}/refFiles/test_RNGComponent_{1}.out".format(get_testsuite_dir(), testcase)
        sdlfile = "{0}/test_RNGComponent_{1}.py".format(testsuitedir, testcase)
        reffile = "{0}/refFiles/test_RNGComponent_{1}.out".format(testsuitedir, testcase)
        outfile = "{0}/test_RNGComponent_{1}.out".format(get_test_output_run_dir(), testcase)
        tmpfile = "{0}/test_RNGComponent_{1}.tmp".format(get_test_output_tmp_dir(), testcase)
        cmpfile = "{0}/test_RNGComponent_{1}.cmp".format(get_test_output_tmp_dir(), testcase)

        self.run_sst(sdlfile, outfile)

        # Post processing of the output data to scrub it into a format to compare
        os.system("grep Random {0} > {1}".format(outfile, tmpfile))
        os.system("tail -5 {0} > {1}".format(tmpfile, cmpfile))

        # Perform the test
        testresult = filecmp.cmp(cmpfile, reffile)
        testerror = "Output/Compare file {0} does not match Reference File {1}".format(cmpfile, reffile)
        self.assertTrue(testresult, testerror)