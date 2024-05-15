# -*- coding: utf-8 -*-
#
# Copyright 2009-2024 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2024, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

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

class testcase_Component(SSTTestCase):

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

    def test_Component(self):
        self.component_test_template("Component")

    def test_Component_time_overflow(self):
        self.component_test_template("Component_time_overflow", 1)

#####

    def component_test_template(self, testtype, exp_rc = 0):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        sdlfile = "{0}/test_{1}.py".format(testsuitedir, testtype)
        reffile = "{0}/refFiles/test_{1}.out".format(testsuitedir, testtype)
        outfile = "{0}/test_{1}.out".format(outdir, testtype)
        errfile = "{0}/test_{1}.err".format(outdir, testtype)

        self.run_sst(sdlfile, outfile, errfile, expected_rc = exp_rc)

        # Check the results if exp_rc isn't equal to 0, then we are
        # expecting an error and we'll put in a LineFilter to filter
        # out everything after the error message before doing the diff
        filter1 = LineFilter()
        filter2 = LineFilter()
        if exp_rc != 0:
            filter1 = IgnoreAllAfterFilter("FATAL:", True)
            filter2 = RemoveRegexFromLineFilter(r"\[[0-9]+:[0-9]+\]")

        testfile = outfile
        if exp_rc != 0:
            testfile = errfile

        cmp_result = testing_compare_filtered_diff(testtype, testfile, reffile, sort=True, filters=[filter1,filter2])
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

