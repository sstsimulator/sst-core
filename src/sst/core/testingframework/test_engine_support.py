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

""" This module provides the low level calls to the OS shell
"""

import os
import subprocess
import threading
import traceback
import shlex
import inspect

################################################################################

class OSCommand():
    """ Enables to run subprocess commands in a different thread with TIMEOUT option.
        This will return a OSCommandResult object

        Based on a modified version of jcollado's solution:
        http://stackoverflow.com/questions/1191374/subprocess-with-timeout/4825933#4825933
    """
###

    def __init__(self, cmd_str, output_file_path=None):
        self._output_file_path = None
        self._cmd_str = None
        self._process = None
        self._timeout = 60
        self._run_status = None
        self._run_output = ''
        self._run_error = ''
        self._run_timeout = False
        self._validate_cmd_str(cmd_str)
        self._validate_output_path(output_file_path)

####

    def run(self, timeout=60, **kwargs):
        """ Run a command then return and OSCmdRtn object. """
        if not (isinstance(timeout, (int, float)) and not isinstance(timeout, bool)):
            raise ValueError("ERROR: Timeout must be an int or a float")

        self._timeout = timeout

        # If No output file defined, default stdout and stderr to normal output
        if 'stdout' not in kwargs and self._output_file_path is None:
            kwargs['stdout'] = subprocess.PIPE
        if 'stderr' not in kwargs and self._output_file_path is None:
            kwargs['stderr'] = subprocess.PIPE

        # Build the thread that will monitor the subprocess with a timeout
        thread = threading.Thread(target=self._run_cmd_in_subprocess, kwargs=kwargs)
        thread.start()
        thread.join(self._timeout)
        if thread.is_alive():
            self._run_timeout = True
            self._process.terminate()
            thread.join()

        # Build a OSCommandResult object to hold the results
        rtn = OSCommandResult(self._cmd_str, self._run_status, self._run_output,
                              self._run_error, self._run_timeout)
        return rtn

####

    def _run_cmd_in_subprocess(self, **kwargs):
        """ Run the command in a subprocess """
        try:
            # Run and dump stderr & stdout to the output file
            if self._output_file_path is not None:
                with open(self._output_file_path, 'w+') as file_out:
                    self._process = subprocess.Popen(self._cmd_str,
                                                     stdout=file_out,
                                                     stderr=file_out,
                                                     **kwargs)
                    self._run_output, self._run_error = self._process.communicate()
                    self._run_status = self._process.returncode
            else:
                # Run and dump stderr & stdout to the normal output
                self._process = subprocess.Popen(self._cmd_str, **kwargs)
                self._run_output, self._run_error = self._process.communicate()
                self._run_status = self._process.returncode

            if self._run_output is None:
                self._run_output = ""

            if self._run_error is None:
                self._run_error = ""

        except:
            self._run_error = traceback.format_exc()
            self._run_status = -1

####

    def _validate_cmd_str(self, cmd_str):
        """ Validate the cmd_str """
        if isinstance(cmd_str, str):
            if cmd_str != "":
                cmd_str = shlex.split(cmd_str)
            else:
                raise ValueError("ERROR: OSCommand() cmd_str must not be empty")
        else:
            raise ValueError("ERROR: OSCommand() cmd_str must be a string")
        self._cmd_str = cmd_str

####

    def _validate_output_path(self, output_file_path):
        """ Validate the cmd_str """
        if output_file_path is not None:
            dirpath = os.path.abspath(os.path.dirname(output_file_path))
            if not os.path.exists(dirpath):
                err_str = (("ERROR: OSCommand() Path to output file {0} ") +
                           ("is not valid")).format(output_file_path)
                raise ValueError(err_str)
        self._output_file_path = output_file_path

################################################################################

class OSCommandResult():
    """ TODO : DOCSTRING
    """
    def __init__(self, cmd_str, status, output, error, timeout):
        self._run_cmd_str = cmd_str
        self._run_status = status
        self._run_output = output
        self._run_error = error
        self._run_timeout = timeout

####

    def __repr__(self):
        rtn_str = (("Cmd = {0}; Status = {1}; Timeout = {2}; ") +
                   ("Error = {3}; Output = {4}")).format(self._run_cmd_str, \
                    self._run_status, self._run_timeout, self._run_error, \
                    self._run_output)
        return rtn_str

####

    def __str__(self):
        return self.__repr__()

####

    def cmd(self):
        """ TODO : DOCSTRING
        """
        return self._run_cmd_str

####

    def result(self):
        """ TODO : DOCSTRING
        """
        return self._run_status

####

    def output(self):
        """ TODO : DOCSTRING
        """
        return self._run_output

####

    def error(self):
        """ TODO : DOCSTRING
        """
        return self._run_error

####

    def timeout(self):
        """ TODO : DOCSTRING
        """
        return self._run_timeout

################################################################################

def check_param_type(varname, vardata, datatype):
    """ TODO : DOCSTRING
    """
    caller = inspect.stack()[1][3]
    if not isinstance(vardata, datatype):
        err_str = (("TEST-ERROR: {0}() param {1} = {2} is a not a {3}; it is a ") +
                   ("{4}")).format(caller, varname, vardata, datatype, type(vardata))
        raise ValueError(err_str)
