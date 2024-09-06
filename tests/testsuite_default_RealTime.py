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

    def initializeClass(self, testName):
        super(type(self), self).initializeClass(testName)
        # Put test based setup code here. it is called before testing starts
        # NOTE: This method is called once for every test

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####
    def test_RealTime_SIGUSR1(self):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()
        
        # Run test
        sdlfile = "{0}/test_RealTime.py".format(testsuitedir)
        outfile = "{0}/test_RealTime_SIGUSR1.out".format(outdir)
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

    
    def test_RealTime_SIGUSR2(self):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()
        
        # Run test
        sdlfile = "{0}/test_RealTime.py".format(testsuitedir)
        outfile = "{0}/test_RealTime_SIGUSR2.out".format(outdir)
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
        self.assertTrue(line_count == num_lines, "Line count incorrect, should be {0}, found {1} in {2}".format(num_lines,line_count,outfile))
    
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
    
    def test_RealTime_SIGALRM(self):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()
        
        # Run test
        sdlfile = "{0}/test_RealTime.py".format(testsuitedir)
        outfile = "{0}/test_RealTime_SIGALRM.out".format(outdir)
        self.run_sst(sdlfile, outfile, other_args="--exit-after=6s --heartbeat-wall-period=1")

        # Cannot diff test output because times may differ
        # Check for 6 heartbeat messages
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
        num_lines = 131 + 2*num_para # basic heartbeat (30) + exit messages (1 + 2*para) + Component Finished messages (100)
        if ranks > 1:
            num_lines += 12 # Extra heartbeat output for MPI
        self.assertTrue(hb_count == 6, "Heartbeat count incorrect, should be 6, found {0} in {1}".format(hb_count,outfile))
        self.assertTrue(exit_count == num_para, "Exit message count incorrect, should be {0}, found {1} in {2}".format(num_para,exit_count,outfile))
        self.assertTrue(line_count == num_lines, "Line count incorrect, should be {0}, found {1} in {2}".format(num_lines,line_count,outfile))
