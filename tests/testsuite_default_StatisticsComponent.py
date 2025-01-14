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
import subprocess

from sst_unittest import *
from sst_unittest_support import *


have_h5 = sst_core_config_include_file_get_value(define="HAVE_HDF5", type=int, default=0, disable_warning=True) == 1


class testcase_StatisticComponent(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    num_threads = test_engine_globals.TESTENGINE_SSTRUN_NUMTHREADS

    #@unittest.skipIf(num_threads > 1, "Statistic test currently fails with threads due to strange interactions of independent threads with stats")
    def test_StatisticsBasic(self):
        self.Statistics_test_template("basic")

#####

    def Statistics_test_template(self, testtype):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        sdlfile = "{0}/test_StatisticsComponent_{1}.py".format(testsuitedir, testtype)
        reffile = "{0}/refFiles/test_StatisticsComponent_{1}.out".format(testsuitedir, testtype)
        outfile = "{0}/test_StatisticsComponent_{1}.out".format(outdir, testtype)
        ref_group_stat_file_csv = "{0}/refFiles/test_StatisticsComponent_{1}_group_stats.csv".format(testsuitedir, testtype)
        out_group_stat_file_csv = "{0}/test_StatisticsComponent_{1}_group_stats.csv".format(outdir, testtype)
        ref_group_stat_file_txt = "{0}/refFiles/test_StatisticsComponent_{1}_group_stats.txt".format(testsuitedir, testtype)
        out_group_stat_file_txt = "{0}/test_StatisticsComponent_{1}_group_stats.txt".format(outdir, testtype)

        # Enable HDF5 test if available
        options = ""
        if have_h5:
            options = "--model-options=\"hdf5\""
            ref_group_stat_file_h5 = "{0}/refFiles/test_StatisticsComponent_{1}_group_stats.h5".format(testsuitedir, testtype)
            out_group_stat_file_h5 = "{0}/test_StatisticsComponent_{1}_group_stats.h5".format(outdir, testtype)
            reffile = "{0}/refFiles/test_StatisticsComponent_{1}_h5.out".format(testsuitedir, testtype)
        
        # Perform the test
        self.run_sst(sdlfile, outfile, other_args=options)

        # Combine the stat output files into a single file
        combine_per_rank_files(out_group_stat_file_txt)
        # Need to skip header after the first file
        combine_per_rank_files(out_group_stat_file_csv)

        filter1 = StartsWithFilter("WARNING: No components are")
        cmp_result = testing_compare_filtered_diff(testtype, outfile, reffile, True, [filter1])
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

        filter2 = StartsWithFilter("ComponentName, StatisticName,")
        cmp_result = testing_compare_filtered_diff(testtype, out_group_stat_file_csv, ref_group_stat_file_csv, True, [filter2])
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(out_group_stat_file_csv, ref_group_stat_file_csv))

        cmp_result = testing_compare_filtered_diff(testtype, out_group_stat_file_txt, ref_group_stat_file_txt, True)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(out_group_stat_file_txt, ref_group_stat_file_txt))

        # Generate raw H5 output
        if have_h5:
            try:
                subprocess.run(["h5diff",ref_group_stat_file_h5,out_group_stat_file_h5],check=True)
            except subprocess.CalledProcessError as h5exc:
                self.assertTrue(False, "Output/Compare file {0} does not match Reference File {1}. H5diff returned {2} {3}".format(ref_group_stat_file_h5, out_group_stat_file_h5,h5exc.returncode, h5exc.output))

