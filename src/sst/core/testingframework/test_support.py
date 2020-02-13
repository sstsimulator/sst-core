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

import sys
import os
#import time
import subprocess
import threading
import traceback
import shlex
import unittest

import test_globals

VERBOSE_QUIET  = 0
VERBOSE_NORMAL = 1
VERBOSE_LOUD   = 2
VERBOSE_DEBUG  = 3

################################################################################
# NOTE: initTestSuite() MUST BE CALLED IN THE setUpModule() function of a
#       testSuite.  For proper operation of the test system.
################################################################################

def initTestSuite(testSuiteFilePath):
    """ Initialize a Test Suite.
        :param: testSuiteFilePath = The file path of the running Test Suite
    """
    test_globals.testSuiteFilePath = testSuiteFilePath

#################################################

""" This class the the SST Unittest class """
class SSTUnitTest(unittest.TestCase):

    def __init__(self, methodName):
        super(SSTUnitTest, self).__init__(methodName)

        # Check to be sure TestSuite has been initialized
        self.assertNotEqual(test_globals.testSuiteFilePath == "",
                         ("TestSuite Not Initialized - Must call ") +
                         ("initTestSuite() in setUpModule()"))

### Path Information

    def isTestingInDebugMode(self):
        return test_globals.debugMode

    def getTestSuiteDir(self):
        return os.path.dirname(test_globals.testSuiteFilePath)

    def getTestOutputRunDir(self):
        return test_globals.testOutputRunDirPath

    def getTestOutputTmpDir(self):
        return test_globals.testOutputTmpDirPath

### OS Basic Commands

    def os_ls(self, directory = "."):
        cmd = "ls -lia {0}".format(directory)
        rtn = OSCommand(cmd).run()
        log("{0}".format(rtn.output()))

    def os_cat(self, filepath):
        cmd = "cat {0}".format(filepath)
        rtn = OSCommand(cmd).run()
        log("{0}".format(rtn.output()))

###################################################
# Global Logging Functions
###################################################

def log(logstr):
    """ Log a message, this will not output unless we are
        outputing in verbose mode.
       :param: logstr = string to be logged
    """
    if test_globals.verbosity >= VERBOSE_LOUD:
        logForced(logstr)

###

def logForced(logstr):
    """ Log a message, no matter what the verbosity is
        :param: logstr = string to be logged
    """
    print("{0}".format(logstr))

###

def logNotice(logstr):
    """ Log a NOTICE: message, no matter what the verbosity is
        :param: logstr = string to be logged
    """
    finalstr = "NOTICE: {0}".format(logstr)
    logForced(finalstr)

###

def logDebug(logstr):
    """ Log a DEBUG: message, only if in debug verbosity mode
    """
    if test_globals.debugMode:
        finalstr = "DEBUG: {0}".format(logstr)
        logForced(finalstr)

###

def logError(logstr):
    """ Log a ERROR: message, no matter what the verbosity is
        :param: logstr = string to be logged
    """
    finalstr = "ERROR: {0}".format(logstr)
    logForced(finalstr)

###

def logWarning(logstr):
    """ Log a WARNING: message, no matter what the verbosity is
        :param: logstr = string to be logged
    """
    finalstr = "WARNING: {0}".format(logstr)
    logForced(finalstr)

###

def logFatal(errstr):
    """ Log a FATAL: message, no matter what the verbosity is
        THIS WILL KILL THE TEST ENGINE AND RETURN FAILURE
        :param: logstr = string to be logged
    """
    finalstr = "FATAL: {0}".format(errstr)
    logForced(finalstr)
    sys.exit(1)

###################################################

class OSCommand(object):
    """
    Enables to run subprocess commands in a different thread with TIMEOUT option.
    This will return a OSCommandResult object

    Based on a modified version of jcollado's solution:
    http://stackoverflow.com/questions/1191374/subprocess-with-timeout/4825933#4825933
    """
    _outputfilepath = None
    _command        = None
    _process        = None
    _timeout        = 60
    _run_status     = None
    _run_output     = ''
    _run_error      = ''
    _run_timeout    = False

    def __init__(self, command, outputfilepath=None):
        if isinstance(command, basestring):
            if command != "":
                command = shlex.split(command)
            else:
                raise ValueError("ERROR: Command must not be empty")
        else:
            raise ValueError("ERROR: Command must be a string")
        self._command = command

        if outputfilepath != None:
            dirpath = os.path.abspath(os.path.dirname(outputfilepath))
            if not os.path.exists(dirpath):
                raise ValueError("ERROR: Path to outputfile {0} is not valid".format(outputfilepath))
        self._outputfilepath = outputfilepath

    def run(self, timeout=60, **kwargs):
        """ Run a command then return and OSCmdRtn object. """
        def target(**kwargs):
            try:
                if self._outputfilepath != None:
                    with open(self._outputfilepath, 'w+') as f:
                        self._process = subprocess.Popen(self._command, stdout=f, **kwargs)
                        self._run_output, self._run_error = self._process.communicate()
                        self._run_status = self._process.returncode
                else:
                        self._process = subprocess.Popen(self._command, **kwargs)
                        self._run_output, self._run_error = self._process.communicate()
                        self._run_status = self._process.returncode

                if self._run_output == None:
                    self._run_output = ""

                if self._run_error == None:
                    self._run_error = ""

            except:
                self._run_error = traceback.format_exc()
                self._run_status = -1

        if not (isinstance(self._timeout, (int, float)) and not isinstance(self._timeout, bool)):
            raise ValueError("ERROR: Timeout must be an int or a float")

        self._timeout = timeout

        # default stdout and stderr
        if 'stdout' not in kwargs and self._outputfilepath == None:
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

class OSCommandResult():
    _run_cmd        = ''
    _run_status     = None
    _run_output     = ''
    _run_error      = ''
    _run_timeout    = False

    def __init__(self, cmd, status, output, error, timeout):
        self._run_cmd        = cmd
        self._run_status     = status
        self._run_output     = output
        self._run_error      = error
        self._run_timeout    = timeout

    def __repr__(self):
        return "Cmd = {0}; Status = {1}; Timeout = {2}; Error = {3}; Output = {4}".format(
        self._run_cmd, self._run_status, self._run_timeout, self._run_error, self._run_output)

    def __str__(self):
        return self.__repr__()

    def cmd(self):
        return self._run_cmd

    def result(self):
        return self._run_status

    def output(self):
        return self._run_output

    def error(self):
        return self._run_error

    def timeout(self):
        return self._run_timeout





