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

import json
import os
from sst_unittest import *
from sst_unittest_support import *


class testcase_TimingInfo(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_TimingInfo(self):
        self.timing_info_test_template("JSONTimingInfo")

#####

    def timing_info_test_template(self, testtype):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        sdlfile = "{0}/test_PerfComponent.py".format(testsuitedir)
        reffile = "{0}/refFiles/test_TimingInfo.out".format(testsuitedir)
        outfile = "{0}/test_TimingInfo.out".format(outdir)
        jsonfile = "{0}/test_TimingInfo.json".format(outdir)

        self.run_sst(sdlfile, outfile, other_args=f"--timing-info-json={jsonfile}")
        
        # Perform the test of the standard simulation output
        cmp_result = testing_compare_sorted_diff(testtype, outfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

        # Verify well formed JSON and navigate Key-Value pairs
        jsonDict = {}
        try:
            with open(jsonfile) as f:
                jsonDict = json.load(f)
        except FileNotFoundError:
            self.assertFalse(True, f"Could not load json timing info file {jsonfile}")

        timingInfo = jsonDict['timing-info']

        timing_keys = [
            "local_max_rss",
            "global_max_rss",
            "local_max_pf",
            "global_pf",
            "global_max_io_in",
            "global_max_io_out",
            "global_max_sync_data_size",
            "global_sync_data_size",
            "max_mempool_size",
            "global_mempool_size",
            "global_active_activities",
            "global_current_tv_depth",
            "global_max_tv_depth",
            "max_build_time",
            "max_run_time",
            "max_total_time",
            "simulated_time_ua",
            "ranks",
            "threads",
        ]
        
        # Check keys exists
        for k in timing_keys:
            self.assertTrue(k in timingInfo, f"JSON data is missing key: {k}")

        # Check no spurious keys
        for k in timingInfo.keys():
            self.assertTrue(k in timing_keys, f"JSON data contains an unknown key: {k}")


