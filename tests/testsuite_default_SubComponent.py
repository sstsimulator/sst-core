# -*- coding: utf-8 -*-
#
# Copyright 2009-2022 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2022, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

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

    @unittest.skipIf(testing_check_get_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_2a(self):
        self.subcomponent_test_template("sc_2a")

    @unittest.skipIf(testing_check_get_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_2u2u(self):
        self.subcomponent_test_template("sc_2u2u")

    @unittest.skipIf(testing_check_get_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_2u(self):
        self.subcomponent_test_template("sc_2u")

    @unittest.skipIf(testing_check_get_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_a(self):
        self.subcomponent_test_template("sc_a")

    @unittest.skipIf(testing_check_get_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_u2u(self):
        self.subcomponent_test_template("sc_u2u")

    @unittest.skipIf(testing_check_get_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_u(self):
        self.subcomponent_test_template("sc_u")

    @unittest.skipIf(testing_check_get_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_2u2a(self):
        self.subcomponent_test_template("sc_2u2a")

    @unittest.skipIf(testing_check_get_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_2ua(self):
        self.subcomponent_test_template("sc_2ua")

    @unittest.skipIf(testing_check_get_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_2uu(self):
        self.subcomponent_test_template("sc_2uu")

    @unittest.skipIf(testing_check_get_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_u2a(self):
        self.subcomponent_test_template("sc_u2a")

    @unittest.skipIf(testing_check_get_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_ua(self):
        self.subcomponent_test_template("sc_ua")

    @unittest.skipIf(testing_check_get_num_ranks() > 2, rankerr)
    def test_SubComponent_sc_uu(self):
        self.subcomponent_test_template("sc_uu")

#####

    def subcomponent_test_template(self, testtype):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        # Set the various file paths
        sdlfile = "{0}/subcomponent_tests/test_{1}.py".format(testsuitedir, testtype)
        reffile = "{0}/subcomponent_tests/refFiles/test_{1}.out".format(testsuitedir, testtype)
        outfile = "{0}/test_SubComponent_{1}.out".format(outdir, testtype)

        self.run_sst(sdlfile, outfile)

        # Perform the test
        filter1 = StartsWithFilter("WARNING: No components are")
        cmp_result = testing_compare_filtered_diff(testtype, outfile, reffile, sort=True, filters=[filter1])
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

