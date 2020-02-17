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
#import time
import subprocess
import threading
import traceback
import shlex
import unittest

import test_globals

#################################################

class SSTUnitTest(unittest.TestCase):
    """ This class the the SST Unittest class """

    def __init__(self, methodName):
        super(SSTUnitTest, self).__init__(methodName)

        parent_module_path = os.path.dirname(sys.modules[self.__module__].__file__)
        self._TESTSUITEDIRPATH = parent_module_path

###

    def get_testsuite_dir(self):
        """ Return the path of the testsuite being run
           :return: str the path
        """
        return self._TESTSUITEDIRPATH

###################################################
### OS Basic Commands
###################################################

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

###################################################
# Information Functions
###################################################

def is_testing_in_debug_mode():
    """ Identify if test engine is in debug mode
       :return: True if in debug mode
    """
    return test_globals.DEBUGMODE

def get_test_output_run_dir():
    """ Return the path of the output run directory
       :return: str the dir
    """
    return test_globals.TESTOUTPUTRUNDIRPATH

def get_test_output_tmp_dir():
    """ Return the path of the output run directory
       :return: str the dir
    """
    return test_globals.TESTOUTPUTTMPDIRPATH

###################################################
# Logging Functions
###################################################

def log(logstr):
    """ Log a message, this will not output unless we are
        outputing in verbose mode.
       :param: logstr = string to be logged
    """
    if test_globals.VERBOSITY >= test_globals.VERBOSE_LOUD:
        log_forced(logstr)

###

def log_forced(logstr):
    """ Log a message, no matter what the verbosity is
        :param: logstr = string to be logged
    """
    print(("{0}".format(logstr)))

###

def log_notice(logstr):
    """ Log a NOTICE: message, no matter what the verbosity is
        :param: logstr = string to be logged
    """
    finalstr = "NOTICE: {0}".format(logstr)
    log_forced(finalstr)

###

def log_debug(logstr):
    """ Log a DEBUG: message, only if in debug verbosity mode
    """
    if test_globals.DEBUGMODE:
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
################################################################################
################################################################################

class OSCommand():
    """ Enables to run subprocess commands in a different thread with TIMEOUT option.
        This will return a OSCommandResult object

        Based on a modified version of jcollado's solution:
        http://stackoverflow.com/questions/1191374/subprocess-with-timeout/4825933#4825933
    """
    _output_file_path = None
    _command = None
    _process = None
    _timeout = 60
    _run_status = None
    _run_output = ''
    _run_error = ''
    _run_timeout = False

    def __init__(self, command, output_file_path=None):
        if isinstance(command, str):
            if command != "":
                command = shlex.split(command)
            else:
                raise ValueError("ERROR: Command must not be empty")
        else:
            raise ValueError("ERROR: Command must be a string")
        self._command = command

        if output_file_path is not None:
            dirpath = os.path.abspath(os.path.dirname(output_file_path))
            if not os.path.exists(dirpath):
                err_str = "ERROR: Path to outputfile {0} is not valid".format(output_file_path)
                raise ValueError(err_str)
        self._output_file_path = output_file_path

    def run(self, timeout=60, **kwargs):
        """ Run a command then return and OSCmdRtn object. """
        def target(**kwargs):
            try:
                if self._output_file_path is not None:
                    with open(self._output_file_path, 'w+') as file_out:
                        self._process = subprocess.Popen(self._command, stdout=file_out, **kwargs)
                        self._run_output, self._run_error = self._process.communicate()
                        self._run_status = self._process.returncode
                else:
                    self._process = subprocess.Popen(self._command, **kwargs)
                    self._run_output, self._run_error = self._process.communicate()
                    self._run_status = self._process.returncode

                if self._run_output is None:
                    self._run_output = ""

                if self._run_error is None:
                    self._run_error = ""

            except:
                self._run_error = traceback.format_exc()
                self._run_status = -1

        if not (isinstance(self._timeout, (int, float)) and not isinstance(self._timeout, bool)):
            raise ValueError("ERROR: Timeout must be an int or a float")

        self._timeout = timeout

        # default stdout and stderr
        if 'stdout' not in kwargs and self._output_file_path is None:
            kwargs['stdout'] = subprocess.PIPE
        if 'stderr' not in kwargs:
            kwargs['stderr'] = subprocess.PIPE

        # thread
        thread = threading.Thread(target=target, kwargs=kwargs)
        thread.start()
        thread.join(self._timeout)
        if thread.is_alive():
            self._run_timeout = True
            self._process.terminate()
            thread.join()

        # Build a results cpass
        rtn = OSCommandResult(self._command, self._run_status, self._run_output,
                              self._run_error, self._run_timeout)
        return rtn

################################################################################
################################################################################
################################################################################

class OSCommandResult():
    """ TODO : DOCSTRING
    """
    _run_cmd = ''
    _run_status = None
    _run_output = ''
    _run_error = ''
    _run_timeout = False

    def __init__(self, cmd, status, output, error, timeout):
        self._run_cmd = cmd
        self._run_status = status
        self._run_output = output
        self._run_error = error
        self._run_timeout = timeout

    def __repr__(self):
        rtn_str = (("Cmd = {0}; Status = {1}; Timeout = {2}; ") +
                   ("Error = {3}; Output = {4}")).format(self._run_cmd, \
                    self._run_status, self._run_timeout, self._run_error, \
                    self._run_output)
        return rtn_str

    def __str__(self):
        return self.__repr__()

    def cmd(self):
        """ TODO : DOCSTRING
        """
        return self._run_cmd

    def result(self):
        """ TODO : DOCSTRING
        """
        return self._run_status

    def output(self):
        """ TODO : DOCSTRING
        """
        return self._run_output

    def error(self):
        """ TODO : DOCSTRING
        """
        return self._run_error

    def timeout(self):
        """ TODO : DOCSTRING
        """
        return self._run_timeout
