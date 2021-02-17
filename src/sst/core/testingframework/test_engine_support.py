# -*- coding: utf-8 -*-

## Copyright 2009-2021 NTESS. Under the terms
## of Contract DE-NA0003525 with NTESS, the U.S.
## Government retains certain rights in this software.
##
## Copyright (c) 2009-2021, NTESS
## All rights reserved.
##
## This file is part of the SST software package. For license
## information, see the LICENSE file in the top level directory of the
## distribution.

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

PY2 = sys.version_info[0] == 2
PY3 = sys.version_info[0] == 3

import test_engine_globals

################################################################################

class OSCommand():
    """ Enables to run subprocess commands in a different thread with a TIMEOUT option.
        This will return a OSCommandResult object.

        Based on a modified version of jcollado's solution:
        http://stackoverflow.com/questions/1191374/subprocess-with-timeout/4825933#4825933
    """
###

    def __init__(self, cmd_str, output_file_path=None, error_file_path=None,
                 set_cwd=None, use_shell=False):
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
        self._cmd_str = None
        self._process = None
        self._timeout_sec = 60
        self._run_status = None
        self._run_output = ''
        self._run_error = ''
        self._run_timeout = False
        self._use_shell = use_shell
        self._set_cwd = set_cwd
        self._validate_cmd_str(cmd_str)
        self._output_file_path = self._validate_output_path(output_file_path)
        self._error_file_path = self._validate_output_path(error_file_path)

####

    def run(self, timeout_sec=60, **kwargs):
        """ Run a command then return and OSCmdRtn object.

            Args:
                timeout_sec (int): The maximum runtime in seconds before thread
                                   will be terminated and a timeout error will occur.
                kwargs: Extra parameters
        """
        if not (isinstance(timeout_sec, (int, float)) and not isinstance(timeout_sec, bool)):
            raise ValueError("ERROR: Timeout must be an int or a float")

        self._timeout_sec = timeout_sec

        # Build the thread that will monitor the subprocess with a timeout
        thread = threading.Thread(target=self._run_cmd_in_subprocess, kwargs=kwargs)
        thread.start()
        thread.join(self._timeout_sec)
        if thread.is_alive():
            self._run_timeout = True
            self._process.kill()
            thread.join()

        # Build a OSCommandResult object to hold the results
        rtn = OSCommandResult(self._cmd_str, self._run_status, self._run_output,
                              self._run_error, self._run_timeout)
        return rtn

####

    def _run_cmd_in_subprocess(self, **kwargs):
        """ Run the command in a subprocess """
        file_out = None
        file_err = None

        try:
            # Run in either the users choosen directory or the run dir
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

    def _validate_output_path(self, file_path):
        """ Validate the output file path """
        if file_path is not None:
            dirpath = os.path.abspath(os.path.dirname(file_path))
            if not os.path.exists(dirpath):
                err_str = (("ERROR: OSCommand() Output path to file {0} ") +
                           ("is not valid")).format(file_path)
                raise ValueError(err_str)
        return file_path

################################################################################

class OSCommandResult():
    """ This class returns result data about the OSCommand that was executed """
    def __init__(self, cmd_str, status, output, error, timeout):
        """
            Args:
                cmd_str (str): The command to be executed
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
        """ return the command that was run """
        return self._run_cmd_str

####

    def result(self):
        """ return the run status result """
        return self._run_status

####

    def output(self):
        """ return the run output result """
        # Sometimes the output can be a unicode or a byte string - convert it
        if PY3:
            if type(self._run_output) is bytes:
                self._run_output = self._run_output.decode(encoding='UTF-8')
            return self._run_output
        else:
            return self._run_output.decode('utf-8')

####

    def error(self):
        """ return the run error output result """
        # Sometimes the output can be a unicode or a byte string - convert it
        if PY3:
            if type(self._run_error) is bytes:
                self._run_error = self._run_error.decode(encoding='UTF-8')
            return self._run_error
        else:
            return self._run_error.decode('utf-8')

####

    def timeout(self):
        """ return true if the run timed out """
        return self._run_timeout

################################################################################

def check_param_type(varname, vardata, datatype):
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

def strclass(cls):
    """ Return the classname of a class"""
    return "%s" % (cls.__module__)

def strqual(cls):
    """ Return the qualname of a class"""
    return "%s" % (_qualname(cls))

################################################################################
# qualname from https://github.com/wbolster/qualname to support Py2 and Py3
# LICENSE -> https://github.com/wbolster/qualname/blob/master/LICENSE.rst
#__all__ = ['qualname']

_cache = {}

def _qualname(obj):
    """Find out the qualified name for a class or function."""

    # For Python 3.3+, this is straight-forward.
    if hasattr(obj, '__qualname__'):
        return obj.__qualname__

    # For older Python versions, things get complicated.
    # Obtain the filename and the line number where the
    # class/method/function is defined.
    try:
        filename = inspect.getsourcefile(obj)
    except TypeError:
        return obj.__qualname__  # raises a sensible error
    if not filename:
        return obj.__qualname__  # raises a sensible error
    if inspect.isclass(obj):
        try:
            _, lineno = inspect.getsourcelines(obj)
        except (OSError, IOError):
            return obj.__qualname__  # raises a sensible error
    elif inspect.isfunction(obj) or inspect.ismethod(obj):
        if hasattr(obj, 'im_func'):
            # Extract function from unbound method (Python 2)
            obj = obj.im_func
        try:
            code = obj.__code__
        except AttributeError:
            code = obj.func_code
        lineno = code.co_firstlineno
    else:
        return obj.__qualname__  # raises a sensible error

    # Re-parse the source file to figure out what the
    # __qualname__ should be by analysing the abstract
    # syntax tree. Use a cache to avoid doing this more
    # than once for the same file.
    qualnames = _cache.get(filename)
    if qualnames is None:
        with open(filename, 'r') as filehandle:
            source = filehandle.read()
        node = ast.parse(source, filename)
        visitor = _Visitor()
        visitor.visit(node)
        _cache[filename] = qualnames = visitor.qualnames
    try:
        return qualnames[lineno]
    except KeyError:
        return obj.__qualname__  # raises a sensible error


class _Visitor(ast.NodeVisitor):
    """Support class for qualname function"""
    def __init__(self):
        super(_Visitor, self).__init__()
        self.stack = []
        self.qualnames = {}

    def store_qualname(self, lineno):
        """Support method for _Visitor class"""
        q_n = ".".join(n for n in self.stack)
        self.qualnames[lineno] = q_n

    def visit_FunctionDef(self, node):
        """Support method for _Visitor class"""
        self.stack.append(node.name)
        self.store_qualname(node.lineno)
        self.stack.append('<locals>')
        self.generic_visit(node)
        self.stack.pop()
        self.stack.pop()

    def visit_ClassDef(self, node):
        """Support method for _Visitor class"""
        self.stack.append(node.name)
        self.store_qualname(node.lineno)
        self.generic_visit(node)
        self.stack.pop()
