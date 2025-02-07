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


class testcase_PortModule(SSTTestCase):

    def setUp(self) -> None:
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self) -> None:
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

    
    ### Non-checkpoint tests
    #### Non-SubCopmponent tests
    ##### Receive side tests
    def test_PortModule_pass_recv(self) -> None:
        self.port_module_test_template("pass", False, False)

    def test_PortModule_drop_recv(self) -> None:
        self.port_module_test_template("drop", False, False)

    def test_PortModule_modify_recv(self) -> None:
        self.port_module_test_template("modify", False, False)

    def test_PortModule_replace_recv(self) -> None:
        self.port_module_test_template("replace", False, False)

    def test_PortModule_randomdrop_recv(self) -> None:
        self.port_module_test_template("randomdrop", False, False)

    ##### Send side tests
    def test_PortModule_pass_send(self) -> None:
        self.port_module_test_template("pass", True, False)

    def test_PortModule_drop_send(self) -> None:
        self.port_module_test_template("drop", True, False)

    def test_PortModule_modify_send(self) -> None:
        self.port_module_test_template("modify", True, False)

    def test_PortModule_replace_send(self) -> None:
        self.port_module_test_template("replace", True, False)

    def test_PortModule_randomdrop_send(self) -> None:
        self.port_module_test_template("randomdrop", True, False)

    #### SubCopmponent tests
    ##### Receive side tests
    def test_PortModule_pass_recv_sub(self) -> None:
        self.port_module_test_template("pass", False, True)

    def test_PortModule_drop_recv_sub(self) -> None:
        self.port_module_test_template("drop", False, True)

    def test_PortModule_modify_recv_sub(self) -> None:
        self.port_module_test_template("modify", False, True)

    def test_PortModule_replace_recv_sub(self) -> None:
        self.port_module_test_template("replace", False, True)

    def test_PortModule_randomdrop_recv_sub(self) -> None:
        self.port_module_test_template("randomdrop", False, True)

    ##### Send side tests
    def test_PortModule_pass_send_sub(self) -> None:
        self.port_module_test_template("pass", True, True)

    def test_PortModule_drop_send_sub(self) -> None:
        self.port_module_test_template("drop", True, True)

    def test_PortModule_modify_send_sub(self) -> None:
        self.port_module_test_template("modify", True, True)

    def test_PortModule_replace_send_sub(self) -> None:
        self.port_module_test_template("replace", True, True)

    def test_PortModule_randomdrop_send_sub(self) -> None:
        self.port_module_test_template("randomdrop", True, True)


    ### Checkpoint tests
    #### Non-SubCopmponent tests
    ##### Receive side tests
    def test_PortModule_Checkpoint_pass_recv(self) -> None:
        self.port_module_test_template("pass", False, False, True)

    def test_PortModule_Checkpoint_drop_recv(self) -> None:
        self.port_module_test_template("drop", False, False, True)

    def test_PortModule_Checkpoint_modify_recv(self) -> None:
        self.port_module_test_template("modify", False, False, True)

    def test_PortModule_Checkpoint_replace_recv(self) -> None:
        self.port_module_test_template("replace", False, False, True)

    def test_PortModule_Checkpoint_randomdrop_recv(self) -> None:
        self.port_module_test_template("randomdrop", False, False, True)

    ##### Send side tests
    def test_PortModule_Checkpoint_pass_send(self) -> None:
        self.port_module_test_template("pass", True, False, True)

    def test_PortModule_Checkpoint_drop_send(self) -> None:
        self.port_module_test_template("drop", True, False, True)

    def test_PortModule_Checkpoint_modify_send(self) -> None:
        self.port_module_test_template("modify", True, False, True)

    def test_PortModule_Checkpoint_replace_send(self) -> None:
        self.port_module_test_template("replace", True, False, True)

    def test_PortModule_Checkpoint_randomdrop_send(self) -> None:
        self.port_module_test_template("randomdrop", True, False, True)

    #### SubCopmponent tests
    ##### Receive side tests
    def test_PortModule_Checkpoint_pass_recv_sub(self) -> None:
        self.port_module_test_template("pass", False, True, True)

    def test_PortModule_Checkpoint_drop_recv_sub(self) -> None:
        self.port_module_test_template("drop", False, True, True)

    def test_PortModule_Checkpoint_modify_recv_sub(self) -> None:
        self.port_module_test_template("modify", False, True, True)

    def test_PortModule_Checkpoint_replace_recv_sub(self) -> None:
        self.port_module_test_template("replace", False, True, True)

    def test_PortModule_Checkpoint_randomdrop_recv_sub(self) -> None:
        self.port_module_test_template("randomdrop", False, True, True)

    ##### Send side tests
    def test_PortModule_Checkpoint_pass_send_sub(self) -> None:
        self.port_module_test_template("pass", True, True, True)

    def test_PortModule_Checkpoint_drop_send_sub(self) -> None:
        self.port_module_test_template("drop", True, True, True)

    def test_PortModule_Checkpoint_modify_send_sub(self) -> None:
        self.port_module_test_template("modify", True, True, True)

    def test_PortModule_Checkpoint_replace_send_sub(self) -> None:
        self.port_module_test_template("replace", True, True, True)

    def test_PortModule_Checkpoint_randomdrop_send_sub(self) -> None:
        self.port_module_test_template("randomdrop", True, True, True)
        
        
#####
    # testtype: which test to run
    # bind_on_send: Use send PortModules if set to True, recv PortModules, otherwise
    # use_subcomp: When set to True, will use subcomponent to configure links to test PortModules on shared ports
    # checkpoint: test checkpointing
    def port_module_test_template(self, testtype: str, bind_on_send: bool, use_subcomp: bool, checkpoint: bool = False) -> None:

        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        if bind_on_send:
            bind_at = "send"
        else:
            bind_at = "recv"

        sdlfile = "{0}/test_PortModule.py".format(testsuitedir)
        if testtype != "randomdrop":
            reffile = "{0}/refFiles/test_PortModule_{1}.out".format(testsuitedir,testtype)
        else:
            reffile = "{0}/refFiles/test_PortModule_{1}_{2}.out".format(testsuitedir,testtype,bind_at)

        if use_subcomp:
            suffix = "{1}_{2}_subcomp".format(outdir,testtype,bind_at)
            subcomp_str = " --subcomp"
        else:
            suffix = "{1}_{2}".format(outdir,testtype,bind_at)
            subcomp_str = ""
            
        options = "--model-options=\"--{0} --{1}{2}\"".format(testtype,bind_at,subcomp_str)

        if checkpoint:
            options += " --checkpoint-sim-period=1.2us --checkpoint-prefix=ckpt_PortModule_{0}".format(suffix)
            outfile = "{0}/test_PortModule_{1}_ckpt.out".format(outdir,suffix)
        else:
            outfile = "{0}/test_PortModule_{1}.out".format(outdir,suffix)

        
        self.run_sst(sdlfile, outfile, other_args=options)

        if not checkpoint:
            filter1 = StartsWithFilter("#")
                
            cmp_result = testing_compare_filtered_diff("PortModule", outfile, reffile, True, filter1)
            self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))
        else:
            # Need to rerun from the checkpoint, then compare this output against the reffile
            options = "--load-checkpoint"
            sdlfile = "{0}/ckpt_PortModule_{1}/ckpt_PortModule_{1}_0_1200000/ckpt_PortModule_{1}_0_1200000.sstcpt".format(outdir,suffix)
            outfile = "{0}/test_PortModule_{1}_restart.out".format(outdir,suffix)

            self.run_sst(sdlfile, outfile, other_args=options)

            filter1 = IgnoreAllBeforeFilter("# Simulation Checkpoint:")
            filter1.apply_to_out_file = False
                
            cmp_result = testing_compare_filtered_diff("PortModule", outfile, reffile, True, filter1)
            self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))
