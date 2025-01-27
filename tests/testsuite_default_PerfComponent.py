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
from sst_unittest import *
from sst_unittest_support import *


class testcase_PerfComponent(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_PerfComponent(self):
        self.perf_component_test_template("PerfComponent")

#####

    def perf_component_test_template(self, testtype):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        sdlfile = "{0}/test_PerfComponent.py".format(testsuitedir)
        reffile = "{0}/refFiles/test_PerfComponent.out".format(testsuitedir)
        outfile = "{0}/test_PerfComponent.out".format(outdir)

        self.run_sst(sdlfile, outfile)
        
        # Perform the test of the standard simulation output
        cmp_result = testing_compare_sorted_diff(testtype, outfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

        #save off the performance data
        perfdata_out = "{0}/rank_0_thread_0".format(outdir)
        perfdata_sav = "{0}/PerfComponent_PerfData.out".format(outdir)

        #if SST was built with SST_PERFORMANCE_INSTRUMENTING enabled, then we will also check that the counters are sane
        if(sst_core_config_include_file_get_value(define="SST_PERFORMANCE_INSTRUMENTING", type=int, default=0, disable_warning=True) > 0):
            if os.path.isfile(perfdata_out):
                os.system("mv {0} {1}".format(perfdata_out, perfdata_sav))
            else:
                self.assertTrue(False, "Output file {0} does not exist".format(perfdata_out))
            
            targetComponentFound = False
            foundClockHandler = False
            foundClockRuntime = False
            success = False
            with open(perfdata_sav, 'r') as file:
                f = file.read()

            lines = f.split('\n')
            for line in lines:
                if foundClockHandler:
                    if (float(line.split()[2].split('s')[0]) > 0):
                        foundClockRuntime = True
                    foundClockHandler = False
                if ("Clock Handlers" in line):
                    foundClockHandler = True
                if "Component c0_0" in line:
                    targetComponentFound = True
              
            success = targetComponentFound and foundClockRuntime
            self.assertTrue(success, "Performance output file {0} does not contain expected value".format(perfdata_sav))

            



