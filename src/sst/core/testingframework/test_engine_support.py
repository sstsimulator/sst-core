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

""" This module provides the low level calls to the OS shell and other support
    functions
"""

import sys
import os
import subprocess
import threading
import traceback
import shlex
import ast
import inspect
import signal
from subprocess import TimeoutExpired
from typing import Any, Dict, List, Optional, Type

import test_engine_globals

################################################################################

class OSCommand:
    """ Enables to run subprocess commands in a different thread with a TIMEOUT option.
        This will return a OSCommandResult object.

        Based on a modified version of jcollado's solution:
        http://stackoverflow.com/questions/1191374/subprocess-with-timeout/4825933#4825933
    """
###

    def __init__(
        self,
        cmd_str: str,
        output_file_path: Optional[str] = None,
        error_file_path: Optional[str] = None,
        set_cwd: Optional[str] = None,
        use_shell: bool = False,
    ) -> None:
        """
            Args:
                cmd_str (str): The command to be executed
                output_file_path (str): The file path to send the std outpput from the cmd
                                        if None send to stdout.
                error_file_path (str): The file path to send the std error from the cmd
                                        if None send to stderr
                set_cwd (str): Path to change dir to before running cmd; if None then
                               use current working directory.
                use_shell (bool): Execute the cmd using the shell (not recommended)
        """
        self._output_file_path = None
        self._error_file_path = None
        self._cmd_str = [""]
        self._process: subprocess.Popen[str] = None  # type: ignore [assignment]
        self._timeout_sec = 60
        # Use an invalid return code rather than None to identify an
        # unintialized value.
        self._run_status_sentinel = -999
        self._run_status = self._run_status_sentinel
        self._run_output = ''
        self._run_error = ''
        self._run_timeout = False
        self._use_shell = use_shell
        self._set_cwd = set_cwd
        self._validate_cmd_str(cmd_str)
        self._output_file_path = self._validate_output_path(output_file_path)
        self._error_file_path = self._validate_output_path(error_file_path)
        self._signal = signal.NSIG
        self._signal_sec = 3
####

    def run(self,
        timeout_sec: int = 60,
        send_signal: int = signal.NSIG,
        signal_sec: int = 3,
        **kwargs: Any,
    ) -> "OSCommandResult":
        """ Run a command then return and OSCmdRtn object.

            Args:
                timeout_sec (int): The maximum runtime in seconds before thread
                                   will be terminated and a timeout error will occur.
                kwargs: Extra parameters e.g., timeout_sec to override the default timeout
        """
        check_param_type("timeout_sec", timeout_sec, int)

        self._timeout_sec = timeout_sec
        self._signal = send_signal
        self._signal_sec = signal_sec
        
        # Build the thread that will monitor the subprocess with a timeout
        thread = threading.Thread(target=self._run_cmd_in_subprocess, kwargs=kwargs)
        thread.start()
        thread.join(self._timeout_sec)
        if thread.is_alive():
            self._run_timeout = True
            assert self._process is not None
            self._process.kill()
            thread.join()

        # Build a OSCommandResult object to hold the results
        rtn = OSCommandResult(self._cmd_str, self._run_status, self._run_output,
                              self._run_error, self._run_timeout)
        return rtn

####

    def _run_cmd_in_subprocess(self, **kwargs: Any) -> None:
        """ Run the command in a subprocess """
        file_out = None
        file_err = None

        try:
            # Run in either the users chosen directory or the run dir
            if self._set_cwd is None:
                subprocess_path = test_engine_globals.TESTOUTPUT_RUNDIRPATH
            else:
                subprocess_path = os.path.abspath(self._set_cwd)

            # If No output files defined, default stdout and stderr to normal output
            if 'stdout' not in kwargs and self._output_file_path is None:
                kwargs['stdout'] = subprocess.PIPE
            if 'stderr' not in kwargs and self._error_file_path is None:
                kwargs['stderr'] = subprocess.PIPE

            # Create the stderr & stdout to the output files, if stderr path is
            # not defined, then use the normal output file
            if 'stdout' not in kwargs and self._output_file_path is not None:
                file_out = open(self._output_file_path, 'w+')
                kwargs['stdout'] = file_out
                if self._error_file_path is None:
                    kwargs['stderr'] = file_out
            if 'stderr' not in kwargs and self._error_file_path is not None:
                file_err = open(self._error_file_path, 'w+')
                kwargs['stderr'] = file_err

            self._process = subprocess.Popen(self._cmd_str,
                                             shell=self._use_shell,
                                             cwd = subprocess_path,
                                             **kwargs)
            
            if self._signal is not signal.NSIG:
                try:
                    self._run_output, self._run_error = self._process.communicate(timeout=self._signal_sec)
                except TimeoutExpired:
                    self._process.send_signal(self._signal)
           
            self._run_output, self._run_error = self._process.communicate()
            self._run_status = self._process.returncode

            if self._run_output is None:
                self._run_output = ""

            if self._run_error is None:
                self._run_error = ""

        except:
            self._run_error = traceback.format_exc()
            self._run_status = -1

        # Close any open files
        if file_out is not None:
            file_out.close()
        if file_err is not None:
            file_err.close()

####

    def _validate_cmd_str(self, cmd_str: str) -> None:
        """ Validate the cmd_str """
        if not isinstance(cmd_str, str):
            raise ValueError("ERROR: OSCommand() cmd_str must be a string")
        elif not cmd_str:
            raise ValueError("ERROR: OSCommand() cmd_str must not be empty")
        self._cmd_str = shlex.split(cmd_str)

####

    def _validate_output_path(self, file_path: Optional[str]) -> Optional[str]:
        """ Validate the output file path """
        if file_path is not None:
            dirpath = os.path.abspath(os.path.dirname(file_path))
            if not os.path.exists(dirpath):
                err_str = (("ERROR: OSCommand() Output path to file {0} ") +
                           ("is not valid")).format(file_path)
                raise ValueError(err_str)
        return file_path

################################################################################

class OSCommandResult:
    """ This class returns result data about the OSCommand that was executed """
    def __init__(self, cmd_str: List[str], status: int, output: str, error: str, timeout: bool) -> None:
        """
            Args:
                cmd_str (list[str]): The command to be executed
                status (int): The return status of the command execution.
                output (str): The standard output of the command execution.
                error (str): The error output of the command execution.
                timeout (bool): True if the command timed out during execution.
        """
        self._run_cmd_str = cmd_str
        self._run_status = status
        self._run_output = output
        self._run_error = error
        self._run_timeout = timeout

####

    def __repr__(self) -> str:
        rtn_str = (("Cmd = {0}; Status = {1}; Timeout = {2}; ") +
                   ("Error = {3}; Output = {4}")).format(self._run_cmd_str, \
                    self._run_status, self._run_timeout, self._run_error, \
                    self._run_output)
        return rtn_str

####

    def __str__(self) -> str:
        return self.__repr__()

####

    def cmd(self) -> List[str]:
        """ return the command that was run """
        return self._run_cmd_str

####

    def result(self) -> int:
        """ return the run status result """
        return self._run_status

####

    def output(self) -> str:
        """ return the run output result """
        # Sometimes the output can be a unicode or a byte string - convert it
        if isinstance(self._run_output, bytes):
            self._run_output = self._run_output.decode(encoding='UTF-8')
        return self._run_output

####

    def error(self) -> str:
        """ return the run error output result """
        # Sometimes the output can be a unicode or a byte string - convert it
        if isinstance(self._run_error, bytes):
            self._run_error = self._run_error.decode(encoding='UTF-8')
        return self._run_error

####

    def timeout(self) -> bool:
        """ return true if the run timed out """
        return self._run_timeout

################################################################################

def check_param_type(varname: str, vardata: Any, datatype: Type[Any]) -> None:
    """ Validate a parameter to ensure it is of the correct type.

        Args:
            varname (str) = The string name of the variable
            vardata (???) = The actual variable of any type
            datatype (???) = The type that we want to confirm

        Raises:
            ValueErr: variable is not of the correct type.
    """
    caller = inspect.stack()[1][3]
    if not isinstance(vardata, datatype):
        err_str = (("TEST-ERROR: {0}() param {1} = {2} is a not a {3}; it is a ") +
                   ("{4}")).format(caller, varname, vardata, datatype, type(vardata))
        print(err_str)
        raise ValueError(err_str)


################################################################################

def strclass(cls: Type[Any]) -> str:
    """ Return the classname of a class"""
    return "%s" % (cls.__module__)

def strqual(cls: Type[Any]) -> str:
    """ Return the qualname of a class"""
    return "%s" % (cls.__qualname__)
