# -*- coding: utf-8 -*-
#
# Copyright 2009-2024 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2024, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import os
import filecmp

from sst_unittest import *
from sst_unittest_support import *

################################################################################
# Code to support a single instance module initialize, must be called setUp method

module_init = 0
module_sema = threading.Semaphore()

def initializeTestModule_SingleInstance(class_inst):
    global module_init
    global module_sema

    module_sema.acquire()
    if module_init != 1:
        # Put your single instance Init Code Here
        module_init = 1
    module_sema.release()

################################################################################

class testcase_Checkpoint(SSTTestCase):

    def initializeClass(self, testName):
        super(type(self), self).initializeClass(testName)
        # Put test based setup code here. it is called before testing starts
        # NOTE: This method is called once for every test

    def setUp(self):
        super(type(self), self).setUp()
        initializeTestModule_SingleInstance(self)
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####
    parallelerr = "Test only suports serial execution"

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint(self):
        self.checkpoint_test_template("Checkpoint", "500us", "500000000_0")
    
    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_SubComponent_sc_2a(self):
        self.checkpoint_test_template("sc_2a", "2500ns", "2500000_0", subcomp=True, modelparams="1")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_SubComponent_sc_2u2u(self):
        self.checkpoint_test_template("sc_2u2u", "2500ns", "2500000_0", subcomp=True, modelparams="1")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_sc_2u(self):
        self.checkpoint_test_template("sc_2u", "2500ns", "2500000_0", subcomp=True, modelparams="1")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_sc_a(self):
        self.checkpoint_test_template("sc_a", "2500ns", "2500000_0", subcomp=True, modelparams="1")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_sc_u2u(self):
        self.checkpoint_test_template("sc_u2u", "2500ns", "2500000_0", subcomp=True, modelparams="1")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_sc_u(self):
        self.checkpoint_test_template("sc_u", "2500ns", "2500000_0", subcomp=True, modelparams="1")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_sc_2u2a(self):
        self.checkpoint_test_template("sc_2u2a", "2500ns", "2500000_0", subcomp=True, modelparams="1")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_sc_2ua(self):
        self.checkpoint_test_template("sc_2ua", "2500ns", "2500000_0", subcomp=True, modelparams="1")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_sc_2uu(self):
        self.checkpoint_test_template("sc_2uu", "2500ns", "2500000_0", subcomp=True, modelparams="1")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_sc_u2a(self):
        self.checkpoint_test_template("sc_u2a", "2500ns", "2500000_0", subcomp=True, modelparams="1")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_sc_ua(self):
        self.checkpoint_test_template("sc_ua", "2500ns", "2500000_0", subcomp=True, modelparams="1")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_sc_uu(self):
        self.checkpoint_test_template("sc_uu", "2500ns", "2500000_0", subcomp=True, modelparams="1")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_SharedObject_array(self):
        self.checkpoint_test_template("SharedObject", "6ns", "6000_0", modelparams="--param=object_type:array --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", outstr = "SharedObject_array")
   
    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_SharedObject_bool_array(self):
        self.checkpoint_test_template("SharedObject", "6ns", "6000_0", modelparams="--param=object_type:bool_array --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", outstr = "SharedObject_bool_array")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_SharedObject_map(self):
        self.checkpoint_test_template("SharedObject", "6ns", "6000_0", modelparams="--param=object_type:map --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", outstr = "SharedObject_map")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_SharedObject_set(self):
        self.checkpoint_test_template("SharedObject", "6ns", "6000_0", modelparams="--param=object_type:set --param=num_entities:12 --param=full_initialization:true --param=checkpoint:true", outstr = "SharedObject_set")

    @unittest.skipIf(testing_check_get_num_ranks() > 1, parallelerr)
    @unittest.skipIf(testing_check_get_num_threads() > 1, parallelerr)
    def test_Checkpoint_Module(self):
        self.checkpoint_test_template("Module", "25us", "50000000_1")

#####
    # testtype: which test to run
    # cptfreq: Checkpoint frequency
    # cpttime: Which checkpoint to use for restart
    def checkpoint_test_template(self, testtype, cptfreq, cptrestart, subcomp=False, modelparams="", outstr=""):

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
        outfile_generate = "{0}/test_Checkpoint_{1}_generate.out".format(outdir, outstr)
        options_checkpoint="--checkpoint-period={0} --checkpoint-prefix={1} --model-options='{2}'".format(cptfreq,teststr,modelparams)
        self.run_sst(sdlfile_generate, outfile_generate, other_args=options_checkpoint)

        # Run from restart
        sdlfile_restart = "{0}/{1}_{2}.sstcpt".format(outdir,teststr,cptrestart)
        outfile_restart = "{0}/test_Checkpoint_{1}_restart.out".format(outdir, teststr)
        options_restart = "--load-checkpoint"
        self.run_sst(sdlfile_restart, outfile_restart, other_args=options_restart)

        # Check that restart output is a subset of checkpoint output
        cmp_result = testing_compare_filtered_subset(outfile_restart, outfile_generate)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile_restart, outfile_generate))

