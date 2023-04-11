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

class testcase_sstinfo(SSTTestCase):

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

    def test_sstinfo_coretestelement(self):
        self.sstinfo_test_template("coreTestElement")

#####

    def sstinfo_test_template(self, testtype):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        outfile = "{0}/test_sstinfo_{1}.out".format(outdir, testtype)
        errfile = "{0}/test_sstinfo_{1}.err".format(outdir, testtype)

        # Get the path to sst-info binary application
        sst_app_path = sstsimulator_conf_get_value_str('SSTCore', 'bindir', default="UNDEFINED")
        err_str = "Path to SST-INFO {0}; does not exist...".format(sst_app_path)
        self.assertTrue(os.path.isdir(sst_app_path), err_str)

        cmd = '{0}/sst-info -q {1}'.format(sst_app_path, testtype)
        rtn = OSCommand(cmd, output_file_path = outfile, error_file_path = errfile).run()
        if rtn.result() != 0:
            self.assertEquals(rtn.result(), 0, "sst-info Test failed running cmdline {0} - return = {1}".format(cmd, rtn.result()))
            with open(outfile, 'r') as f:
                log_failure("FAILURE: sst-info cmdline {0}; output =\n{1}".format(cmd, f.read()))

        err_file_not_empty = os_test_file(errfile, expression='-s')
        if err_file_not_empty:
            self.assertFalse(err_file_not_empty, "sst-info Test failed because the error file is not empty".format(cmd, rtn.result()))
            with open(errfile, 'r') as f:
                log_failure("FAILURE: sst-info cmdline {0}; error output =\n{1}".format(cmd, f.read()))
