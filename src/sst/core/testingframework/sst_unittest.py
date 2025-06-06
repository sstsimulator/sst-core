# -*- coding: utf-8 -*-

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

""" This module provides the basic SSTTestCase class (based upon Python's
    unittest.TestCase) for the SST Test Frameworks.

    SST Tests are expected to derive a class from SSTTestCase to create testssuites.
    Its general operation follows Pythons unittest Frameworks and users should
    reference Pythons documentation for more info.

    There are a number of methods within the class to provide support tools for the
    test developer.  More support functions exist within sst_unittest_support.py
"""

import multiprocessing
import os
import signal
import sys
import time
import unittest
from typing import Optional

import test_engine_globals
from sst_unittest_support import *
from test_engine_junit import JUnitTestSuite, junit_to_xml_report_file
from test_engine_support import check_param_type, strclass, strqual

if not sys.warnoptions:
    import warnings
    warnings.simplefilter("once") # Change the filter in this process
    os.environ["PYTHONWARNINGS"] = "once" # Also affect subprocesses

class SSTTestCase(unittest.TestCase):
    """ This class is main SSTTestCase class for the SST Testing Frameworks

        Developers must derive classes from this class to create a testsuite
        discovered by the testing frameworks.

        This class is derived from Python's unittest.TestCase which provides an
        basic resource for how to develop tests for this frameworks.
    """

    def __init__(self, methodName: str) -> None:
        # NOTE: __init__ is called at startup for all tests before any
        #       setUpModules(), setUpClass(), setUp() and the like are called.
        super().__init__(methodName)
        self.testname = methodName
        parent_module_path: str = os.path.dirname(sys.modules[self.__class__.__module__].__file__)  # type: ignore [assignment,type-var]
        self._testsuite_dirpath = parent_module_path
        self.initializeClass(self.testname)
        self._start_test_time = time.time()
        self._stop_test_time = time.time()

###

    def initializeClass(self, testname: str) -> None:
        """ The method is called by the Frameworks immediately before class is
        initialized.

        **NOTICE**:
            If a derived class defines its own copy of this method, this
            method (the parent method) MUST be called for proper operation
            of the testing frameworks

        **NOTE**:
            (Single Thread Testing) - Called by frameworks.
            (Concurrent Thread Testing) - Called by frameworks.

        Args:
            testname (str): Name of the test being initialized
        """
        # Placeholder method for overridden method in derived class
        #log_forced("\nSSTTestCase: initializeClass() - {0}".format(testname))
        from warnings import warn
        warn("initializeClass() is deprecated and will be removed in SST 15.",
             DeprecationWarning, stacklevel=2)

###

    def setUp(self) -> None:
        """ The method is called by the Frameworks immediately before a test is run

        **NOTICE**:
            If a derived class defines its own copy of this method, this
            method (the parent method) MUST be called for proper operation
            of the testing frameworks

        **NOTE**:
            (Single Thread Testing) - Called by frameworks.
            (Concurrent Thread Testing) - Called by frameworks.
        """
        #log_forced("SSTTestCase: setUp() - {0}".format(self.testname))
        if not test_engine_globals.TESTENGINE_CONCURRENTMODE:
            test_engine_globals.TESTRUN_TESTRUNNINGFLAG = True

        # TESTRUN_SINGTHREAD_TESTSUITE_NAME is used for non-concurrent
        # (ie single thread) runs
        test_engine_globals.TESTRUN_SINGTHREAD_TESTSUITE_NAME = self.get_testsuite_name()
        self._start_test_time = time.time()

###

    def tearDown(self) -> None:
        """ The method is called by the Frameworks immediately after a test finishes

        **NOTICE**:
            If a derived class defines its own copy of this method, this
            method (the parent method) MUST be called for proper operation
            of the testing frameworks

        **NOTE**:
            (Single Thread Testing) - Called by frameworks.
            (Concurrent Thread Testing) - Called by frameworks.
        """
        #log_forced("SSTTestCase: tearDown() - {0}".format(self.testname))
        if not test_engine_globals.TESTENGINE_CONCURRENTMODE:
            test_engine_globals.TESTRUN_TESTRUNNINGFLAG = False

        self._stop_test_time = time.time()

###

    @classmethod
    def setUpClass(cls) -> None:
        """ This method is called by the Frameworks immediately before the TestCase starts

        **NOTICE**:
            If a derived class defines its own copy of this method, this
            method (the parent method) MUST be called for proper operation
            of the testing frameworks

        **NOTE**:
            (Single Thread Testing): Called by frameworks.
            (Concurrent Thread Testing): NOT CALLED, NOT AVAILABLE.
        """
        #log_forced("SSTTestCase: setUpClass() - {0}".\
        #format(sys.modules[cls.__module__].__file__))

###

    @classmethod
    def tearDownClass(cls) -> None:
        """ This method is called by the Frameworks immediately after a TestCase finishes

        **NOTICE**:
            If a derived class defines its own copy of this method, this
            method (the parent method) MUST be called for proper operation
            of the testing frameworks

        **NOTE**:
            (Single Thread Testing) - Called by frameworks.
            (Concurrent Thread Testing) - NOT CALLED, NOT AVAILABLE.
        """
        #log_forced("SSTTestCase: tearDownClass() - {0}".\
        #format(sys.modules[cls.__module__].__file__))

###

    def get_testsuite_name(self) -> str:
        """ Return the testsuite (module) name

        Returns:
            (str) Testsuite (module) name
        """
        return "{0}".format(strclass(self.__class__))

###

    def get_testcase_name(self) -> str:
        """ Return the testcase name

        Returns:
            (str) testcase name
        """
        return "{0}".format(strqual(self.__class__))
###

    def get_testsuite_dir(self) -> str:
        """ Return the directory path of the testsuite that is being run

        Returns:
            (str) The path of the testsite directory
        """
        return self._testsuite_dirpath

###

    def get_test_output_run_dir(self) -> str:
        """ Return the path of the test output run directory

        Returns:
            (str) The path to the output run directory
        """
        return test_output_get_run_dir()

###

    def get_test_output_tmp_dir(self) -> str:
        """ Return the path of the test tmp directory

        Returns:
            (str) The path to the test tmp directory
        """
        return test_output_get_tmp_dir()

###

    def get_test_runtime_sec(self) -> float:
        """ Return the current runtime (walltime) of the test

        Returns:
            (float) The runtime of the test in seconds
        """
        self._stop_test_time = time.time()
        return self._stop_test_time - self._start_test_time


################################################################################
### Method to run an SST simulation
################################################################################

    def run_sst(
        self,
        sdl_file: str,
        out_file: str,
        err_file: Optional[str] = None,
        set_cwd: Optional[str] = None,
        mpi_out_files: str = "",
        other_args: str = "",
        num_ranks: Optional[int] = None,
        num_threads: Optional[int] = None,
        global_args: Optional[str] = None,
        timeout_sec: int = 120,
        expected_rc: int = 0,
        check_sdl_file: bool = True,
        send_signal: int = signal.NSIG,
        signal_sec: int = 3
    ) -> str:
        """ Launch sst with with the command line and send output to the
            output file.  The SST execution will be monitored for result errors and
            timeouts.  On an error or timeout, a SSTTestCase.assert() will be generated
            indicating the details of the failure.

            Args:
                sdl_file (str): The FilePath to the test SDL (python) file.
                out_file (str): The FilePath to the finalized output file.
                err_file (str): The FilePath to the finalized error file.
                                Default = same file as the output file.
                mpi_out_files (str): The FilePath to the mpi run output files.
                                     These will be merged into the out_file 
                                     at the end of a multi-rank run.
                other_args (str): Any other arguments used in the SST cmd
                                   that the caller wishes to use.
                num_ranks (int): The number of ranks to run SST with.
                num_threads (int): The number of threads to run SST with.
                global_args (str): Global Arguments provided from test engine args
                timeout_sec (int): Allowed runtime in seconds
                expected_rc (int): The expected return code from the SST run
                check_sdl_file (bool): If True, will check to make sure sdl file exists
                send_signal (signal): If not signal.NSIG, this signal will be sent to the SST process after signal_sec seconds
                signal_sec  (int): The number of seconds to wait before sending a signal to SST
            Returns:
                (str) The command string used to launch sst
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
            check_param_type("err_file", err_file, str)
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
        check_param_type("timeout_sec", timeout_sec, int)
        if expected_rc is not None:
            check_param_type("expected_rc", expected_rc, int)

        # Make sure sdl file is exists and is a file, if check_sdl_file flag is True
        if check_sdl_file:
            if not os.path.exists(sdl_file) or not os.path.isfile(sdl_file):
                log_error("sdl_file {0} does not exist".format(sdl_file))

        # Figure out a name for the mpi_output files if the default is provided
        if mpi_out_files == "":
            mpiout_filename = "{0}.testfile".format(out_file)
        else:
            mpiout_filename = mpi_out_files

        # Get the path to sst binary application
        sst_app_path = sstsimulator_conf_get_value('SSTCore', 'bindir', str, default="UNDEFINED")
        err_str = "Path to SST {0}; does not exist...".format(sst_app_path)
        self.assertTrue(os.path.isdir(sst_app_path), err_str)

        # Set the initial os launch command for sst.
        # If multi-threaded, include the number of threads
        if num_threads > 1:
            oscmd = "{0}/sst -n {1} {2} {3} {4}".format(sst_app_path,
                                                        num_threads,
                                                        global_args,
                                                        other_args,
                                                        sdl_file)
        else:
            oscmd = "{0}/sst {1} {2} {3}".format(sst_app_path,
                                                 global_args,
                                                 other_args,
                                                 sdl_file)

        # Update the os launch command if we are running multi-rank
        num_cores = multiprocessing.cpu_count()

        # Perform any multi-rank checks/setup
        mpi_avail = False
        numa_param = ""
        if num_ranks > 1:
            # Check to see if mpirun is available
            rtn_mpirun = os.system("which mpirun > /dev/null 2>&1")
            if rtn_mpirun == 0:
                mpi_avail = True

            numa_param = "-map-by numa:PE={0}".format(num_threads)

            oscmd = "mpirun -np {0} {1} --output-filename {2} {3}".format(num_ranks,
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

        # Check for OpenMPI
        if num_ranks > 1 and mpi_avail is False:
            log_fatal("OpenMPI IS NOT FOUND/AVAILABLE")

        # Launch SST
        rtn = os_command(oscmd, output_file_path = out_file,
                        error_file_path = err_file,
                        set_cwd = set_cwd).run(timeout_sec=timeout_sec, send_signal=send_signal, signal_sec=signal_sec)
        if num_ranks > 1:
            testing_merge_mpi_files("{0}*".format(mpiout_filename), mpiout_filename, out_file, errorfilepath=err_file)


        # Look for runtime error conditions
        err_str = "SST Timed-Out ({0} secs) while running {1}".format(timeout_sec, oscmd)
        self.assertFalse(rtn.timeout(), err_str)
        if expected_rc:
            err_str = "SST returned {0}; while running {1}".format(rtn.result(), oscmd)
            self.assertEqual(rtn.result(), expected_rc, err_str)

        # Return the command used to launch SST
        return oscmd

################################################################################
### Module level support
################################################################################

def setUpModule() -> None:
    """ Perform setup functions before the testing Module loads.

        This function is called by the Frameworks before tests in any TestCase
        defined in the module are run.  This function is only called during
        Single Thread testing mode.

        **NOTICE**:
            This function is part of the testing Frameworks infrastructure
            and should not need to be defined by testsuites.  However, If a testing module
            must define its own copy of this function, this function MUST be called
            for proper operation of the testing frameworks

        **NOTE**:
            This function will only be called when the test frameworks is running
            in Single Thread testing mode.  It will NOT be called in Concurrent Thread
            testing mode.
    """
    #log_forced("SSTTestCase: setUpModule() - {0}".format(__file__))
    test_engine_globals.TESTRUN_JUNIT_TESTCASE_DICTLISTS['singlethread'] = []

###

def tearDownModule() -> None:
    """ Perform teardown functions immediately after a testing Module finishes.

        This function is called by the Frameworks after all tests in all TestCases
        defined in the module.  This function is only called during Single Thread
        testing mode.

        **NOTICE**:
            This function is part of the testing Frameworks infrastructure
            and should not need to be defined by testsuites.  However, If a testing module
            must define its own copy of this function, this function MUST be called
            for proper operation of the testing frameworks

        **NOTE**:
            This function will only be called when the test frameworks is running
            in Single Thread testing mode.  It will NOT be called in Concurrent Thread
            testing mode.
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

def setUpModuleConcurrent(test: SSTTestCase) -> None:
    """ Perform setup functions before the testing Module loads.

        This function is called by the Frameworks before tests in any TestCase
        defined in the module are run.  This function is only called during
        Concurrent Threading testing mode.

        **NOTICE**:
            This function is part of the testing Frameworks infrastructure
            and should not need to be defined by testsuites.  However, If a testing module
            must define its own copy of this function, this function MUST be called
            for proper operation of the testing frameworks

        **NOTE**:
            This function will only be called when the test frameworks is running
            in Concurrent Threading testing mode.  It will NOT be called in Single thread
            testing mode.
    """

    testsuite_name = test.get_testsuite_name()
#    testcase_name = test.get_testcase_name()
#    log_forced("\nSSTTestCase - setUpModuleConcurrent suite={0}; case={1}; test={2}".\
#    format(testsuite_name, testcase_name, test))
    if not testsuite_name in test_engine_globals.TESTRUN_JUNIT_TESTCASE_DICTLISTS:
        test_engine_globals.TESTRUN_JUNIT_TESTCASE_DICTLISTS[testsuite_name] = []

###

def tearDownModuleConcurrent(test: SSTTestCase) -> None:
    """ Perform teardown functions immediately after a testing Module finishes.

        This function is called by the Frameworks after all tests in all TestCases
        defined in the module.  This function is only called during Concurrent
        Threading testing mode

        **NOTICE**:
            This function is part of the testing Frameworks infrastructure
            and should not need to be defined by testsuites.  However, If a testing module
            must define its own copy of this function, this function MUST be called
            for proper operation of the testing frameworks

        **NOTE**:
            This function will only be called when the test frameworks is running
            in Concurrent Thread testing mode.  It will NOT be called in Single thread
            testing mode.
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
