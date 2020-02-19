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

################################################################################

class SSTUnitTest(unittest.TestCase):
    """ This class the the SST Unittest class """

    def __init__(self, methodName):
        super(SSTUnitTest, self).__init__(methodName)

        # Save the path of the testsuite that is being run
        parent_module_path = os.path.dirname(sys.modules[self.__module__].__file__)
        self._TESTSUITEDIRPATH = parent_module_path

###

    def run_sst(self, sdl_file, out_file, other_params="", timeout=60):
        # TODO Figure out how to set threads and ranks here
        oscmd = "sst {0}".format(sdl_file)
        log_debug("--SST Launch Command = {0}".format(oscmd))
        rtn = OSCommand(oscmd, out_file).run()
        self.assertFalse(rtn.timeout(), "SST Timed-Out while running {0}".format(oscmd))
        self.assertEqual(rtn.result(), 0, "SST returned {0}; while running {1}".format(rtn.result(), oscmd))

###

    def get_testsuite_dir(self):
        """ Return the directory path of the testsuite that is being run
           :return: str the path
        """
        return self._TESTSUITEDIRPATH

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
        :param: logstr = string to be logged
    """
    print(("{0}".format(logstr)))

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
