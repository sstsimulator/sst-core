# -*- coding: utf-8 -*-
#
# Copyright 2009-2023 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2023, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import os
import filecmp

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

class testcase_StatisticComponent(SSTTestCase):

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

    @unittest.skipIf(testing_check_get_num_ranks() > 1, "Test only supports single rank runs")
    def test_MemPool_overflow(self):
        self.Statistics_test_template("overflow", 4) # force 4 threads

    def test_MemPool_undeleted_items(self):
        self.Statistics_test_template("undeleted_items")

#####

    def Statistics_test_template(self, testtype, num_threads = None):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        sdlfile = "{0}/test_MemPool_{1}.py".format(testsuitedir, testtype)
        reffile = "{0}/refFiles/test_MemPool_{1}.out".format(testsuitedir, testtype)
        outfile = "{0}/test_MemPool_{1}.out".format(outdir, testtype)

        self.run_sst(sdlfile, outfile, num_threads=num_threads)

        # Perform the test
        filter1 = StartsWithFilter("WARNING: No components are")
        filter2 = StartsWithFilter("#")
        cmp_result = testing_compare_filtered_diff(testtype, outfile, reffile, True, [filter1, filter2])
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))
