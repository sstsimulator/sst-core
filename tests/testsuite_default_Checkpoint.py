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

    def test_Checkpoint(self) -> None:
        self.checkpoint_test_template("Checkpoint", "500us", "0_500000000")

    def test_Checkpoint_SubComponent_sc_2a(self) -> None:
        self.checkpoint_test_template("sc_2a", "2500ns", "0_2500000", subcomp=True, modelparams="1")

    def test_Checkpoint_SubComponent_sc_2u2u(self) -> None:
        self.checkpoint_test_template("sc_2u2u", "2500ns", "0_2500000", subcomp=True, modelparams="1")

    def test_Checkpoint_sc_2u(self) -> None:
        self.checkpoint_test_template("sc_2u", "2500ns", "0_2500000", subcomp=True, modelparams="1")

    def test_Checkpoint_sc_a(self) -> None:
        self.checkpoint_test_template("sc_a", "2500ns", "0_2500000", subcomp=True, modelparams="1")

    def test_Checkpoint_sc_u2u(self) -> None:
        self.checkpoint_test_template("sc_u2u", "2500ns", "0_2500000", subcomp=True, modelparams="1")

    def test_Checkpoint_sc_u(self) -> None:
        self.checkpoint_test_template("sc_u", "2500ns", "0_2500000", subcomp=True, modelparams="1")

    def test_Checkpoint_sc_2u2a(self) -> None:
        self.checkpoint_test_template("sc_2u2a", "2500ns", "0_2500000", subcomp=True, modelparams="1")

    def test_Checkpoint_sc_2ua(self) -> None:
        self.checkpoint_test_template("sc_2ua", "2500ns", "0_2500000", subcomp=True, modelparams="1")

    def test_Checkpoint_sc_2uu(self) -> None:
        self.checkpoint_test_template("sc_2uu", "2500ns", "0_2500000", subcomp=True, modelparams="1")

    def test_Checkpoint_sc_u2a(self) -> None:
        self.checkpoint_test_template("sc_u2a", "2500ns", "0_2500000", subcomp=True, modelparams="1")

    def test_Checkpoint_sc_ua(self) -> None:
        self.checkpoint_test_template("sc_ua", "2500ns", "0_2500000", subcomp=True, modelparams="1")

    def test_Checkpoint_sc_uu(self) -> None:
        self.checkpoint_test_template("sc_uu", "2500ns", "0_2500000", subcomp=True, modelparams="1")

    def test_Checkpoint_SharedObject_array(self) -> None:
        self.checkpoint_test_template("SharedObject", "6ns", "0_6000", modelparams="--param=object_type:array --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", outstr = "SharedObject_array")

    def test_Checkpoint_SharedObject_bool_array(self) -> None:
        self.checkpoint_test_template("SharedObject", "6ns", "0_6000", modelparams="--param=object_type:bool_array --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", outstr = "SharedObject_bool_array")

    def test_Checkpoint_SharedObject_map(self) -> None:
        self.checkpoint_test_template("SharedObject", "6ns", "0_6000", modelparams="--param=object_type:map --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", outstr = "SharedObject_map")

    def test_Checkpoint_SharedObject_set(self) -> None:
        self.checkpoint_test_template("SharedObject", "6ns", "0_6000", modelparams="--param=object_type:set --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", outstr = "SharedObject_set")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_Module(self) -> None:
        self.checkpoint_test_template("Module", "25us", "1_50000000")

#####
    # testtype: which test to run
    # cptfreq: Checkpoint frequency
    # cpttime: Which checkpoint to use for restart
    def checkpoint_test_template(self, testtype: str, cptfreq: str, cptrestart: str, subcomp: bool = False, modelparams: str = "", outstr: str = "") -> None:

        # Different kind of test
        # Run specified test file (will generate a checkpoint)
        # Re-run from a checkpoint
        # Check that new reffile is a subset of the original

        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        if outstr != "":
            teststr = outstr
        else:
            teststr = testtype

        # Generate checkpoint
        if ( subcomp ):
            sdlfile_generate = "{0}/subcomponent_tests/test_{1}.py".format(testsuitedir, testtype)
        else:
            sdlfile_generate = "{0}/test_{1}.py".format(testsuitedir,testtype)

        outfile_generate = "{0}/test_Checkpoint_{1}_generate.out".format(outdir,teststr)
        options_checkpoint="--checkpoint-sim-period={0} --checkpoint-prefix={1} --output-directory=testsuite_checkpoint --model-options='{2}'".format(cptfreq,teststr,modelparams)
        self.run_sst(sdlfile_generate, outfile_generate, other_args=options_checkpoint)
        
        # Run from restart
        sdlfile_restart = "{0}/testsuite_checkpoint/{1}/{1}_{2}/{1}_{2}.sstcpt".format(outdir,teststr,cptrestart)
        outfile_restart = "{0}/test_Checkpoint_{1}_restart.out".format(outdir, teststr)
        options_restart = "--load-checkpoint"
        self.run_sst(sdlfile_restart, outfile_restart, other_args=options_restart)

        # Check that restart output is a subset of checkpoint output
        cmp_result = testing_compare_filtered_subset(outfile_restart, outfile_generate)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile_restart, outfile_generate))

