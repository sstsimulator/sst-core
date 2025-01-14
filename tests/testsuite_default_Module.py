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

import os
import filecmp

from sst_unittest import *
from sst_unittest_support import *


class testcase_Module(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    num_threads = test_engine_globals.TESTENGINE_SSTRUN_NUMTHREADS

    def test_Module(self):
        self.Module_test_template("Module")

#####

    def Module_test_template(self, testtype):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        sdlfile = "{0}/test_Module.py".format(testsuitedir)
        reffile = "{0}/refFiles/test_Module.out".format(testsuitedir, testtype)
        outfile = "{0}/test_Module.out".format(outdir, testtype)

        # Perform the test
        self.run_sst(sdlfile, outfile)

        filter1 = StartsWithFilter("WARNING: No components are") # Appears in multi-thread/rank runs since we just have one component, filter it out
        cmp_result = testing_compare_filtered_diff(testtype, outfile, reffile, True, [filter1])
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

