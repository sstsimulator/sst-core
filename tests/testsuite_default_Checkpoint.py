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

    def test_Checkpoint_sc_2u2u(self) -> None:
        self.checkpoint_test_template("sc_2u2u", 1, 2, subcomp=True, modelparams="1")

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
        self.checkpoint_test_template("SharedObject", 1, 1, modelparams="--param=object_type:array --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", outstr = "SharedObject_array")

    def test_Checkpoint_SharedObject_bool_array(self) -> None:
        self.checkpoint_test_template("SharedObject", 1, 2, modelparams="--param=object_type:bool_array --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", outstr = "SharedObject_bool_array")

    def test_Checkpoint_SharedObject_map(self) -> None:
        self.checkpoint_test_template("SharedObject", 2, 1, modelparams="--param=object_type:map --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", outstr = "SharedObject_map")

    def test_Checkpoint_SharedObject_set(self) -> None:
        self.checkpoint_test_template("SharedObject", 2, 2, modelparams="--param=object_type:set --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", outstr = "SharedObject_set")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_Module(self) -> None:
        self.checkpoint_test_template("Module")

#####
    # testtype: which test to run
    # rst_index: checkpoint index to restart from on first restart
    # cr_index: checkpoint index to restart from when restarting from a checkpointed restart run
    # subcom: Set to True if this is a subcomponent test as the file/path name differs
    # modelparams: Set what is passed into --model-params when running SST
    # outstr: If set, is used in place of testtype to generate reference and output filenames
    def checkpoint_test_template(self, testtype: str, rst_index: int = 1, cr_index: int = 0, subcomp: bool = False, modelparams: str = "", outstr: str = "") -> None:

        # name conventions:
        # X_cpt - files/options associated with first checkpoint run
        # X_rst - files/options assocated with restart run
        # X_cr  - files/options associated with restart of checkpointed restart run

        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        # Outstr used to differentiate output files for runs that take
        # the same input file
        if outstr != "":
            teststr = outstr
        else:
            teststr = testtype


        if ( subcomp ):
            reffile = "{0}/subcomponent_tests/refFiles/test_{1}.out".format(testsuitedir, testtype)
        else:
            reffile = "{0}/refFiles/test_{1}.out".format(testsuitedir, teststr)

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
        if ( subcomp ):
            sdlfile_cpt = "{0}/subcomponent_tests/test_{1}.py".format(testsuitedir, testtype)
            outfile_cpt = "{0}/test_SubComponent_{1}.out".format(outdir, teststr)
        else:
            sdlfile_cpt = "{0}/test_{1}.py".format(testsuitedir,testtype)
            outfile_cpt = "{0}/test_{1}.out".format(outdir, teststr)


        prefix_cpt = teststr + "_cpt"

        options_checkpoint_cpt = (
            "--checkpoint-sim-period='{0}' --checkpoint-prefix={1} "
            "--checkpoint-name-format='%p_%n' --output-directory=testsuite_checkpoint "
            "--model-options='{2}'".format(checkpoint_period, prefix_cpt, modelparams))

        self.run_sst(sdlfile_cpt, outfile_cpt, other_args=options_checkpoint_cpt)

        # Check that original run got the right results
        filters_cpt = [
            CheckpointInfoFilter(),
            StartsWithFilter("WARNING: No components are assigned") ]

        cmp_result = testing_compare_filtered_diff("NeedSomethingHere", outfile_cpt, reffile, True, filters_cpt)
        self.assertTrue(cmp_result, "Output from original checkpoint run {0} did not match reference file {1}".format(outfile_cpt, reffile))

        prefix_rst = teststr + "_rst"

        ## Restart run
        sdlfile_rst = "{0}/testsuite_checkpoint/{1}/{1}_{2}/{1}_{2}.sstcpt".format(outdir, prefix_cpt, rst_index)
        outfile_rst = "{0}/test_Checkpoint_{1}_restart.out".format(outdir, testtype)
        options_rst = "--load-checkpoint"
        options_checkpoint_rst = ""

        # If we are checkpointing the restart run, need to add those options
        if cr_index > 0:
            options_checkpoint_rst = (
                " --checkpoint-sim-period='{0}' --checkpoint-prefix={1} "
                "--checkpoint-name-format='%p_%n' --output-directory=testsuite_checkpoint "
                .format(checkpoint_period, prefix_rst))

        options_rst += options_checkpoint_rst

        self.run_sst(sdlfile_rst, outfile_rst, other_args=options_rst)

        # Check that restart output is a subset of checkpoint output
        filters_rst = [
            CheckpointRefFileFilter(rst_index),
            CheckpointInfoFilter(),
            StartsWithFilter("WARNING: No components are assigned") ]

        cmp_result = testing_compare_filtered_diff("PortModule", outfile_rst, reffile, True, filters_rst)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile_rst, reffile))

        ## Optionally run from restart
        if cr_index <= 0: return

        sdlfile_cr = "{0}/testsuite_checkpoint/{1}/{1}_{2}/{1}_{2}.sstcpt".format(outdir, prefix_rst, cr_index )
        outfile_cr = "{0}/test_Checkpoint_{1}_ckpt_restart.out".format(outdir, testtype)
        options_cr = "--load-checkpoint"

        self.run_sst(sdlfile_cr, outfile_cr, other_args=options_cr)

        # Check that restart output is a subset of checkpoint output
        filters_cr = [
            CheckpointRefFileFilter(cr_index + rst_index),
            CheckpointInfoFilter() ]

        cmp_result = testing_compare_filtered_diff("PortModule", outfile_cr, reffile, True, filters_cr)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile_cr, reffile))
