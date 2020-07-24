# -*- coding: utf-8 -*-

## Copyright 2009-2020 NTESS. Under the terms
## of Contract DE-NA0003525 with NTESS, the U.S.
## Government retains certain rights in this software.
##
## Copyright (c) 2009-2020, NTESS
## All rights reserved.
##
## This file is part of the SST software package. For license
## information, see the LICENSE file in the top level directory of the
## distribution.

""" This module provides the basic UnitTest class for the test system, along
    with a large number of support functions that tests can call
"""

import sys
import os
import unittest
import threading

import test_engine_globals
from sst_unittest_support import *
from test_engine_support import OSCommand
from test_engine_support import check_param_type
from test_engine_support import strclass
from test_engine_support import strqual
from test_engine_junit import JUnitTestSuite
from test_engine_junit import junit_to_xml_report_file
#from test_engine_junit import junit_to_xml_report_string

################################################################################

PY2 = sys.version_info[0] == 2
PY3 = sys.version_info[0] == 3

################################################################################

class SSTTestCase(unittest.TestCase):
    """ This class is the SST TestCase class """

    def __init__(self, methodName):
        # NOTE: __init__ is called at startup for all tests before any
        #       setUpModules(), setUpClass(), setUp() and the like are called.
        super(SSTTestCase, self).__init__(methodName)
        self.testname = methodName
        parent_module_path = os.path.dirname(sys.modules[self.__class__.__module__].__file__)
        self._testsuite_dirpath = parent_module_path
        #log_forced("SSTTestCase: __init__() - {0}".format(self.testname))
        self.initializeClass(self.testname)

###

    def initializeClass(self, testname):
        """ Called immediately before class is initialized
        """
        # Placeholder method for overridden method in derived class
        #log_forced("\nSSTTestCase: initializeClass() - {0}".format(testname))

###

    def setUp(self):
        """ (Single Thread Testing) - Called immediately before a test is run
            (Concurrent Thread Testing) - Called immediately before a test is run
        """
        #log_forced("SSTTestCase: setUp() - {0}".format(self.testname))
        if not test_engine_globals.TESTENGINE_CONCURRENTMODE:
            test_engine_globals.TESTRUN_TESTRUNNINGFLAG = True

        # TESTRUN_SINGTHREAD_TESTSUITE_NAME is used for non-concurrent
        # (ie single thread) runs
        test_engine_globals.TESTRUN_SINGTHREAD_TESTSUITE_NAME = self.get_testsuite_name()

###

    def tearDown(self):
        """ (Single Thread Testing) - Called immediately after a test finishes
            (Concurrent Thread Testing) - Called immediately after a test finishes
        """
        #log_forced("SSTTestCase: tearDown() - {0}".format(self.testname))
        if not test_engine_globals.TESTENGINE_CONCURRENTMODE:
            test_engine_globals.TESTRUN_TESTRUNNINGFLAG = False

###

    @classmethod
    def setUpClass(cls):
        """ (Single Thread Testing) - Called immediately before the TestCase starts
            This is called before any tests in a TestCase are run
            (Concurrent Thread Testing) - NOT CALLED, NOT AVAILABLE
        """
        #log_forced("SSTTestCase: setUpClass() - {0}".\
        #format(sys.modules[cls.__module__].__file__))

###

    @classmethod
    def tearDownClass(cls):
        """ (Single Thread Testing) - Called immediately after a TestCase finishes
            This is called after all tests in a TestCase have finished running
            (Concurrent Thread Testing) - NOT CALLED, NOT AVAILABLE
        """
        #log_forced("SSTTestCase: tearDownClass() - {0}".\
        #format(sys.modules[cls.__module__].__file__))

###

    def get_testsuite_name(self):
        """ Return the testsuite (module) name
           :return: str The Name
        """
        return "{0}".format(strclass(self.__class__))

###

    def get_testcase_name(self):
        """ Return the testscase name
           :return: str The Name
        """
        return "{0}".format(strqual(self.__class__))
###

    def get_testsuite_dir(self):
        """ Return the directory path of the testsuite that is being run
           :return: str the path
        """
        return self._testsuite_dirpath

###

    def get_test_output_run_dir(self):
        """ Return the path of the output run directory
           :return: str the dir
        """
        return get_test_output_run_dir()

###

    def get_test_output_tmp_dir(self):
        """ Return the path of the output run directory
           :return: str the dir
        """
        return get_test_output_tmp_dir()

################################################################################
### Method to run an SST simulation
################################################################################

    def run_sst(self, sdl_file, out_file, err_file=None, set_cwd=None, mpi_out_files="",
                other_args="", num_ranks=None, num_threads=None, global_args=None,
                timeout_sec=60):
        """ Launch sst with with the command line and send output to the
            output file.  Other parameters can also be passed in.
           :param: sdl_file (str): The FilePath to the sdl file
           :param: out_file (str): The FilePath to the finalized output file
           :param: err_file (str): The FilePath to the finalized error file
                                   The default is the same as the output file.
           :param: mpi_out_files (str): The FilePath to the mpi run output files
                                        These will be merged into the out_file at
                                        the end of a multi-rank run
           :param: other_args (str): Any other arguments used in the SST cmd
                                     that the caller wishes to use
           :param: num_ranks (int): The number of ranks to run SST with
           :param: num_threads (int): The number of threads to run SST with
           :param: global_args (str): Global Arguments provided from test engine args
           :param: timeout_sec (int|float): Allowed runtime in seconds
        """
        # NOTE: We cannot set the default of param to the global variable due to
        # oddities on how this class loads, so we do it here.
        if num_ranks is None:
            num_ranks = test_engine_globals.TESTENGINE_SSTRUN_NUMRANKS
        if num_threads is None:
            num_threads = test_engine_globals.TESTENGINE_SSTRUN_NUMTHREADS
        if global_args is None:
            global_args = test_engine_globals.TESTENGINE_SSTRUN_GLOBALARGS

        # Make sure arguments are of valid types
        check_param_type("sdl_file", sdl_file, str)
        check_param_type("out_file", out_file, str)
        if err_file is not None:
            check_param_type("err_file", out_file, str)
        if set_cwd is not None:
            check_param_type("set_cwd", set_cwd, str)
        check_param_type("mpi_out_files", mpi_out_files, str)
        check_param_type("other_args", other_args, str)
        if num_ranks is not None:
            check_param_type("num_ranks", num_ranks, int)
        if num_threads is not None:
            check_param_type("num_threads", num_threads, int)
        if global_args is not None:
            check_param_type("global_args", global_args, str)
        if not (isinstance(timeout_sec, (int, float)) and not isinstance(timeout_sec, bool)):
            raise ValueError("ERROR: Timeout_sec must be a postive int or a float")

        # Make sure sdl file is exists and is a file
        if not os.path.exists(sdl_file) or not os.path.isfile(sdl_file):
            log_error("sdl_file {0} does not exist".format(sdl_file))

        # Figure out a name for the mpi_output files if the default is provided
        if mpi_out_files == "":
            mpiout_filename = "{0}.testfile".format(out_file)
        else:
            mpiout_filename = mpi_out_files

        # Set the initial os launch command for sst.
        # If multi-threaded, include the number of threads
        if num_threads > 1:
            oscmd = "sst -n {0} {1} {2} {3}".format(num_threads,
                                                    global_args,
                                                    other_args,
                                                    sdl_file)
        else:
            oscmd = "sst {0} {1} {2}".format(global_args,
                                             other_args,
                                             sdl_file)

        # Update the os launch command if we are running multi-rank
        num_cores = get_num_cores_on_system()

        # Check to see if mpirun is available
        mpi_avail = False
        rtn = os.system("which mpirun > /dev/null")
        if rtn == 0:
            mpi_avail = True

        if num_ranks > 1:
            numa_param = ""
            if 2 <= num_cores <= 4:
                numa_param = "-map-by numa:pe=2 -oversubscribe"
            elif num_cores >= 4:
                numa_param = "-map-by numa:pe=2"

            oscmd = "mpirun -np {0} {1} -output-filename {2} {3}".format(num_ranks,
                                                                         numa_param,
                                                                         mpiout_filename,
                                                                         oscmd)

        # Identify the working directory that we are launching SST from
        final_wd = os.getcwd()
        if set_cwd is not None:
            final_wd = os.path.abspath(set_cwd)

        # Log some debug info on the launch of SST
        log_debug((("-- SST Launch Command (In Dir={0}; Ranks:{1}; Threads:{2};") +
                   (" Num Cores:{3}) = {4}")).format(final_wd, num_ranks, num_threads,
                                                     num_cores, oscmd))

        # Launch SST
        if num_ranks > 1:
            if mpi_avail is False:
                log_fatal("OpenMPI IS NOT FOUND/AVAILABLE")
            rtn = OSCommand(oscmd, out_file, err_file, set_cwd).run(timeout_sec=timeout_sec)
            merge_mpi_files("{0}*".format(mpiout_filename), mpiout_filename, out_file)
        else:
            rtn = OSCommand(oscmd, out_file, err_file, set_cwd).run(timeout_sec=timeout_sec)

        # Look for runtime error conditions
        err_str = "SST Timed-Out ({0} secs) while running {1}".format(timeout_sec, oscmd)
        self.assertFalse(rtn.timeout(), err_str)
        err_str = "SST returned {0}; while running {1}".format(rtn.result(), oscmd)
        self.assertEqual(rtn.result(), 0, err_str)

################################################################################
### Module level support
################################################################################

def setUpModule():
    """ (Single Thread Testing) - Called immediately before the testing Module loads
        This is called before any a testsuite module is loaded, and before
        any TestCases or tests are run
        (Concurrent Thread Testing) - NOT CALLED
    """
    #log_forced("SSTTestCase: setUpModule() - {0}".format(__file__))
    test_engine_globals.TESTRUN_JUNIT_TESTCASE_DICTLISTS['singlethread'] = []

###

def tearDownModule():
    """ (Single Thread Testing) - Called immediately after a testing module finishes
        This is called after all tests in all TestCases have finished running
        (Concurrent Thread Testing) - NOT CALLED
    """
    #log_forced("SSTTestCase: tearDownModule() - {0}".format(__file__))
    t_s = JUnitTestSuite(test_engine_globals.TESTRUN_SINGTHREAD_TESTSUITE_NAME,
                         test_engine_globals.TESTRUN_JUNIT_TESTCASE_DICTLISTS['singlethread'])

    # Write out Test Suite Results
    #log_forced(junit_to_xml_report_string([t_s]))
    xml_out_filepath = ("{0}/{1}.xml".format(test_engine_globals.TESTOUTPUT_XMLDIRPATH,
                                             test_engine_globals.TESTRUN_SINGTHREAD_TESTSUITE_NAME))

    with open(xml_out_filepath, 'w') as file_out:
        junit_to_xml_report_file(file_out, [t_s])

###################

def setUpModuleConcurrent(test):
    """ (Single Thread Testing) - NOT CALLED
        (Concurrent Thread Testing) - Called immediately before the testing Module loads
        This is called before any a testsuite module is loaded, and before
        any TestCases or tests are run
    """
    testsuite_name = test.get_testsuite_name()
#    testcase_name = test.get_testcase_name()
#    log_forced("\nSSTTestCase - setUpModuleConcurrent suite={0}; case={1}; test={2}".\
#    format(testsuite_name, testcase_name, test))
    if not testsuite_name in test_engine_globals.TESTRUN_JUNIT_TESTCASE_DICTLISTS:
        test_engine_globals.TESTRUN_JUNIT_TESTCASE_DICTLISTS[testsuite_name] = []

def tearDownModuleConcurrent(test):
    """ (Single Thread Testing) - NOT CALLED
        (Concurrent Thread Testing) - Called immediately after a testing module finishes
        This is called after all tests in all TestCases have finished running
    """
    testsuite_name = test.get_testsuite_name()
#    testcase_name = test.get_testcase_name()
#    log_forced("\nSSTTestCase - tearDownModuleConcurrent suite={0}; case={1}; test={2}".\
#    format(testsuite_name, testcase_name, test))
    t_s = JUnitTestSuite(testsuite_name,
                         test_engine_globals.TESTRUN_JUNIT_TESTCASE_DICTLISTS[testsuite_name])

    # Write out Test Suite Results
    #log_forced(junit_to_xml_report_string([t_s]))
    xml_out_filepath = ("{0}/{1}.xml".format(test_engine_globals.TESTOUTPUT_XMLDIRPATH,
                                             testsuite_name))

    with open(xml_out_filepath, 'w') as file_out:
        junit_to_xml_report_file(file_out, [t_s])
