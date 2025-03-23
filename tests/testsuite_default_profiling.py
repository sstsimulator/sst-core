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
import sys

from sst_unittest import *
from sst_unittest_support import *


class testcase_Profiling(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

###

    parallelerr = "Test only suports serial execution"

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_event_global(self):
        self.profiling_test_template("event_global", "events:sst.profile.handler.event.count(level=global)[event]")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_event_component(self):
        self.profiling_test_template("event_component", "events:sst.profile.handler.event.count(level=component)[event]")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_event_subcomponent(self):
        self.profiling_test_template("event_subcomponent", "events:sst.profile.handler.event.count(level=subcomponent)[event]")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_event_type(self):
        self.profiling_test_template("event_type", "events:sst.profile.handler.event.count(level=type)[event]")


#####

    def profiling_test_template(self, testtype, profile_options):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        options = "--output-directory=testsuite_profiling --profiling-output=prof_{0}.out --model-options=\"4 4\" --enable-profiling=\"{1}\"".format(testtype,profile_options);
        
        # Set the various file paths
        sdlfile = "{0}/test_MessageMesh.py".format(testsuitedir)
        reffile = "{0}/refFiles/test_Profiling_{1}.out".format(testsuitedir, testtype)
        outfile = "{0}/test_Profiling_{1}.out".format(outdir, testtype)
        checkfile = "{0}/testsuite_profiling/prof_{1}.out".format(outdir, testtype)

        self.run_sst(sdlfile, outfile, other_args=options)

        # Perform the test
        cmp_result = testing_compare_sorted_diff(testtype, checkfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))
