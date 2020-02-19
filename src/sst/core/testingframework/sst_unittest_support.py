# -*- coding: utf-8 -*-

## Copyright 2009-2020 NTESS. Under the terms
## of Contract DE-NA0003525 with NTESS, the U.S.
## Government retains certain rights in this software.
##
## Copyright (c) 2009-2019, NTESS
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

import test_engine_globals
from test_engine_support import OSCommand
from test_engine_support import check_param_type

################################################################################

class SSTUnitTestCase(unittest.TestCase):
    """ This class the the SST Unittest class """

    def __init__(self, methodName):
        super(SSTUnitTestCase, self).__init__(methodName)

        # Save the path of the testsuite that is being run
        parent_module_path = os.path.dirname(sys.modules[self.__module__].__file__)
        self._test_suite_dir_path = parent_module_path

###

    def setUp(self):
        pass

###

    def tearDown(self):
        pass

###

    @classmethod
    def setUpClass(cls):
        test_engine_globals.TESTCASERUNNING = True

###

    @classmethod
    def tearDownClass(cls):
        test_engine_globals.TESTCASERUNNING = False

###

    def run_sst(self, sdl_file, out_file, other_params="", timeout=60):
        check_param_type("sdl_file", sdl_file, str)
        check_param_type("out_file", out_file, str)
        check_param_type("other_params", other_params, str)
        if not (isinstance(timeout, (int, float)) and not isinstance(timeout, bool)):
            raise ValueError("ERROR: Timeout must be an int or a float")

        # TODO Figure out how to set threads and ranks here
        oscmd = "sst {0}".format(sdl_file)
        log_debug("--SST Launch Command = {0}".format(oscmd))
        rtn = OSCommand(oscmd, out_file).run(timeout = timeout)
        err_str = "SST Timed-Out while running {0}".format(oscmd)
        self.assertFalse(rtn.timeout(), err_str)
        err_str = "SST returned {0}; while running {1}".format(rtn.result(), oscmd)
        self.assertEqual(rtn.result(), 0, err_str)

###

    def get_testsuite_dir(self):
        """ Return the directory path of the testsuite that is being run
           :return: str the path
        """
        return self._test_suite_dir_path

    def get_test_output_run_dir(self):
        """ Return the path of the output run directory
           :return: str the dir
        """
        return test_engine_globals.TESTOUTPUTRUNDIRPATH

    def get_test_output_tmp_dir(self):
        """ Return the path of the output run directory
           :return: str the dir
        """
        return test_engine_globals.TESTOUTPUTTMPDIRPATH

################################################################################
### Module level support
################################################################################

def test_engine_setup_module():
    """ Common calls for all modules for setup """
    pass

def test_engine_teardown_module():
    """ Common calls for all modules for teardown """
    pass

################################################################################
# Information Functions
################################################################################

def is_testing_in_debug_mode():
    """ Identify if test engine is in debug mode
       :return: True if in debug mode
    """
    return test_engine_globals.DEBUGMODE

################################################################################
# Logging Functions
################################################################################

def log(logstr):
    """ Log a message, this will not output unless we are
        outputing in verbose mode.
       :param: logstr = string to be logged
    """
    if test_engine_globals.VERBOSITY >= test_engine_globals.VERBOSE_LOUD:
        log_forced(logstr)

###

def log_forced(logstr):
    """ Log a message, no matter what the verbosity is
        if in the middle of testing, precede with a \n to slip in-between
        unittest outputs
        :param: logstr = string to be logged
    """
    extra_lf = ""
    if test_engine_globals.TESTCASERUNNING:
        extra_lf = "\n"
    print(("{0}{1}".format(extra_lf, logstr)))

###

def log_info(logstr):
    """ Log a INFO: message, no matter what the verbosity is
        :param: logstr = string to be logged
    """
    finalstr = "INFO: {0}".format(logstr)
    log_forced(finalstr)

###

def log_debug(logstr):
    """ Log a DEBUG: message, only if in debug verbosity mode
    """
    if test_engine_globals.DEBUGMODE:
        finalstr = "DEBUG: {0}".format(logstr)
        log_forced(finalstr)

###

def log_error(logstr):
    """ Log a ERROR: message, no matter what the verbosity is
        :param: logstr = string to be logged
    """
    finalstr = "ERROR: {0}".format(logstr)
    log_forced(finalstr)

###

def log_warning(logstr):
    """ Log a WARNING: message, no matter what the verbosity is
        :param: logstr = string to be logged
    """
    finalstr = "WARNING: {0}".format(logstr)
    log_forced(finalstr)

###

def log_fatal(errstr):
    """ Log a FATAL: message, no matter what the verbosity is
        THIS WILL KILL THE TEST ENGINE AND RETURN FAILURE
        :param: logstr = string to be logged
    """
    finalstr = "FATAL: {0}".format(errstr)
    log_forced(finalstr)
    sys.exit(1)

################################################################################
### OS Basic Commands
################################################################################

def os_ls(directory="."):
    """ TODO : DOCSTRING
    """
    cmd = "ls -lia {0}".format(directory)
    rtn = OSCommand(cmd).run()
    log("{0}".format(rtn.output()))

def os_cat(filepath):
    """ TODO : DOCSTRING
    """
    cmd = "cat {0}".format(filepath)
    rtn = OSCommand(cmd).run()
    log("{0}".format(rtn.output()))

################################################################################
