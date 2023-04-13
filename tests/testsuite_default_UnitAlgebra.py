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

class testcase_UnitAlgebra(SSTTestCase):

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

    def test_UnitAlgebra(self):
        self.unitalgebra_test_template("UnitAlgebra")
    
    def test_PythonUnitAlgebra(self):
        self.unitalgebra_test_template("PythonUnitAlgebra")

#####

    def unitalgebra_test_template(self, testtype):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        sdlfile = "{0}/test_{1}.py".format(testsuitedir, testtype)
        reffile = "{0}/refFiles/test_{1}.out".format(testsuitedir, testtype)
        outfile = "{0}/test_{1}.out".format(outdir, testtype)

        self.run_sst(sdlfile, outfile)

        testing_remove_component_warning_from_file(outfile)

        # Perform the test
        cmp_result = testing_compare_sorted_diff(testtype, outfile, reffile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testtype)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))
