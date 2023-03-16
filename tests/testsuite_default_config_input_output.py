# -*- coding: utf-8 -*-
#
# Copyright 2009-2022 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2022, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import os
import sys

from sst_unittest import *
from sst_unittest_support import *

################################################################################
# Code to support a single instance module initialize, must be called setUp method

module_init = 0
module_sema = threading.Semaphore()

have_mpi = sst_core_config_include_file_get_value_int("SST_CONFIG_HAVE_MPI", default=0, disable_warning=True) == 1

def initializeTestModule_SingleInstance(class_inst):
    global module_init
    global module_sema

    module_sema.acquire()
    if module_init != 1:
        # Put your single instance Init Code Here
        module_init = 1
    module_sema.release()

################################################################################

class testcase_Config_input_output(SSTTestCase):

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

###

    def test_python_io(self):
        self.configio_test_template("python_io", "6 6", "py", False, "NONE")

    def test_python_io_comp(self):
        self.configio_test_template("python_io_comp", "", "py", False, "NONE", True)

    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_python_io_parallel(self):
        self.configio_test_template("python_io_parallel", "6 6", "py", True, "MULTI")

    def test_json_io(self):
        self.configio_test_template("json_io", "6 6", "json", False, "NONE")

    def test_json_io_comp(self):
        self.configio_test_template("json_io_comp", "", "json", False, "NONE", True)

    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_json_io_parallel(self):
        self.configio_test_template("json_io_parallel", "6 6", "json", True, "MULTI")


    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_python_single_parallel_load(self):
        self.configio_test_template("python_single_parallel_load", "6 6", "py", False, "SINGLE")


#####

    def configio_test_template(self, testtype, model_options, output_type, parallel_io, load_mode, use_component_test=False):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        output_config = "{0}/test_configio_{1}.{2}".format(outdir,testtype,output_type)
        if ( output_type == "py" ): out_flag = "--output-config"
        elif ( output_type == "json"): out_flag = "--output-json"
        else:
            print("Unknown output type: {0}".format(output_type))
            sys.exit(1)

        if parallel_io:
            options_ref = "{0}={1} --parallel-output --model-options=\"{2}\"".format(out_flag,output_config,model_options);
        else:
            options_ref = "{0}={1} --output-partition --model-options=\"{2}\"".format(out_flag,output_config,model_options);
        
        if have_mpi:
            options_check = "--parallel-load={0}".format(load_mode)
        else:
            options_check = ""

        # Set the various file paths
        if use_component_test:
            sdlfile = "{0}/test_Component.py".format(testsuitedir)
        else:
            sdlfile = "{0}/test_MessageMesh.py".format(testsuitedir)

        #reffile = "{0}/sharedobject_tests/refFiles/test_{1}.out".format(testsuitedir, testtype)
        outfile_ref = "{0}/test_configio_ref_{1}.out".format(outdir, testtype)
        outfile_check = "{0}/test_configio_check_{1}.out".format(outdir, testtype)

        self.run_sst(sdlfile, outfile_ref, other_args=options_ref)
        self.run_sst(output_config, outfile_check, other_args=options_check, check_sdl_file=False)

        # Perform the test
        cmp_result = testing_compare_sorted_diff(testtype, outfile_ref, outfile_check)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile_ref, outfile_check))
