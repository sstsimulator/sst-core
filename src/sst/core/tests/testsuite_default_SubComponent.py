# -*- coding: utf-8 -*-

import os
import sys

from sst_unittest import *
from sst_unittest_support import *

################################################################################
# Code to support a single instance module initialize, must be called setUp method  

module_init = 0
module_sema = threading.Semaphore()

def initializeTestModule_SingleInstance(class_inst):
    global module_init
    global module_sema
    
    module_sema.acquire()
    if module_init != 1:
        # Put your single instance Init Code Here
        module_init = 1
    module_sema.release()
    
################################################################################

class testcase_SubComponent(SSTTestCase):

    def initializeClass(self, testName):
        super(type(self), self).initializeClass(testName)
        # Put test based setup code here. it is called before testing starts
        # NOTE: This method is called once for every test
        
    def setUp(self):
        super(type(self), self).setUp()
        initializeTestModule_SingleInstance(self)
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####
    rankerr = "Test only suports ranks <= 2"

    @unittest.skipIf(get_testing_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_2a(self):
        self.subcomponent_test_template("sc_2a")

    @unittest.skipIf(get_testing_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_2u2u(self):
        self.subcomponent_test_template("sc_2u2u")

    @unittest.skipIf(get_testing_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_2u(self):
        self.subcomponent_test_template("sc_2u")

    @unittest.skipIf(get_testing_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_a(self):
        self.subcomponent_test_template("sc_a")

    @unittest.skipIf(get_testing_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_u2u(self):
        self.subcomponent_test_template("sc_u2u")

    @unittest.skipIf(get_testing_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_u(self):
        self.subcomponent_test_template("sc_u")

    @unittest.skipIf(get_testing_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_2u2a(self):
        self.subcomponent_test_template("sc_2u2a")

    @unittest.skipIf(get_testing_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_2ua(self):
        self.subcomponent_test_template("sc_2ua")

    @unittest.skipIf(get_testing_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_2uu(self):
        self.subcomponent_test_template("sc_2uu")

    @unittest.skipIf(get_testing_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_u2a(self):
        self.subcomponent_test_template("sc_u2a")

    @unittest.skipIf(get_testing_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_ua(self):
        self.subcomponent_test_template("sc_ua")

    @unittest.skipIf(get_testing_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_uu(self):
        self.subcomponent_test_template("sc_uu")

#####

    def subcomponent_test_template(self, testtype):
        testsuitedir = self.get_testsuite_dir()
        outdir = get_test_output_run_dir()

        # Set the various file paths
        sdlfile = "{0}/subcomponent_tests/test_{1}.py".format(testsuitedir, testtype)
        reffile = "{0}/subcomponent_tests/refFiles/test_{1}.out".format(testsuitedir, testtype)
        outfile = "{0}/test_SubComponent_{1}.out".format(outdir, testtype)

        self.run_sst(sdlfile, outfile)

        # Perform the test
        cmp_result = compare_sorted(testtype, outfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

