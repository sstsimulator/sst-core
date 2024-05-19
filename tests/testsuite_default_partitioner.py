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

class testcase_Partitioners(SSTTestCase):

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

###


    def test_linear(self):
        self.partitioner_test_template("linear", "6 6", "sst.linear")

    def test_roundrobin(self):
        self.partitioner_test_template("roundrobin", "6 6", "sst.roundrobin")

    def test_simple(self):
        self.partitioner_test_template("simple", "6 6", "sst.simple")

#####

    def partitioner_test_template(self, testtype, model_options, partitioner):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        options = "--model-options=\"{0}\" --partitioner={1}".format(model_options, partitioner);
        
        # Set the various file paths
        sdlfile = "{0}/test_MessageMesh.py".format(testsuitedir)
        outfile_ref = "{0}/test_partitioner_ref_{1}.out".format(outdir, testtype)
        outfile_check = "{0}/test_partitioner_check_{1}.out".format(outdir, testtype)

        # Do a serial reference run
        self.run_sst(sdlfile, outfile_ref, other_args=options, num_ranks=1, num_threads=1)
        self.run_sst(sdlfile, outfile_check, other_args=options)

        # Perform the test
        cmp_result = testing_compare_sorted_diff(testtype, outfile_ref, outfile_check)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile_ref, outfile_check))
