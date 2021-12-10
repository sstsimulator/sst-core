# -*- coding: utf-8 -*-
import os
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

class testcase_PerfComponent(SSTTestCase):

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
        if(sst_core_config_include_file_get_value_int(define="SST_PERFORMANCE_INSTRUMENTING", default=0, disable_warning=True) > 0):
            if os.path.isfile(perfdata_out):
                os.system("mv {0} {1}".format(perfdata_out, perfdata_sav))
            else:
                self.assertTrue(False, "Output file {0} does not exist".format(perfdata_out))
            
            targetComponentFound = False
            success = False
            with open(perfdata_sav, 'r') as file:
                f = file.read()

            lines = f.split('\n')
            for line in lines:
                if targetComponentFound and ("Clock Handler Runtime:" in line):
                    if (float(line.split()[3].split('s')[0]) > 0):
                        success = True
                if "Component Name c0.0" in line:
                    targetComponentFound = True
                
            self.assertTrue(success, "Performance output file {0} does not contain expected value".format(perfdata_sav))

            



