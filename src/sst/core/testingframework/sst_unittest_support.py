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
import filecmp

import test_engine_globals
from test_engine_support import OSCommand
from test_engine_support import check_param_type
from test_engine_support import strclass

from test_engine_junit import JUnitTestCase
from test_engine_junit import JUnitTestSuite
from test_engine_junit import junit_to_xml_report_string
from test_engine_junit import junit_to_xml_report_file

################################################################################

class SSTTestCase(unittest.TestCase):
    """ This class is the SST TestCase class """

    def __init__(self, methodName):
        super(SSTTestCase, self).__init__(methodName)
        #log_forced("DEBUG SSTTestCase __init__")

        # Save the path of the testsuite that is being run
        test_engine_globals.TESTSUITE_NAME_STR = ("{0}".format(strclass(self.__class__)))
        self._testName = methodName

###

    def setUp(self):
        """ Called when the TestCase is starting up """
        #log_forced("DEBUG SSTTestCase setUp()")
        pass

###
    def tearDown(self):
        """ Called when the TestCase is shutting down """
        #log_forced("DEBUG SSTTestCase tearDown()")
        pass

###

    @classmethod
    def setUpClass(cls):
        #log_forced("DEBUG SSTTestCase setUpClass()")
        """ Called when the class is starting up """
        test_engine_globals.TESTRUNNINGFLAG = True
        parent_module_path = os.path.dirname(sys.modules[cls.__module__].__file__)
        test_engine_globals.TESTSUITEDIRPATH = parent_module_path

###

    @classmethod
    def tearDownClass(cls):
        #log_forced("DEBUG SSTTestCase tearDownClass()")
        """ Called when the class is shutting down """
        test_engine_globals.TESTRUNNINGFLAG = False

###

    def run_sst(self, sdl_file, out_file, other_params="", timeout_sec=60):
        """ TODO: Launch sst with with the command line and send output to the
            output file.  Other parameters can also be passed in.
           :param: sdl_file (str): The FilePath to the sdl file
           :param: out_file (str): The FilePath to the output file
           :param: other_params (str): Any other parameters used in the SST cmd
           :param: timeout_sec (int): Allowed runtime in seconds
        """
        #TODO: validate files exist
        check_param_type("sdl_file", sdl_file, str)
        check_param_type("out_file", out_file, str)
        check_param_type("other_params", other_params, str)
        if not (isinstance(timeout_sec, (int, float)) and not isinstance(timeout_sec, bool)):
            raise ValueError("ERROR: Timeout_sec must be an int or a float")

        # TODO Figure out how to set threads and ranks here
        oscmd = "sst {0} {1}".format(other_params, sdl_file)
        log_debug("--SST Launch Command = {0}".format(oscmd))
        rtn = OSCommand(oscmd, out_file).run(timeout_sec=timeout_sec)
        err_str = "SST Timed-Out ({0} secs) while running {1}".format(timeout_sec, oscmd)
        self.assertFalse(rtn.timeout(), err_str)
        err_str = "SST returned {0}; while running {1}".format(rtn.result(), oscmd)
        self.assertEqual(rtn.result(), 0, err_str)

################################################################################
### Module level support
################################################################################

def setUpModule():
    #log_forced("DEBUG: SSTTestCase setUpModule")
    test_engine_globals.JUNITTESTCASELIST = []

###

def tearDownModule():
    #log_forced("DEBUG: SSTTestCase tearDownModule")
    t_s = JUnitTestSuite(test_engine_globals.TESTSUITE_NAME_STR,
                         test_engine_globals.JUNITTESTCASELIST)

    # Write out Test Suite Results
    #log_forced(junit_to_xml_report_string([t_s]))
    xml_out_filepath = ("{0}/{1}.xml".format(test_engine_globals.TESTOUTPUTXMLDIRPATH,
                                             test_engine_globals.TESTSUITE_NAME_STR))

    with open(xml_out_filepath, 'w') as file_out:
        junit_to_xml_report_file(file_out, [t_s])

################################################################################
# Information Functions
################################################################################

def is_testing_in_debug_mode():
    """ Identify if test engine is in debug mode
       :return: True if in debug mode
    """
    return test_engine_globals.DEBUGMODE

#TODO: Get OS and Version Information Here

#TODO: Get SST-Core Build Information Here

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
    if test_engine_globals.TESTRUNNINGFLAG:
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
### Testing Directories
################################################################################

def get_testsuite_dir():
    """ Return the directory path of the testsuite that is being run
       :return: str the path
    """
    return test_engine_globals.TESTSUITEDIRPATH

###

def get_test_output_run_dir():
    """ Return the path of the output run directory
       :return: str the dir
    """
    return test_engine_globals.TESTOUTPUTRUNDIRPATH

###

def get_test_output_tmp_dir():
    """ Return the path of the output run directory
       :return: str the dir
    """
    return test_engine_globals.TESTOUTPUTTMPDIRPATH

################################################################################
### Testing Support
################################################################################

def compare_sorted(test_name, outfile, reffile):
   sorted_outfile = "{1}/{0}_sorted_outfile".format(test_name, get_test_output_tmp_dir())
   sorted_reffile = "{1}/{0}_sorted_reffile".format(test_name, get_test_output_tmp_dir())

   os.system("sort -o {0} {1}".format(sorted_outfile, outfile))
   os.system("sort -o {0} {1}".format(sorted_reffile, reffile))

   return filecmp.cmp(sorted_outfile, sorted_reffile)

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
