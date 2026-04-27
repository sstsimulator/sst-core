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


class testcase_DebugConsole(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

###

    parallelism = testing_check_get_num_ranks() * testing_check_get_num_threads()

    # Serial
    #@unittest.skipIf(parallelism > 16, "Test does not support greater than 16-way parallelism")
    @unittest.skipIf(testing_check_get_num_ranks() > 1, "Test only supports serial execution")
    @unittest.skipIf(testing_check_get_num_threads() > 1, "Test only supports serial execution")
    def test_serial0(self):
        self.debugconsole_test_template("serial0", "0")

    #@unittest.skipIf(parallelism > 16, "Test does not support greater than 16-way parallelism")
    @unittest.skipIf(testing_check_get_num_ranks() > 1, "Test only supports serial execution")
    @unittest.skipIf(testing_check_get_num_threads() > 1, "Test only supports serial execution")
    def test_serial1(self):
        self.debugconsole_test_template("serial1", "1ps")

    # Multithreaded - 4 threads
    #@unittest.skipIf(parallelism > 16, "Test does not support greater than 16-way parallelism")
    @unittest.skipIf(testing_check_get_num_ranks() > 1, "Test requires single rank with 4 threads")
    @unittest.skipIf(testing_check_get_num_threads() != 4, "Test requires single rank with 4 threads")
    def test_rank_thread0(self):
        self.debugconsole_test_template("thread0", "0")

    #@unittest.skipIf(parallelism > 16, "Test does not support greater than 16-way parallelism")
    @unittest.skipIf(testing_check_get_num_ranks() > 1, "Test requires single rank with 4 threads")
    @unittest.skipIf(testing_check_get_num_threads() != 4, "Test requires single rank with 4 threads")
    def test_rank_thread1(self):
        self.debugconsole_test_template("thread1", "1ps")

    # Rank Serial - 2 ranks, each with 1 thread
    #@unittest.skipIf(parallelism > 16, "Test does not support greater than 16-way parallelism")
    @unittest.skipIf(testing_check_get_num_ranks() != 2, "Test requires 2 ranks, each with 1 thread")
    @unittest.skipIf(testing_check_get_num_threads() > 1, "Test requires 2 ranks, each with 1 thread")
    def test_r2_rankserial0(self):
        self.debugconsole_test_template("r2_rankserial0", "0")

    #@unittest.skipIf(parallelism > 16, "Test does not support greater than 16-way parallelism")
    @unittest.skipIf(testing_check_get_num_ranks() != 2, "Test requires 2 ranks, each with 1 thread")
    @unittest.skipIf(testing_check_get_num_threads() > 1, "Test requires 2 ranks, each with 1 thread")
    def test_r2_rankserial1(self):
        self.debugconsole_test_template("r2_rankserial1", "1ps")

    # Rank Serial - 4 ranks, each with 1 thread
    #@unittest.skipIf(parallelism > 16, "Test does not support greater than 16-way parallelism")
    @unittest.skipIf(testing_check_get_num_ranks() != 4, "Test requires 4 ranks, each with 1 thread")
    @unittest.skipIf(testing_check_get_num_threads() > 1, "Test requires 4 ranks, each with 1 thread")
    def test_r4_rankserial0(self):
        self.debugconsole_test_template("r4_rankserial0", "0")

    #@unittest.skipIf(parallelism > 16, "Test does not support greater than 16-way parallelism")
    @unittest.skipIf(testing_check_get_num_ranks() != 4, "Test requires 4 ranks, each with 1 thread")
    @unittest.skipIf(testing_check_get_num_threads() > 1, "Test requires 4 ranks, each with 1 thread")
    def test_r4_rankserial1(self):
        self.debugconsole_test_template("r4_rankserial1", "1ps")

    # Rank Parallel - 2 ranks, each with 2 threads
    #@unittest.skipIf(parallelism > 16, "Test does not support greater than 16-way parallelism")
    @unittest.skipIf(testing_check_get_num_ranks() != 2, "Test requires 2 ranks, each with 2 threads")
    @unittest.skipIf(testing_check_get_num_threads() != 2, "Test requires 2 ranks, each with 2 threads")
    def test_rankparallel0(self):
        self.debugconsole_test_template("rankparallel0", "0")

    #@unittest.skipIf(parallelism > 16, "Test does not support greater than 16-way parallelism")
    @unittest.skipIf(testing_check_get_num_ranks() != 2, "Test requires 2 ranks, each with 2 threads")
    @unittest.skipIf(testing_check_get_num_threads() != 2, "Test requires 2 ranks, each with 2 threads")
    def test_rankparallel1(self):
        self.debugconsole_test_template("rankparallel1", "1ps")

#####

    def debugconsole_test_template(self, testtype, starttime):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        cmdfile = "{0}/cmd_DebugConsole_{1}.cmd".format(testsuitedir, testtype)
        options = "--interactive-start={1} --replay-file={0} --model-options=\"4 4\"".format(cmdfile, starttime);

        # Set the various file paths
        sdlfile = "{0}/test_MessageMesh.py".format(testsuitedir)
        reffile = "{0}/refFiles/test_DebugConsole_{1}.out".format(testsuitedir, testtype)
        outfile = "{0}/test_DebugConsole_{1}.out".format(outdir, testtype)
        checkfile = "{0}/testsuite_DebugConsole/debug_{1}.txt".format(outdir, testtype)

        # Run the test
        self.run_sst(sdlfile, outfile, other_args=options)

        # Remove replay file path which will vary
        filter1 = LineFilter();
        filter1 = RemoveRegexFromLineFilter(r"replay.*")
        # Filter out printstatus queue order due to differences across architectures
        filter2 = LineFilter();
        filter2 = RemoveRegexFromLineFilter(r"queue order:.*")

        # Perform the test comparison with refFile
        #cmp_result = testing_compare_sorted_diff(testtype, outfile, reffile)
        cmp_result = testing_compare_filtered_diff(testtype, outfile, reffile, sort=True, filters=[filter1, filter2])
        if not cmp_result:
            diffdata = testing_get_diff_data(testtype)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(checkfile, reffile))
