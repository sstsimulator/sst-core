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

class testcase_Links(SSTTestCase):

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

    def test_Links(self):
        self.component_test_template("basic")

    def test_Links_dangling(self):
        self.component_test_template("dangling", "--model-options=dangling", 1)

    def test_Links_wrong_port(self):
        self.component_test_template("wrong_port", "--model-options=wrong_port", 1)

#####

    def component_test_template(self, testtype, extra_args="", rc=0):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        sdlfile = "{0}/test_Links.py".format(testsuitedir)
        reffile = "{0}/refFiles/test_Links_{1}.out".format(testsuitedir,testtype)
        outfile = "{0}/test_Links_{1}.out".format(outdir,testtype)

        if rc == 0: self.run_sst(sdlfile, outfile, other_args=extra_args, expected_rc=rc)
        else:
            errfile = "{0}/test_Links_{1}.err".format(outdir,testtype)
            self.run_sst(sdlfile, outfile, errfile, other_args=extra_args, expected_rc=rc)

        # Perform the test
        filter1 = StartsWithFilter("WARNING: No components are")
        filter2 = IgnoreAllAfterFilter("SST Fatal")
        filters = [filter1, filter2]
        # Do fitered diff.  Sort only when we are expecting success
        if rc == 0:
            cmpfile = outfile
            cmp_result = testing_compare_filtered_diff("Links_{0}".format(testtype), outfile, reffile, rc == 0, filters)
        else:
            cmpfile = errfile
            filters.append(RemoveRegexFromLineFilter("FATAL: (\[[0-9]+:[0-9]+\])* "))
            cmp_result = testing_compare_filtered_diff("Links_{0}".format(testtype), errfile, reffile, rc == 0, filters)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(cmpfile, reffile))

