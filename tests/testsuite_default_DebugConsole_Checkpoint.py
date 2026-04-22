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

have_mpi = sst_core_config_include_file_get_value(define="SST_CONFIG_HAVE_MPI", type=int, default=0, disable_warning=True) == 1

class testcase_Checkpoint(SSTTestCase):

    def setUp(self) -> None:
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self) -> None:
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####
    parallelerr = "Test only supports serial execution"

    ## All tests will run from a checkpoint of a normal run and a
    ## checkpoint of a restarted run. Different tests will restart
    ## from the first and second checkpoints on both restart types to
    ## make sure that checkpoints after the first work correctly.

    # Serial tests
    @unittest.skipIf(testing_check_get_num_ranks() > 1, "Test only supports serial execution")
    @unittest.skipIf(testing_check_get_num_threads() > 1, "Test only supports serial execution")
    def test_DebugConsole_Checkpoint_serial0(self) -> None:
        self.debugconsole_checkpoint_test_template("DebugConsole_Checkpoint_serial0", 1, 1, istart="0")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, "Test only supports serial execution")
    @unittest.skipIf(testing_check_get_num_threads() > 1, "Test only supports serial execution")
    def test_DebugConsole_Checkpoint_serial1(self) -> None:
        self.debugconsole_checkpoint_test_template("DebugConsole_Checkpoint_serial1", 1, 1, istart="1us")



#####
    # This function will run sst 3 times.  In the absence of other options being set, all three runs will use the
    # parallelism set on the command line to the main test script.  The the runs are described here, along with
    # how the other run options affect them:
    #
    # 1. Normal start with checkpointing turned on
    #    - start_serial: this causes this run to execute serially (1 rank, 1 thread)
    #
    # 2. This run uses the checkpoint files from the first run to restart the simulation.  This run also turns on
    #    checkpointing
    #    - n_to_one: This will cause the restart to be serial (1 rank, 1 thread)
    #    - swap_rank_thread: This will cause the restart to swap the number of ranks and threads used
    #    - restart_smaller: This will cause the larger of ranks or threads to restart one smaller than the specified
    #      parallelism (zero will round up to 1). If rank and thread count are the same, it will reduce rank count
    #    NOTE: swap_rank_thread and restart_smaller can be combined. If both are set restart_smaller will resolve first
    #
    # 3. The final run restarts from the checkpoint files of the restarted run above.  This run will always be at
    #    the parallelism specified in the test script run parameters. This run can be skipped by setting cr_index = 0
    #
    # File naming:
    #
    # Names of files and directories needs to be managed to ensure that tests are using the correct files. In particular,
    # we need to ensure that directories used for checkpoint data are unique or tests may pull files from earlier tests
    # instead of for the current test.  The names of the files used are controlled by three parameters:
    #
    # testtype: This is the main variable and sets the base name for input, output, and ref files, as well as checkpoint
    #    directories. The filenames based on test type are:
    #      input file:     test_<testtype>.py
    #      output file:    test_<testtype>.out
    #      reference file: test_<testtype>.out
    #      original checkpoint directory: <testtype>_cpt
    #      restart checkpoint directory:  <testtype>_rst
    #
    # out_suffix: This is used when the output file has an added suffix compared to the input file.  This allows the same
    #    input to be used, but have different reference output.  This will add a suffix to the following:
    #
    #      output file:    test_<testtype><out_suffix>.out
    #      reference file: test_<testtype><out_suffix>.out
    #      original checkpoint directory: <testtype><outsuffix>_cpt
    #      restart checkpoint directory:  <testtype><outsuffix>_rst
    #
    # cpt_suffix: This is used to add an additional suffix to the checkpoint directories for tests that use the same
    #    input and output files, but have tests for multiple levels of parallelism.  These will add a suffix to the following:
    #
    #      output file:    test_<testtype><out_suffix><cpt_suffix>.out
    #      reference file: test_<testtype><out_suffix>.out (cpt_suffix not used for reference file)
    #      original checkpoint directory: <testtype><outsuffix><cpt_suffix>_cpt
    #      restart checkpoint directory:  <testtype><outsuffix><cpt_suffix>_rst
    #
    # Other parameters:
    # rst_index: checkpoint index to restart from on first restart
    # cr_index: checkpoint index to restart from when restarting from a checkpointed restart run
    # subcom: Set to True if this is a subcomponent test as the file/path name differs
    # modelparams: Set what is passed into --model-params when running SST
    def debugconsole_checkpoint_test_template(self, testtype: str, rst_index: int = 1, cr_index: int = 0, subcomp: bool = False, modelparams: str = "", out_suffix: str = "", cpt_suffix : str = "", n_to_one: bool = False, swap_rank_thread : bool = False, start_serial : bool = False, restart_smaller : bool = False, istart: str = "0") -> None:

        # name conventions:
        # X_cpt - files/options associated with first checkpoint run
        # X_rst - files/options assocated with restart run
        # X_cr  - files/options associated with restart of checkpointed restart run

        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        reffile = "{0}/refFiles/test_{1}{2}.out".format(testsuitedir, testtype, out_suffix)

        ## Get the checkpoint interval from the reference file. We use
        ## the same period for both the original checkpoint run and
        ## the checkpoint of a restart run
        checkpoint_period = "10 us"
        
        ## First run

        # Get the primary sdl file and output file
        sdlfile_cpt = "{0}/test_Checkpoint.py".format(testsuitedir)
        outfile_cpt = "{0}/test_{1}{2}{3}.out".format(outdir, testtype, out_suffix, cpt_suffix)
        reffile_rst = "{0}/refFiles/test_{1}{2}_restart.out".format(testsuitedir, testtype, out_suffix, cpt_suffix)

        # Get the checkpoint prefix
        prefix_cpt = "{0}{1}{2}_cpt".format(testtype, out_suffix, cpt_suffix)

        cmdfile = "{0}/cmd_{1}.cmd".format(testsuitedir, testtype)
        ckpt_outdir = "testsuite_debugconsole_checkpoint"
        options_checkpoint_cpt = (
            "--interactive-start={4} --replay-file='{0}' --checkpoint-enable "
            "--checkpoint-prefix={1} "
            "--checkpoint-name-format='%p_%n' --output-directory={2} "
            "--model-options='{3}'".format(cmdfile, prefix_cpt, ckpt_outdir, modelparams, istart))

        if start_serial:
            # Need to run with 1 rank and 1 thread
            self.run_sst(sdlfile_cpt, outfile_cpt, other_args=options_checkpoint_cpt, num_ranks=1, num_threads=1)
        else:
            self.run_sst(sdlfile_cpt, outfile_cpt, other_args=options_checkpoint_cpt)

        # Check that original run got the right results
        #filters_cpt = [
        #    CheckpointInfoFilter(),
        #    StartsWithFilter("WARNING: No components are assigned") ]
        filter1 = LineFilter();
        filter1 = RemoveRegexFromLineFilter(r"replay.*")
        filter2 = LineFilter();
        filter2 = RemoveRegexFromLineFilter(r"last checkpoint.*")

        cmp_result = testing_compare_filtered_diff(testtype, outfile_cpt, reffile, True, filters=[filter1, filter2])
        if not cmp_result:
            diffdata = testing_get_diff_data(testtype)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Output from original checkpoint run {0} did not match reference file {1}".format(outfile_cpt, reffile))

        prefix_rst = "{0}{1}{2}_rst".format(testtype, out_suffix, cpt_suffix)

        ## Restart run ---------------------------------------------------------------------
        sdlfile_rst = "{0}/{3}/{1}/{1}_{2}/{1}_{2}.sstcpt".format(outdir, prefix_cpt, rst_index, ckpt_outdir)
        outfile_rst = "{0}/test_{1}{2}{3}_restart.out".format(outdir, testtype, out_suffix, cpt_suffix)
        options_rst = "--load-checkpoint"
        options_checkpoint_rst = ""
        

        # If we are checkpointing the restart run, need to add those options
        if cr_index > 0:
            options_checkpoint_rst = (
                " --checkpoint-sim-period='{0}' --checkpoint-prefix={1} "
                "--checkpoint-name-format='%p_%n' --output-directory={2} "
                .format(checkpoint_period, prefix_rst, ckpt_outdir))

        options_rst += options_checkpoint_rst

        # Get the threads and ranks for the job
        num_ranks = testing_check_get_num_ranks()
        num_threads = testing_check_get_num_threads()

        if restart_smaller:
            if num_ranks >= num_threads:
                num_ranks -= 1
                if num_ranks <= 0: num_ranks = 1
            else:
                num_threads -= 1
                if num_threads <= 0: num_threads = 1

        if swap_rank_thread:
            tmp = num_ranks
            num_ranks = num_threads
            num_threads = tmp

        if n_to_one:
            num_ranks = 1
            num_threads = 1
        self.run_sst(sdlfile_rst, outfile_rst, other_args=options_rst, num_ranks=num_ranks, num_threads=num_threads)

        # Check that restart output is a subset of checkpoint output
        #filters_rst = [
        #    CheckpointRefFileFilter(rst_index),
        #    CheckpointInfoFilter(),
        #    StartsWithFilter("WARNING: No components are assigned") ]
        filter2_rst = LineFilter();
        filter2_rst = RemoveRegexFromLineFilter(r"last checkpoint.*")
        filters_rst = [filter2_rst]

        cmp_result = testing_compare_filtered_diff(testtype, outfile_rst, reffile_rst, True, filters_rst)
        if not cmp_result:
            diffdata = testing_get_diff_data(testtype)
            log_failure(diffdata)
            print(diffdata)
        self.assertTrue(cmp_result, "Output/Compare file {0} from first restart does not match Reference File {1}".format(outfile_rst, reffile_rst))
