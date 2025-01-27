# -*- coding: utf-8 -*-
#
# Copyright 2009-2025 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2025, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

from sst_unittest import *
from sst_unittest_support import *


have_curses = sst_core_config_include_file_get_value(define="HAVE_CURSES", type=int, default=0, disable_warning=True) == 1


class testcase_sstinfo(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_sstinfo_coretestelement(self):
        self.sstinfo_test_template("coreTestElement", "-q")

    @unittest.skipIf(not have_curses, "Curses library not loaded, skipping interactive test")
    def test_sstinfo_interactive(self):
        self.sstinfo_test_template("interactive", "-i")
        
#####

    def sstinfo_test_template(self, testtype, flags):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        outfile = "{0}/test_sstinfo_{1}.out".format(outdir, testtype)
        errfile = "{0}/test_sstinfo_{1}.err".format(outdir, testtype)

        # Get the path to sst-info binary application
        sst_app_path = sstsimulator_conf_get_value(section='SSTCore', key='bindir', type=str, default="UNDEFINED")
        err_str = "Path to SST-INFO {0}; does not exist...".format(sst_app_path)
        self.assertTrue(os.path.isdir(sst_app_path), err_str)

        cmd = '{0}/sst-info coreTestElement {1}'.format(sst_app_path, flags)
        rtn = OSCommand(cmd, output_file_path = outfile, error_file_path = errfile).run(timeout_sec = 1)

        if testtype == "coreTestElement":
            if rtn.result() != 0:
                self.assertEqual(rtn.result(), 0, "sst-info Test failed running cmdline {0} - return = {1}".format(cmd, rtn.result()))
                with open(outfile, 'r') as f:
                    log_failure("FAILURE: sst-info cmdline {0}; output =\n{1}".format(cmd, f.read()))
        elif testtype == "interactive":
            if not rtn.timeout():
                self.assertEqual(rtn.timeout(), False, "sst-info Test failed running cmdline {0} - timeout failed".format(cmd))
                with open(outfile, 'r') as f:
                    log_failure("FAILURE: sst-info cmdline {0}; output =\n{1}".format(cmd, f.read()))

        err_file_not_empty = os_test_file(errfile, expression='-s')
        if err_file_not_empty:
            self.assertFalse(err_file_not_empty, f"sst-info Test failed because the error file is not empty: {errfile}")
            with open(errfile, 'r') as f:
                log_failure("FAILURE: sst-info cmdline {0}; error output =\n{1}".format(cmd, f.read()))
