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
import signal

from sst_unittest import *
from sst_unittest_support import *


################################################################################
# These tests test the RealTime features of SST including:
# - SIGUSR1 / SIGUSR2
# - SIGINT / SIGTERM
# --exit-after option
# --heartbeat-wall-period option
#
# Notes:
#   - signal_sec=1 is too small to be reliably caught on some of the VMs
#     and containers.
################################################################################

class testcase_Signals(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####
    
    # Test default SIGUSR1 action
    def test_RealTime_SIGUSR1_default(self):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()
        
        # Run test
        sdlfile = "{0}/test_RealTime.py".format(testsuitedir)
        outfile = "{0}/test_RealTime_SIGUSR1_default.out".format(outdir)
        self.run_sst(sdlfile, outfile, other_args="--exit-after=10", send_signal=signal.SIGUSR1, signal_sec=5)

        line_count = 0
        exit_count = 0
        sig_found = False
        with open(outfile) as fp:
            for line in fp:
                if "CurrentSimCycle:" in line:
                    sig_found = True
                elif "EXIT-AFTER TIME REACHED" in line:
                    exit_count += 1
                line_count += 1 
        
        ranks = testing_check_get_num_ranks()
        threads = testing_check_get_num_threads()
        num_para = threads * ranks
        num_lines = 101 + (threads * ranks) * 3
        self.assertTrue(sig_found, "Output file is missing SIGUSR1 triggered output message")
        self.assertTrue(exit_count == num_para, "Exit message count incorrect, should be {0}, found {1} in {2}".format(num_para,exit_count,outfile))
        self.assertTrue(line_count >= num_lines, "Line count incorrect, should be at least {0}, found {1} in {2}".format(num_lines,line_count,outfile))

    
    # Test default SIGUSR2 action
    def test_RealTime_SIGUSR2_default(self):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()
        
        # Run test
        sdlfile = "{0}/test_RealTime.py".format(testsuitedir)
        outfile = "{0}/test_RealTime_SIGUSR2_default.out".format(outdir)
        self.run_sst(sdlfile, outfile, other_args="--exit-after=10", send_signal=signal.SIGUSR2, signal_sec=5)

        line_count = 0
        exit_count = 0
        sig_found = False
        with open(outfile) as fp:
            for line in fp:
                if "---- Components: ----" in line:
                    sig_found = True
                elif "EXIT-AFTER TIME REACHED" in line:
                    exit_count += 1
                line_count += 1 
        
        ranks = testing_check_get_num_ranks()
        threads = testing_check_get_num_threads()
        num_para = threads * ranks
        num_lines = 101 + (threads * ranks) * 5
        self.assertTrue(sig_found, "Output file is missing SIGUSR2 triggered output message")
        self.assertTrue(exit_count >= num_para, "Exit message count incorrect, should be at least {0}, found {1} in {2}".format(exit_count,num_para,outfile))
        self.assertTrue(line_count >= num_lines, "Line count incorrect, should be at least {0}, found {1} in {2}".format(num_lines,line_count,outfile))
    
    # Test SIGINT
    @unittest.skipIf(testing_check_get_num_ranks() > 1, "This test does not run reliably with mpirun")
    def test_RealTime_SIGINT(self):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()
        
        # Run test
        sdlfile = "{0}/test_RealTime.py".format(testsuitedir)
        outfile = "{0}/test_RealTime_SIGINT.out".format(outdir)
        self.run_sst(sdlfile, outfile, other_args="--exit-after=10", send_signal=signal.SIGINT, signal_sec=5)

        line_count = 0
        exit_count = 0
        sig_found = False
        with open(outfile) as fp:
            for line in fp:
                if "EMERGENCY SHUTDOWN (" in line:
                    sig_found = True
                elif "EMERGENCY SHUTDOWN Complete (" in line:
                    exit_count += 1
                line_count += 1 
        
        ranks = testing_check_get_num_ranks()
        threads = testing_check_get_num_threads()
        num_para = threads * ranks
        num_lines = 101 + (threads * ranks) * 3
        self.assertTrue(sig_found, "Output file is missing SIGINT triggered output message")
        self.assertTrue(exit_count == num_para, "Exit message count incorrect, should be {0}, found {1} in {2}".format(num_para,exit_count,outfile))
        self.assertTrue(line_count == num_lines, "Line count incorrect, should be {0}, found {1} in {2}".format(num_lines,line_count,outfile))        

    # Test SIGTERM
    @unittest.skipIf(testing_check_get_num_ranks() > 1, "This test does not run reliably with mpirun")
    def test_RealTime_SIGTERM(self):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()
        
        # Run test
        sdlfile = "{0}/test_RealTime.py".format(testsuitedir)
        outfile = "{0}/test_RealTime_SIGTERM.out".format(outdir)
        self.run_sst(sdlfile, outfile, other_args="--exit-after=10", send_signal=signal.SIGTERM, signal_sec=5)

        line_count = 0
        exit_count = 0
        sig_found = False
        with open(outfile) as fp:
            for line in fp:
                if "EMERGENCY SHUTDOWN (" in line:
                    sig_found = True
                elif "EMERGENCY SHUTDOWN Complete (" in line:
                    exit_count += 1
                line_count += 1 
        
        ranks = testing_check_get_num_ranks()
        threads = testing_check_get_num_threads()
        num_para = threads * ranks
        num_lines = 101 + (threads * ranks) * 3
        self.assertTrue(sig_found, "Output file is missing SIGTERM triggered output message")
        self.assertTrue(exit_count == num_para, "Exit message count incorrect, should be {0}, found {1} in {2}".format(num_para,exit_count,outfile))
        self.assertTrue(line_count == num_lines, "Line count incorrect, should be {0}, found {1} in {2}".format(num_lines,line_count,outfile))  
    
    # Test SIGALRM + heartbeat action via heartbeat option
    def test_RealTime_heartbeat(self):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()
        
        # Run test
        sdlfile = "{0}/test_RealTime.py".format(testsuitedir)
        outfile = "{0}/test_RealTime_heartbeat.out".format(outdir)
        self.run_sst(sdlfile, outfile, other_args="--exit-after=6s --heartbeat-wall-period=1")

        # Cannot diff test output because times may differ
        # Check for at least 5 heartbeat messages
        # Check for ended due to exit-after
        # Check for right number of lines in output
        hb_count = 0
        exit_count = 0
        line_count = 0
        with open(outfile) as fp:
            for line in fp:
                if "Simulation Heartbeat" in line:
                    hb_count += 1
                elif "EXIT-AFTER TIME REACHED" in line:
                    exit_count += 1
                line_count += 1 
        
        ranks = testing_check_get_num_ranks()
        threads = testing_check_get_num_threads()
        num_para = threads * ranks
        num_lines = 126 + 2*num_para # basic heartbeat (>25) + exit messages (1 + 2*para) + Component Finished messages (100)
        if ranks > 1:
            num_lines += 10 # Extra heartbeat output for MPI
        self.assertTrue(hb_count >= 5, "Heartbeat count incorrect, should be at least 5, found {0} in {1}".format(hb_count,outfile))
        self.assertTrue(exit_count == num_para, "Exit message count incorrect, should be {0}, found {1} in {2}".format(num_para,exit_count,outfile))
        self.assertTrue(line_count >= num_lines, "Line count incorrect, should be {0}, found {1} in {2}".format(num_lines,line_count,outfile))
    
    # Test SIGALRM + checkpoint action via checkpoint option
    def test_RealTime_checkpoint(self):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()
        
        # Run test
        sdlfile = "{0}/test_RealTime.py".format(testsuitedir)
        outfile = "{0}/test_RealTime_checkpoint.out".format(outdir)
        self.run_sst(sdlfile, outfile, other_args="--exit-after=6s --checkpoint-wall-period=1 --checkpoint-prefix=realtime-cpt")

        # Simple check - test that checkpoint was generated
        cptdir = "{0}/realtime-cpt/".format(outdir)
        self.assertTrue(os.path.exists(cptdir), "Checkpoint directory was not created. Did not find '{0}'".format(cptdir))
        cptdir_list = os.listdir(cptdir)
        self.assertTrue(len(cptdir_list) > 0, "Checkpoint directory '{0}' is empty, expected at least one checkpoint.".format(cptdir))
        cptfile = cptdir + cptdir_list[0] + "/" + cptdir_list[0] + ".sstcpt"
        self.assertTrue(os.path.exists(cptfile), "Checkpoint file does not exist, file='{0}'".format(cptfile))
    
    # Test SIGUSR1 + heartbeat action via --sigusr1=
    def test_RealTime_SIGUSR1_heartbeat(self):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()
        
        # Run test
        sdlfile = "{0}/test_RealTime.py".format(testsuitedir)
        outfile = "{0}/test_RealTime_SIGUSR1_heartbeat.out".format(outdir)
        self.run_sst(sdlfile, outfile, other_args="--exit-after=10 --sigusr1=sst.rt.heartbeat", send_signal=signal.SIGUSR1, signal_sec=5)

        # Check for a heartbeat message
        # Check for ended due to exit-after
        hb_count = 0
        exit_count = 0
        line_count = 0
        with open(outfile) as fp:
            for line in fp:
                if "Simulation Heartbeat" in line:
                    hb_count += 1
                elif "EXIT-AFTER TIME REACHED" in line:
                    exit_count += 1
                line_count += 1 
        
        ranks = testing_check_get_num_ranks()
        threads = testing_check_get_num_threads()
        num_para = threads * ranks
        self.assertTrue(hb_count >= 1, "Heartbeat count incorrect, should be >= 1, found {0} in {1}".format(hb_count,outfile))
        self.assertTrue(exit_count == num_para, "Exit message count incorrect, should be {0}, found {1} in {2}".format(num_para,exit_count,outfile))


    # Test SIGUSR2 + heartbeat action via --sigusr2=
    def test_RealTime_SIGUSR2_heartbeat(self):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()
        
        # Run test
        sdlfile = "{0}/test_RealTime.py".format(testsuitedir)
        outfile = "{0}/test_RealTime_SIGUSR2_heartbeat.out".format(outdir)
        self.run_sst(sdlfile, outfile, other_args="--exit-after=10 --sigusr2=sst.rt.heartbeat", send_signal=signal.SIGUSR2, signal_sec=5)

        # Check for a heartbeat message
        # Check for ended due to exit-after
        hb_count = 0
        exit_count = 0
        line_count = 0
        with open(outfile) as fp:
            for line in fp:
                if "Simulation Heartbeat" in line:
                    hb_count += 1
                elif "EXIT-AFTER TIME REACHED" in line:
                    exit_count += 1
                line_count += 1 
        
        ranks = testing_check_get_num_ranks()
        threads = testing_check_get_num_threads()
        num_para = threads * ranks
        self.assertTrue(hb_count >= 1, "Heartbeat count incorrect, should be >= 1, found {0} in {1}".format(hb_count,outfile))
        self.assertTrue(exit_count == num_para, "Exit message count incorrect, should be {0}, found {1} in {2}".format(num_para,exit_count,outfile))

    # Test SIGALRM + core status + heartbeat action via --sigalrm=
    def test_RealTime_SIGALRM_multiaction(self):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()
        
        # Run test
        sdlfile = "{0}/test_RealTime.py".format(testsuitedir)
        outfile = "{0}/test_RealTime_SIGALRM_multiaction.out".format(outdir)
        self.run_sst(sdlfile, outfile, other_args="--exit-after=6s --sigalrm=sst.rt.heartbeat(interval=1s);sst.rt.status.core(interval=2s)")

        # Check for a heartbeat message: 5-6
        # Check for status: 2-3
        # Check for ended due to exit-after
        hb_count = 0
        exit_count = 0
        line_count = 0
        status_count = 0
        with open(outfile) as fp:
            for line in fp:
                if "Simulation Heartbeat" in line:
                    hb_count += 1
                elif "EXIT-AFTER TIME REACHED" in line:
                    exit_count += 1
                elif "CurrentSimCycle" in line:
                    status_count += 1
                line_count += 1 
        
        ranks = testing_check_get_num_ranks()
        threads = testing_check_get_num_threads()
        num_para = threads * ranks
        num_lines = 126 + 4*num_para # basic heartbeat (>25) + exit messages (1 + 2*para) + status (> 2*para) + Component Finished messages (100)
        if ranks > 1:
            num_lines += 10 # Extra heartbeat output for MPI (2 per HB)
        self.assertTrue(hb_count >= 5, "Heartbeat count incorrect, should be at least 5, found {0} in {1}".format(hb_count,outfile))
        self.assertTrue(status_count >= 2, "Output file is missing core status output messages")
        self.assertTrue(exit_count == num_para, "Exit message count incorrect, should be {0}, found {1} in {2}".format(num_para,exit_count,outfile))
        self.assertTrue(line_count >= num_lines, "Line count incorrect, should be {0}, found {1} in {2}".format(num_lines,line_count,outfile))
