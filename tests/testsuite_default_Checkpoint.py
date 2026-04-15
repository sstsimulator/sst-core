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

    def test_Checkpoint(self) -> None:
        self.checkpoint_test_template("Checkpoint", 1, 1)

    def test_Checkpoint_sc_2a(self) -> None:
        self.checkpoint_test_template("sc_2a", 2, 1, subcomp=True, modelparams="1")

    def test_Checkpoint_sc_2u(self) -> None:
        self.checkpoint_test_template("sc_2u", 2, 2, subcomp=True, modelparams="1")

    def test_Checkpoint_sc_a(self) -> None:
        self.checkpoint_test_template("sc_a", 1, 1, subcomp=True, modelparams="1")

    def test_Checkpoint_sc_u2u(self) -> None:
        self.checkpoint_test_template("sc_u2u", 2, 1, subcomp=True, modelparams="1")

    def test_Checkpoint_sc_u(self) -> None:
        self.checkpoint_test_template("sc_u", 1, 1, subcomp=True, modelparams="1")

    def test_Checkpoint_sc_2u2a(self) -> None:
        self.checkpoint_test_template("sc_2u2a", 2, 2, subcomp=True, modelparams="1")

    def test_Checkpoint_sc_2ua(self) -> None:
        self.checkpoint_test_template("sc_2ua", 1, 1, subcomp=True, modelparams="1")

    def test_Checkpoint_sc_2uu(self) -> None:
        self.checkpoint_test_template("sc_2uu", 2, 1, subcomp=True, modelparams="1")

    def test_Checkpoint_sc_u2a(self) -> None:
        self.checkpoint_test_template("sc_u2a", 1, 2, subcomp=True, modelparams="1")

    def test_Checkpoint_sc_ua(self) -> None:
        self.checkpoint_test_template("sc_ua", 2, 2, subcomp=True, modelparams="1")

    def test_Checkpoint_sc_uu(self) -> None:
        self.checkpoint_test_template("sc_uu", 1, 1, subcomp=True, modelparams="1")

    def test_Checkpoint_SharedObject_array(self) -> None:
        self.checkpoint_test_template("SharedObject", 1, 1, modelparams="--param=object_type:array --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", out_suffix = "_array")

    def test_Checkpoint_SharedObject_bool_array(self) -> None:
        self.checkpoint_test_template("SharedObject", 1, 2, modelparams="--param=object_type:bool_array --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", out_suffix = "_bool_array")

    def test_Checkpoint_SharedObject_map(self) -> None:
        self.checkpoint_test_template("SharedObject", 2, 1, modelparams="--param=object_type:map --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", out_suffix = "_map")

    def test_Checkpoint_SharedObject_set(self) -> None:
        self.checkpoint_test_template("SharedObject", 2, 2, modelparams="--param=object_type:set --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", out_suffix = "_set")

    def test_Checkpoint_SharedObject_bool_array_n2one(self) -> None:
        self.checkpoint_test_template("SharedObject", 1, 2, modelparams="--param=object_type:bool_array --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", out_suffix = "_bool_array", n_to_one=True, cpt_suffix="_n2one")

    def test_Checkpoint_n2one(self) -> None:
        self.checkpoint_test_template("Checkpoint", 1, 1, n_to_one=True, cpt_suffix="_n2one")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_Module(self) -> None:
        self.checkpoint_test_template("Module")

    ### Stats tests, including repartitioned restart tests
    @unittest.skipIf(host_os_get_distribution_type() == OS_DIST_ROCKY and host_os_get_distribution_version() == "10", "This test fails on Rocky 10")
    def test_Checkpoint_Statistics_basic(self) -> None:
        self.checkpoint_test_template("StatisticsComponent_basic")

    @unittest.skipIf(host_os_get_distribution_type() == OS_DIST_ROCKY and host_os_get_distribution_version() == "10", "This test fails on Rocky 10")
    def test_Checkpoint_Statistics_basic_n2one(self) -> None:
        self.checkpoint_test_template("StatisticsComponent_basic", n_to_one=True, cpt_suffix="_n2one")

    @unittest.skipIf(host_os_get_distribution_type() == OS_DIST_ROCKY and host_os_get_distribution_version() == "10", "This test fails on Rocky 10")
    def test_Checkpoint_Statistics_basic_start_serial(self) -> None:
        self.checkpoint_test_template("StatisticsComponent_basic", start_serial=True, cpt_suffix="_start_serial")

    @unittest.skipIf(host_os_get_distribution_type() == OS_DIST_ROCKY and host_os_get_distribution_version() == "10", "This test fails on Rocky 10")
    def test_Checkpoint_Statistics_basic_restart_smaller(self) -> None:
        self.checkpoint_test_template("StatisticsComponent_basic", restart_smaller=True, cpt_suffix="_restart_smaller")

    @unittest.skipIf(host_os_get_distribution_type() == OS_DIST_ROCKY and host_os_get_distribution_version() == "10", "This test fails on Rocky 10")
    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_Checkpoint_Statistics_basic_remap(self) -> None:
        self.checkpoint_test_template("StatisticsComponent_basic", swap_rank_thread=True, cpt_suffix="_remap")

    @unittest.skipIf(host_os_get_distribution_type() == OS_DIST_ROCKY and host_os_get_distribution_version() == "10", "This test fails on Rocky 10")
    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_Checkpoint_Statistics_basic_swap_restart_smaller(self) -> None:
        self.checkpoint_test_template("StatisticsComponent_basic", swap_rank_thread=True, restart_smaller=True, cpt_suffix="_swap_restart_smaller")



    ### sc_2u2u tests, including repartitioned restart tests
    def test_Checkpoint_sc_2u2u(self) -> None:
        self.checkpoint_test_template("sc_2u2u", 1, 2, subcomp=True, modelparams="1")

    def test_Checkpoint_sc_2u2u_n2one(self) -> None:
        self.checkpoint_test_template("sc_2u2u", 1, 2, subcomp=True, modelparams="1", n_to_one=True, cpt_suffix="_n2one")

    def test_Checkpoint_sc_2u2u_start_serial(self) -> None:
        self.checkpoint_test_template("sc_2u2u", 1, 2, subcomp=True, modelparams="1", start_serial=True, cpt_suffix="_start_serial")

    def test_Checkpoint_sc_2u2u_restart_smaller(self) -> None:
        self.checkpoint_test_template("sc_2u2u", 1, 2, subcomp=True, modelparams="1", restart_smaller=True, cpt_suffix="_restart_smaller")

    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_Checkpoint_sc_2u2u_remap(self) -> None:
        self.checkpoint_test_template("sc_2u2u", 1, 2, subcomp=True, modelparams="1", swap_rank_thread=True, cpt_suffix="_remap")

    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_Checkpoint_sc_2u2u_swap_restart_smaller(self) -> None:
        self.checkpoint_test_template("sc_2u2u", 1, 2, subcomp=True, modelparams="1", swap_rank_thread=True, restart_smaller=True, cpt_suffix="_swap_restart_smaller")


    ### Message Mesh tests, including repartitioned restart tests
    def test_Checkpiont_MessageMesh(self) -> None:
        self.checkpoint_test_template("MessageMesh", 1, 1, modelparams="6 6")

    def test_Checkpoint_MessageMesh_n2one(self) -> None:
        self.checkpoint_test_template("MessageMesh", 1, 2, modelparams="6 6", n_to_one=True, cpt_suffix="_n2one")

    def test_Checkpoint_MessageMesh_start_serial(self) -> None:
        self.checkpoint_test_template("MessageMesh", 1, 2, modelparams="6 6", start_serial=True, cpt_suffix="_start_serial")

    def test_Checkpoint_MessageMesh_restart_smaller(self) -> None:
        self.checkpoint_test_template("MessageMesh", 1, 2, modelparams="6 6", restart_smaller=True, cpt_suffix="_restart_smaller")

    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_Checkpoint_MessageMesh_remap(self) -> None:
        self.checkpoint_test_template("MessageMesh", 1, 2, modelparams="6 6", swap_rank_thread=True, cpt_suffix="_remap")

    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_Checkpoint_MessageMesh_swap_restart_smaller(self) -> None:
        self.checkpoint_test_template("MessageMesh", 1, 2, modelparams="6 6", swap_rank_thread=True, restart_smaller=True, cpt_suffix="_swap_restart_smaller")

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
    def checkpoint_test_template(self, testtype: str, rst_index: int = 1, cr_index: int = 0, subcomp: bool = False, modelparams: str = "", out_suffix: str = "", cpt_suffix : str = "", n_to_one: bool = False, swap_rank_thread : bool = False, start_serial : bool = False, restart_smaller : bool = False) -> None:

        # name conventions:
        # X_cpt - files/options associated with first checkpoint run
        # X_rst - files/options assocated with restart run
        # X_cr  - files/options associated with restart of checkpointed restart run

        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        # If this is a subcomponent test, the input and reffiles are in a subdirectory
        if subcomp:
            testsuitedir = testsuitedir + "/subcomponent_tests"

        reffile = "{0}/refFiles/test_{1}{2}.out".format(testsuitedir, testtype, out_suffix)

        ## Get the checkpoint interval from the reference file. We use
        ## the same period for both the original checkpoint run and
        ## the checkpoint of a restart run
        checkpoint_period = None
        with open(reffile, 'r') as file:
            for line in file:
                if line.startswith("# Creating simulation checkpoint at simulated time period of"):
                    # Split the line at 'of' and take the second part
                    parts = line.split('of', 1)
                    if len(parts) > 1:
                        # Get the part after 'of', remove trailing period and strip whitespace
                        checkpoint_period = parts[1].strip()[:-1]
                        break

        # Check to make sure this reference file had a checkpoint
        # period in it
        self.assertTrue(checkpoint_period, "Reference file {0} does not support checkpoint testing".format(reffile))


        ## First run

        # Get the primary sdl file and output file
        sdlfile_cpt = "{0}/test_{1}.py".format(testsuitedir,testtype)
        outfile_cpt = "{0}/test_{1}{2}{3}.out".format(outdir, testtype, out_suffix, cpt_suffix)

        # Get the checkpoint prefix
        prefix_cpt = "{0}{1}{2}_cpt".format(testtype, out_suffix, cpt_suffix)

        options_checkpoint_cpt = (
            "--checkpoint-sim-period='{0}' --checkpoint-prefix={1} "
            "--checkpoint-name-format='%p_%n' --output-directory=testsuite_checkpoint "
            "--model-options='{2}'".format(checkpoint_period, prefix_cpt, modelparams))

        if start_serial:
            # Need to run with 1 rank and 1 thread
            self.run_sst(sdlfile_cpt, outfile_cpt, other_args=options_checkpoint_cpt, num_ranks=1, num_threads=1)
        else:
            self.run_sst(sdlfile_cpt, outfile_cpt, other_args=options_checkpoint_cpt)

        # Check that original run got the right results
        filters_cpt = [
            CheckpointInfoFilter(),
            StartsWithFilter("WARNING: No components are assigned") ]

        cmp_result = testing_compare_filtered_diff(testtype, outfile_cpt, reffile, True, filters_cpt)
        if not cmp_result:
            diffdata = testing_get_diff_data(testtype)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Output from original checkpoint run {0} did not match reference file {1}".format(outfile_cpt, reffile))

        prefix_rst = "{0}{1}{2}_rst".format(testtype, out_suffix, cpt_suffix)

        ## Restart run
        sdlfile_rst = "{0}/testsuite_checkpoint/{1}/{1}_{2}/{1}_{2}.sstcpt".format(outdir, prefix_cpt, rst_index)
        outfile_rst = "{0}/test_{1}{2}{3}_restart.out".format(outdir, testtype, out_suffix, cpt_suffix)
        options_rst = "--load-checkpoint"
        options_checkpoint_rst = ""

        # If we are checkpointing the restart run, need to add those options
        if cr_index > 0:
            options_checkpoint_rst = (
                " --checkpoint-sim-period='{0}' --checkpoint-prefix={1} "
                "--checkpoint-name-format='%p_%n' --output-directory=testsuite_checkpoint "
                .format(checkpoint_period, prefix_rst))

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
        filters_rst = [
            CheckpointRefFileFilter(rst_index),
            CheckpointInfoFilter(),
            StartsWithFilter("WARNING: No components are assigned") ]

        cmp_result = testing_compare_filtered_diff(testtype, outfile_rst, reffile, True, filters_rst)
        if not cmp_result:
            diffdata = testing_get_diff_data(testtype)
            log_failure(diffdata)
            print(diffdata)
        self.assertTrue(cmp_result, "Output/Compare file {0} from first restart does not match Reference File {1}".format(outfile_rst, reffile))

        ## Optionally run from restart
        if cr_index <= 0: return

        sdlfile_cr = "{0}/testsuite_checkpoint/{1}/{1}_{2}/{1}_{2}.sstcpt".format(outdir, prefix_rst, cr_index )
        outfile_cr = "{0}/test_{1}{2}{3}_ckpt_restart.out".format(outdir, testtype, out_suffix, cpt_suffix)
        options_cr = "--load-checkpoint"

        # If swap_rank_thread is on, we will still just rerun the checkpoint from the restart with the original parallelism
        #if n_to_one:
        #    self.run_sst(sdlfile_cr, outfile_cr, other_args=options_cr, num_ranks=1, num_threads=1)
        #else:
        #    self.run_sst(sdlfile_cr, outfile_cr, other_args=options_cr)
        self.run_sst(sdlfile_cr, outfile_cr, other_args=options_cr)

        # Check that restart output is a subset of checkpoint output
        filters_cr = [
            CheckpointRefFileFilter(cr_index + rst_index),
            CheckpointInfoFilter() ]

        cmp_result = testing_compare_filtered_diff(testtype, outfile_cr, reffile, True, filters_cr)
        if not cmp_result:
            diffdata = testing_get_diff_data(testtype)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Output/Compare file {0} from second restart does not match Reference File {1}".format(outfile_cr, reffile))
