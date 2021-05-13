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

""" This module provides the a large number of support functions available
    for tests within a SSTTestCase.
"""

import sys
import os
import unittest
import platform
import re
import time
import multiprocessing
import tarfile
import shutil

PY2 = sys.version_info[0] == 2
PY3 = sys.version_info[0] == 3

# ConfigParser module changes name between Py2->Py3
if PY3:
    import configparser
else:
    import ConfigParser as configparser

import test_engine_globals
from test_engine_support import OSCommand
from test_engine_support import check_param_type

################################################################################

OS_DIST_OSX = "OSX"
OS_DIST_CENTOS = "CENTOS"
OS_DIST_RHEL = "RHEL"
OS_DIST_TOSS = "TOSS"
OS_DIST_UBUNTU = "UBUNTU"
OS_DIST_UNDEF = "UNDEFINED"

################################################################################

pin_exec_path = ""

################################################################################

class SSTTestCaseException(Exception):
    """ Generic Exception support for SSTTestCase
    """
    def __init__(self, value):
        super(SSTTestCaseException, self).__init__(value)
        self.value = value
    def __str__(self):
        return repr(self.value)

################################################################################

################################################################################
# Commandline Information Functions
################################################################################

def testing_check_is_in_debug_mode():
    """ Identify if test frameworks is in debug mode

        Returns:
            (bool) True if test frameworks is in debug mode
    """
    return test_engine_globals.TESTENGINE_DEBUGMODE

###

def testing_check_is_in_log_failures_mode():
    """ Identify if test frameworks is in log failures mode

        Returns:
            (bool) True if test frameworks is in log failures mode
    """
    return test_engine_globals.TESTENGINE_LOGFAILMODE

###

def testing_check_is_in_concurrent_mode():
    """ Identify if test frameworks is in concurrent mode

        Returns:
            (bool) True if test frameworks is in concurrent mode
    """
    return test_engine_globals.TESTENGINE_CONCURRENTMODE

###

def testing_check_get_num_ranks():
    """ Get the number of ranks defined to be run during testing

        Returns:
            (int) The number of requested run ranks
    """
    return test_engine_globals.TESTENGINE_SSTRUN_NUMRANKS

###

def testing_check_get_num_threads():
    """ Get the number of threads defined to be run during testing

        Returns:
            (int) The number of requested run threads
    """
    return test_engine_globals.TESTENGINE_SSTRUN_NUMTHREADS

################################################################################
# PIN Information Functions
################################################################################

def testing_is_PIN_loaded():
    # Look to see if PIN is available
    pindir_found = False
    pin_path = os.environ.get('INTEL_PIN_DIRECTORY')
    if pin_path is not None:
        pindir_found = os.path.isdir(pin_path)
    #log_debug("testing_is_PIN_loaded() - Intel_PIN_Path = {0}; Valid Dir = {1}".format(pin_path, pindir_found))
    return pindir_found

def testing_is_PIN_Compiled():
    global pin_exec_path
    pin_crt = sst_elements_config_include_file_get_value_int("HAVE_PINCRT", 0, True)
    pin_exec = sst_elements_config_include_file_get_value_str("PINTOOL_EXECUTABLE", "", True)
    #log_debug("testing_is_PIN_Compiled() - Detected PIN_CRT = {0}".format(pin_crt))
    #log_debug("testing_is_PIN_Compiled() - Detected PIN_EXEC = {0}".format(pin_exec))
    pin_exec_path = pin_exec
    rtn = pin_exec != ""
    #log_debug("testing_is_PIN_Compiled() - Rtn {0}".format(rtn))
    return rtn

def testing_is_PIN2_used():
    global pin_exec_path
    if testing_is_PIN_Compiled():
        if "/pin.sh" in pin_exec_path:
            #log_debug("testing_is_PIN2_used() - Rtn True because pin.sh is found")
            return True
        else:
            #log_debug("testing_is_PIN2_used() - Rtn False because pin.sh not found")
            return False
    else:
        #log_debug("testing_is_PIN2_used() - Rtn False because PIN Not Compiled")
        return False

def testing_is_PIN3_used():
    global pin_exec_path
    if testing_is_PIN_Compiled():
        if testing_is_PIN2_used():
            #log_debug("testing_is_PIN3_used() - Rtn False because PIN2 is used")
            return False
        else:
            # Make sure pin is at the end of the string
            pinstr = "/pin"
            idx = pin_exec_path.rfind(pinstr)
            if idx == -1:
                #log_debug("testing_is_PIN3_used() - Rtn False because 'pin' is not in path")
                return False
            else:
                found_pin = (idx+len(pinstr)) == len(pin_exec_path)
                #log_debug("testing_is_PIN3_used() - Rtn {0} comparing path lengths".format(found_pin))
                return found_pin
    else:
        #log_debug("testing_is_PIN3_used() - Rtn False because PIN Not Compiled")
        return False

################################################################################
# System Information Functions
################################################################################

def testing_check_is_py_2():
    """ Check if Test Frameworks is running via Python Version 2

        Returns:
            (bool) True if test frameworks is being run by Python Version 2
    """
    return PY2

def testing_check_is_py_3():
    """ Check if Test Frameworks is running via Python Version 3

        Returns:
            (bool) True if test frameworks is being run by Python Version 3
    """
    return PY3

###

def host_os_get_system_node_name():
    """ Get the node name of the system

        Returns:
            (str) Returns the system node name
    """
    return platform.node()

###

def host_os_get_kernel_type():
    """ Get the Kernel Type

        Returns:
            (str) 'Linux' or 'Darwin' as the Kernel Type
    """
    return platform.system()

def host_os_get_kernel_release():
    """ Get the Kernel Release number

        Returns:
            (str) Kernel Release Number.  Note: This is not the same as OS version
    """
    return platform.release()

def host_os_get_kernel_arch():
    """ Get the Kernel System Arch

        Returns:
            (str) 'x86_64' on Linux; 'i386' on OSX as the Kernel Architecture
    """
    return platform.machine()

def host_os_get_distribution_type():
    """ Get the os distribution type

        Returns:
            (str)
            'OSX' for Mac OSX;
            'CENTOS' CentOS;
            'RHEL' for Red Hat Enterprise Linux;
            'TOSS' for Toss;
            'UBUNTU' for Ubuntu;
            'UNDEFINED' an undefined OS.
    """
    k_type = host_os_get_kernel_type()
    if k_type == 'Linux':
        lin_dist = _get_linux_distribution()
        dist_name = lin_dist[0].lower()
        if "centos" in dist_name:
            return OS_DIST_CENTOS
        if "red hat" in dist_name:
            return OS_DIST_RHEL
        if "toss" in dist_name:
            return OS_DIST_TOSS
        if "ubuntu" in dist_name:
            return OS_DIST_UBUNTU
    elif k_type == 'Darwin':
        return OS_DIST_OSX
    return OS_DIST_UNDEF

def host_os_get_distribution_version():
    """ Get the os distribution version

        Returns:
            (str) The OS distribution version
    """
    k_type = host_os_get_kernel_type()
    if k_type == 'Linux':
        lin_dist = _get_linux_distribution()
        return lin_dist[1]
    if k_type == 'Darwin':
        mac_ver = platform.mac_ver()
        return mac_ver[0]
    return "Undefined"

###

def host_os_is_osx():
    """ Check if OS distribution is OSX

        Returns:
            (bool) True if OS Distribution is OSX
    """
    return host_os_get_distribution_type() == OS_DIST_OSX

def host_os_is_linux():
    """ Check if OS distribution is Linux

        Returns:
            (bool) True if OS Distribution is Linux
    """
    return not host_os_get_distribution_type() == OS_DIST_OSX

def host_os_is_centos():
    """ Check if OS distribution is CentOS

        Returns:
            (bool) True if OS Distribution is CentOS
    """
    return host_os_get_distribution_type() == OS_DIST_CENTOS

def host_os_is_rhel():
    """ Check if OS distribution is RHEL

        Returns:
            (bool) True if OS Distribution is RHEL
    """
    return host_os_get_distribution_type() == OS_DIST_RHEL

def host_os_is_toss():
    """ Check if OS distribution is Toss

        Returns:
            (bool) True if OS Distribution is Toss
    """
    return host_os_get_distribution_type() == OS_DIST_TOSS

def host_os_is_ubuntu():
    """ Check if OS distribution is Ubuntu

        Returns:
            (bool) True if OS Distribution is Ubuntu
    """
    return host_os_get_distribution_type() == OS_DIST_UBUNTU

###

def host_os_get_num_cores_on_system():
    """ Get number of cores on the system

        Returns:
            (int) Number of cores on the system
    """
    num_cores = multiprocessing.cpu_count()
    return num_cores

################################################################################
# SST Skipping Support
################################################################################

def testing_check_is_scenario_filtering_enabled(scenario_name):
    """ Detirmine if a scenario filter name is enabled

        Args:
            scenario_name (str): The scenario filter name to check

        Returns:
           (bool) True if the scenario filter name is enabled
    """
    check_param_type("scenario_name", scenario_name, str)
    return scenario_name in test_engine_globals.TESTENGINE_SCENARIOSLIST

###

def skip_on_scenario(scenario_name, reason):
    """ Skip a test if a scenario filter name is enabled

        Args:
            scenario_name (str): The scenario filter name to check
            reason (str): The reason for the skip
    """
    check_param_type("scenario_name", scenario_name, str)
    check_param_type("reason", reason, str)
    if not testing_check_is_scenario_filtering_enabled(scenario_name):
        return lambda func: func
    return unittest.skip(reason)

###

def skip_on_sstsimulator_conf_empty_str(section, key, reason):
    """ Skip a test if a section/key in the sstsimulator.conf file is missing an
        entry

        Args:
            section (str): The section in the sstsimulator.conf to check
            key (str): The key in the sstsimulator.conf to check
            reason (str): The reason for the skip
    """
    check_param_type("section", section, str)
    check_param_type("key", key, str)
    check_param_type("reason", reason, str)
    rtn_str = sstsimulator_conf_get_value_str(section, key, "")
    if rtn_str != "":
        return lambda func: func
    return unittest.skip(reason)

################################################################################
# SST Core Configuration include file (sst_config.h.conf) Access Functions
################################################################################

def sst_core_config_include_file_get_value_int(define, default=None, disable_warning = False):
    """ Retrieve a define from the SST Core Configuration Include File (sst_config.h)

        Args:
            define (str): The define to look for
            default (int): Default Return if failure occurs
            disable_warning (bool): Disable logging the warning if define is not found

        Returns:
            (int) The returned data or default if not found in the include file

        Raises:
            SSTTestCaseException: if type is incorrect OR no data AND default
                                  is not provided
    """
    return _get_sst_config_include_file_value(test_engine_globals.TESTENGINE_CORE_CONFINCLUDE_DICT,
                                              "sst_config.h", define, default, int, disable_warning)

###

def sst_core_config_include_file_get_value_str(define, default=None, disable_warning = False):
    """ Retrieve a define from the SST Core Configuration Include File (sst_config.h)

        Args:
            define (str): The define to look for
            default (str): Default Return if failure occurs
            disable_warning (bool): Disable logging the warning if define is not found

        Returns:
            (str) The returned data or default if not found in the include file

        Raises:
            SSTTestCaseException: if type is incorrect OR no data AND default
                                  is not provided
    """
    return _get_sst_config_include_file_value(test_engine_globals.TESTENGINE_CORE_CONFINCLUDE_DICT,
                                              "sst_config.h", define, default, str, disable_warning)

################################################################################
# SST Elements Configuration include file (sst_element_config.h.conf) Access Functions
################################################################################

def sst_elements_config_include_file_get_value_int(define, default=None, disable_warning = False):
    """ Retrieve a define from the SST Elements Configuration Include File (sst_element_config.h)

        Args:
            define (str): The define to look for
            default (int): Default Return if failure occurs
            disable_warning (bool): Disable logging the warning if define is not found

        Returns:
            (int) The returned data or default if not found in the include file

        Raises:
            SSTTestCaseException: if type is incorrect OR no data AND default
                                  is not provided
    """
    return _get_sst_config_include_file_value(test_engine_globals.TESTENGINE_ELEM_CONFINCLUDE_DICT,
                                              "sst_element_config.h", define, default, int, disable_warning)

###

def sst_elements_config_include_file_get_value_str(define, default=None, disable_warning = False):
    """ Retrieve a define from the SST Elements Configuration Include File (sst_element_config.h)

        Args:
            define (str): The define to look for
            default (str): Default Return if failure occurs
            disable_warning (bool): Disable logging the warning if define is not found

        Returns:
            (str) The returned data or default if not found in the include file

        Raises:
            SSTTestCaseException: if type is incorrect OR no data AND default
                                  is not provided
    """
    return _get_sst_config_include_file_value(test_engine_globals.TESTENGINE_ELEM_CONFINCLUDE_DICT,
                                              "sst_element_config.h", define, default, str, disable_warning)

################################################################################
# SST Configuration file (sstsimulator.conf) Access Functions
################################################################################

def sstsimulator_conf_get_value_str(section, key, default=None):
    """ Retrieve a Section/Key from the SST Configuration File (sstsimulator.conf)

        Args:
            section (str): The [section] to look for the key
            key (str): The key to find
            default (str): Default Return if failure occurs

        Returns:
            (str) The returned data or default if not found in the config file

        Raises:
            SSTTestCaseException: if no data AND default is not provided
    """
    return _get_sstsimulator_conf_value(section, key, default, str)

###

def sstsimulator_conf_get_value_int(section, key, default=None):
    """ Retrieve a Section/Key from the SST Configuration File (sstsimulator.conf)

        Args:
            section (str): The [section] to look for the key
            key (str): The key to find
            default (int): Default Return if failure occurs

        Returns:
            (int) The returned data or default if not found in the config file

        Raises:
            SSTTestCaseException: if no data AND default is not provided
    """
    return _get_sstsimulator_conf_value(section, key, default, int)

###

def sstsimulator_conf_get_value_float(section, key, default=None):
    """ Retrieve a Section/Key from the SST Configuration File (sstsimulator.conf)

        Args:
            section (str): The [section] to look for the key
            key (str): The key to find
            default (float): Default Return if failure occurs

        Returns:
            (float) The returned data or default if not found in the config file

        Raises:
            SSTTestCaseException: if no data AND default is not provided
    """
    return _get_sstsimulator_conf_value(section, key, default, float)

###

def sstsimulator_conf_get_value_bool(section, key, default=None):
    """ Retrieve a Section/Key from the SST Configuration File (sstsimulator.conf)

        NOTE: "1", "yes", "true", and "on" will return True;
              "0", "no", "false", and "off" willl return False

        Args:
            section (str): The [section] to look for the key
            key (str): The key to find
            default (bool): Default Return if failure occurs

        Returns:
            (bool) The returned data or default if not found in the config file

        Raises:
            SSTTestCaseException: if no data AND default is not provided
    """
    return _get_sstsimulator_conf_value(section, key, default, bool)

###

def sstsimulator_conf_get_sections():
    """ Retrieve a list of sections that exist in the SST Configuration File (sstsimulator.conf)

        Returns:
            (list of str) The list of sections in the file

        Raises:
            SSTTestCaseException: If an error occurs
    """
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        return core_conf_file_parser.sections()
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def sstsimulator_conf_get_section_keys(section):
    """ Retrieve a list of keys under a section that exist in the
        SST Configuration File  (sstsimulator.conf)

        Args:
            section (str): The [section] to look for the list of keys

        Returns:
            (list of str) The list of keys under a section in the file

        Raises:
            SSTTestCaseException: If an error occurs
    """
    check_param_type("section", section, str)
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        return core_conf_file_parser.options(section)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def sstsimulator_conf_get_all_keys_values_from_section(section):
    """ Retrieve a list of tuples that contain all the key - value pairs
        under a section that exists in the SST Configuration File (sstsimulator.conf)

        Args:
            section (str): The [section] to look for the key

        Returns:
            (list of tuples of str) The list of tuples of key - value pairs

        Raises:
            SSTTestCaseException: If an error occurs
    """
    check_param_type("section", section, str)
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        return core_conf_file_parser.items(section)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def sstsimulator_conf_does_have_section(section):
    """ Check if the SST Configuration File (sstsimulator.conf) has a defined
        section

        Args:
            section (str): The [section] to look for

        Returns:
            (bool) True if the [section] is found

        Raises:
            SSTTestCaseException: If an error occurs
    """
    check_param_type("section", section, str)
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        return core_conf_file_parser.has_section(section)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def sstsimulator_conf_does_have_key(section, key):
    """ Check if the SST Configuration File (sstsimulator.conf) has a defined key
        within a section
        Args:
            section (str): The [section] within to search for the key
            key (str): The key to search for

        Returns:
            (bool) True if the key exists within the [section]

        Raises:
            SSTTestCaseException: If an error occurs
    """
    check_param_type("section", section, str)
    check_param_type("key", key, str)
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        return core_conf_file_parser.has_option(section, key)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

################################################################################
# Logging Functions
################################################################################

def log(logstr):
    """ Log a general message.

        This will not output unless we are outputing in >= normal mode.

        Args:
            logstr (str): string to be logged
    """
    check_param_type("logstr", logstr, str)
    if test_engine_globals.TESTENGINE_VERBOSITY >= test_engine_globals.VERBOSE_NORMAL:
        log_forced(logstr)

###

def log_forced(logstr):
    """ Log a general message, no matter what the verbosity is.

        if in the middle of testing, it will precede with a '\\n' to slip
        in-between unittest outputs

        Args:
            logstr (str): string to be logged
    """
    check_param_type("logstr", logstr, str)
    extra_lf = ""
    if test_engine_globals.TESTRUN_TESTRUNNINGFLAG:
        extra_lf = "\n"
    print(("{0}{1}".format(extra_lf, logstr)))

###

def log_debug(logstr):
    """ Log a 'DEBUG:' message.

        Log will only happen if in debug verbosity mode.

        Args:
            logstr (str): string to be logged
    """
    if test_engine_globals.TESTENGINE_DEBUGMODE:
        finalstr = "DEBUG: {0}".format(logstr)
        log_forced(finalstr)

###

def log_failure(logstr):
    """ Log a test failure.

        Log will only happen if in log failure mode.
        NOTE: Testcases may call log_failure to log any test failure data.  Log
              Log output will only occur if log failure mode is enabled.
              The intent of this function is to allow tests to log failure info
              (useful for troubleshooting) during a run if the 'log failure mode'
              is enabled.

        Args:
            logstr (str): string to be logged
    """
    if test_engine_globals.TESTENGINE_LOGFAILMODE:
        finalstr = "{0}".format(logstr)
        log_forced(finalstr)

###

def log_info(logstr, forced=True):
    """ Log a 'INFO:' message.

        Args:
            logstr (str): string to be logged
            forced (bool): If true Always force the logging regardless of verbosity;
                           otherwise, perform a normal log.
    """
    check_param_type("logstr", logstr, str)
    finalstr = "INFO: {0}".format(logstr)
    if forced:
        log_forced(finalstr)
    else:
        log(finalstr)
###

def log_error(logstr):
    """ Log a 'ERROR:' message.

        Log will occur no matter what the verbosity is

        Args:
            logstr (str): string to be logged
    """
    check_param_type("logstr", logstr, str)
    finalstr = "ERROR: {0}".format(logstr)
    log_forced(finalstr)
    test_engine_globals.TESTENGINE_ERRORCOUNT += 1

###

def log_warning(logstr):
    """ Log a 'WARNING:' message.

        Log will occur no matter what the verbosity is

        Args:
            logstr (str): string to be logged
    """
    check_param_type("logstr", logstr, str)
    finalstr = "WARNING: {0}".format(logstr)
    log_forced(finalstr)

###

def log_fatal(errstr):
    """ Log a 'FATAL:' message.

        Log will occur no matter what the verbosity is and
        THIS WILL KILL THE TEST ENGINE AND RETURN FAILURE

        Args:
            errstr (str): string to be logged
    """
    check_param_type("errstr", errstr, str)
    finalstr = "FATAL: {0}".format(errstr)
    log_forced(finalstr)
    sys.exit(2)

###

def log_testing_note(note_str):
    """ Log a testing note

        Add a testing note that will be displayed at the end of the test run
        in the results section.  These notes can be used for adding any
        info/notes/observations that is desired to be reviewed by the user at the
        end of the test run.  Adding a note has no impact on the Pass/Fail status
        of any test.  Also, if debug mode is enabled, the note will be logged
        to the console.  The text "NOTE : " will be prepended to the front of the
        note_str.

        Args:
            note_str (str): string to be added to notes list
    """
    check_param_type("note_str", note_str, str)
    final_note = "NOTE: {0}".format(note_str)
    test_engine_globals.TESTENGINE_TESTNOTESLIST.append(final_note)
    log_debug(final_note)

################################################################################
### Testing Directories
################################################################################

def test_output_get_run_dir():
    """ Return the path of the output run directory

        Returns:
            (str) Path to the output run directory
    """
    return test_engine_globals.TESTOUTPUT_RUNDIRPATH

###

def test_output_get_tmp_dir():
    """ Return the path of the output tmp directory

        Returns:
            (str) Path to the output tmp directory
    """
    return test_engine_globals.TESTOUTPUT_TMPDIRPATH

################################################################################
### Testing Support
################################################################################

def testing_compare_diff(test_name, outfile, reffile, ignore_ws=False):
    """ compare 2 files for a diff.

        Args:
            test_name (str): Unique name to prefix the diff file.
            outfile (str): Path to the output file.
            reffile (str): Path to the reference file.
            ignore_ws (bool): ignore whitespace during the compare [False].

        Returns:
            (bool) True if the 2 files match.
    """
    check_param_type("test_name", test_name, str)
    check_param_type("outfile", outfile, str)
    check_param_type("reffile", reffile, str)
    check_param_type("ignore_ws", ignore_ws, bool)

    # Use diff (ignore whitespace) to see if the sorted files are the same
    if not os.path.isfile(outfile):
        log_error("Cannot diff files: Out File {0} does not exist".format(outfile))
        return False

    if not os.path.isfile(reffile):
        log_error("Cannot diff files: Ref File {0} does not exist".format(reffile))
        return False

    diff_file = "{1}/{0}_diff_file".format(test_name, test_output_get_tmp_dir())

    ws_flag = ""
    if ignore_ws:
        ws_flag = "-b "

    cmd = "diff {0} {1} {2} > {3}".format(ws_flag, outfile, reffile, diff_file)
    filesAreTheSame = (os.system(cmd) == 0)

    return filesAreTheSame

###

def testing_compare_sorted_diff(test_name, outfile, reffile):
    """ Sort and then compare 2 files for a difference.

        Args:
            test_name (str): Unique name to prefix the diff file and 2 sorted files.
            outfile (str): Path to the output file
            reffile (str): Path to the reference file

        Returns:
            (bool) True if the 2 sorted files match
    """
    check_param_type("test_name", test_name, str)
    check_param_type("outfile", outfile, str)
    check_param_type("reffile", reffile, str)

    if not os.path.isfile(outfile):
        log_error("Cannot diff files: Out File {0} does not exist".format(outfile))
        return False

    if not os.path.isfile(reffile):
        log_error("Cannot diff files: Ref File {0} does not exist".format(reffile))
        return False

    sorted_outfile = "{1}/{0}_sorted_outfile".format(test_name, test_output_get_tmp_dir())
    sorted_reffile = "{1}/{0}_sorted_reffile".format(test_name, test_output_get_tmp_dir())
    diff_file = "{1}/{0}_diff_file".format(test_name, test_output_get_tmp_dir())

    os.system("sort -o {0} {1}".format(sorted_outfile, outfile))
    os.system("sort -o {0} {1}".format(sorted_reffile, reffile))

    # Use diff (ignore whitespace) to see if the sorted files are the same
    cmd = "diff -b {0} {1} > {2}".format(sorted_outfile, sorted_reffile, diff_file)
    filesAreTheSame = (os.system(cmd) == 0)

    return filesAreTheSame

###

def testing_get_diff_data(test_name):
    """ Return the diff data file from a regular diff. This should be used after
        a call to testing_compare_sorted_diff() or testing_compare_diff() to
        return the sorted diff data

        Args:
            test_name (str): Unique name used to generate the diff file.

        Returns:
            (str) The diff data file if it exists; otherwise an empty string
    """
    check_param_type("test_name", test_name, str)

    diff_file = "{1}/{0}_diff_file".format(test_name, test_output_get_tmp_dir())

    if not os.path.isfile(diff_file):
        log_debug("diff_file {0} is not found".format(diff_file))
        return ""

    with open(diff_file, 'r') as file:
        data = file.read()

    return data

###

def testing_merge_mpi_files(filepath_wildcard, mpiout_filename, outputfilepath):
    """ Merge a group of common MPI files into an output file

        Args:
            filepath_wildcard (str): The wildcard Path to the files to be mreged
            mpiout_filename (str): The name of the MPI output filename
            outputfilepath (str): The output file path
    """
    check_param_type("filepath_wildcard", filepath_wildcard, str)
    check_param_type("mpiout_filename", mpiout_filename, str)
    check_param_type("outputfilepath", outputfilepath, str)

    # Delete any output files that might exist
    cmd = "rm -rf {0}".format(outputfilepath)
    os.system(cmd)

    # Check for existing mpiout_filepath (for OpenMPI V4)
    mpipath = "{0}/1".format(mpiout_filename)
    if os.path.isdir(mpipath):
        dirItemList = os.listdir(mpipath)
        for rankdir in dirItemList:
            mpirankoutdir = "{0}/{1}".format(mpipath, rankdir)
            mpioutfile = "{0}/{1}".format(mpirankoutdir, "stdout")
            if os.path.isdir(mpirankoutdir) and os.path.isfile(mpioutfile):
                cmd = "cat {0} >> {1}".format(mpioutfile, outputfilepath)
                os.system(cmd)
    else:
        # Cat the files together normally (OpenMPI V2)
        cmd = "cat {0} > {1}".format(filepath_wildcard, outputfilepath)
        os.system(cmd)

###

def testing_remove_component_warning_from_file(input_filepath):
    """ Remove SST Component warnings from a file

        This will re-write back to the file with the removed warnings

        Args:
            input_filepath (str): Path to the file to have warnings removed from
    """
    check_param_type("input_filepath", input_filepath, str)

    bad_string = 'WARNING: No components are'
    _remove_lines_with_string_from_file(bad_string, input_filepath)

################################################################################
### OS Basic Or Equivalent Commands
################################################################################

def os_simple_command(os_cmd, run_dir=None):
    """ Perform an simple os command and return a tuple of the (rtncode, rtnoutput).

        NOTE: Simple command cannot have pipes or redirects

        Args:
            os_cmd (str): Command to run
            run_dir (str): Directory where to run the command; if None (defaut)
                           current working directory is used.

        Returns:
            (tuple) Returns a tuple of the (rtncode, rtnoutput) of types (int, str)
    """
    check_param_type("os_cmd", os_cmd, str)
    if run_dir is not None:
        check_param_type("run_dir", run_dir, str)
    rtn = OSCommand(os_cmd, set_cwd=run_dir).run()
    rtn_data = (rtn.result(), rtn.output())
    return rtn_data

def os_ls(directory=".", echo_out=True):
    """ Perform an simple ls -lia shell command and dump output to screen.

        Args:
            directory (str): Directory to run in [.]
            echo_out (bool): echo output to console [True]

        Returns:
            (str) Output from ls command
    """
    check_param_type("directory", directory, str)
    cmd = "ls -lia {0}".format(directory)
    rtn = OSCommand(cmd).run()
    if echo_out:
        log("{0}".format(rtn.output()))
    return rtn.output()

def os_pwd(echo_out=True):
    """ Perform an simple pwd shell command and dump output to screen.

        Args:
            echo_out (bool): echo output to console [True]

        Returns:
            (str) Output from pwd command
    """
    cmd = "pwd"
    rtn = OSCommand(cmd).run()
    if echo_out:
        log("{0}".format(rtn.output()))
    return rtn.output()

def os_cat(filepath, echo_out=True):
    """ Perform an simple cat shell command and dump output to screen.

        Args:
            filepath (str): Path to file to cat

        Returns:
            (str) Output from cat command
    """
    check_param_type("filepath", filepath, str)
    cmd = "cat {0}".format(filepath)
    rtn = OSCommand(cmd).run()
    if echo_out:
        log("{0}".format(rtn.output()))
    return rtn.output()

def os_symlink_file(srcdir, destdir, filename):
    """ Create a symlink of a file

        Args:
            srcdir (str): Path to source dir of the file
            destdir (str): Path to destination dir of the file
            filename (str): Name of the file
    """
    check_param_type("srcdir", srcdir, str)
    check_param_type("destdir", destdir, str)
    check_param_type("filename", filename, str)
    srcfilepath = "{0}/{1}".format(srcdir, filename)
    dstfilepath = "{0}/{1}".format(destdir, filename)
    os.symlink(srcfilepath, dstfilepath)

def os_symlink_dir(srcdir, destdir):
    """ Create a symlink of a directory

        Args:
            srcdir (str): Path to source dir
            destdir (str): Path to destination dir
    """
    check_param_type("srcdir", srcdir, str)
    check_param_type("destdir", destdir, str)
    os.symlink(srcdir, destdir)

def os_awk_print(in_str, fields_index_list):
    """ Perform an awk / print (equivalent) command which returns specific
        fields of an input string as a string.

        Args:
            in_str (str): Input string to parse
            fields_index_list (list of ints): A list of ints and in the order of
                                             fields to be returned

        Returns:
            (str) Space separated string of extracted fields.
    """
    if PY2:
        check_param_type("in_str", in_str, unicode)
    else:
        if isinstance(in_str, bytes):
            check_param_type("in_str", in_str, bytes)
        else:
            check_param_type("in_str", in_str, str)
    check_param_type("fields_index_list", fields_index_list, list)
    for index, field_index in enumerate(fields_index_list):
        check_param_type("field_index - {0}".format(index), field_index, int)
    finalstrdata = ""
    split_list = in_str.split()
    for field_index in fields_index_list:
        finalstrdata +=  "{0} ".format(split_list[field_index])
    return finalstrdata

def os_wc(in_file, fields_index_list=[]):
    """ Run a wc (equivalent) command on a file and then extract specific
        fields of the result as a string.

        Args:
            in_file (str): Input string to parse
            fields_index_list (list of ints): A list of ints and in the order of
                                             fields to be returned

        Returns:
            (str) Space separated string of extracted fields.
    """
    check_param_type("in_file", in_file, str)
    check_param_type("fields_index_list", fields_index_list, list)
    for index, field_index in enumerate(fields_index_list):
        check_param_type("field_index - {0}".format(index), field_index, int)
    cmd = "wc {0}".format(in_file)
    rtn = OSCommand(cmd).run()
    wc_out = rtn.output()
    if fields_index_list:
        wc_out = os_awk_print(wc_out, fields_index_list)
    return wc_out

def os_test_file(file_path, expression="-e"):
    """ Run a shell 'test' command on a file.

        Args:
            file_path (str): Path to the file to be tested
            expression (str): Test expression [-e = exists]

        Returns:
            (bool) True if test is successful.
    """
    check_param_type("file_path", file_path, str)
    check_param_type("expression", expression, str)
    if os.path.exists(file_path):
        cmd = "test {0} {1}".format(expression, file_path)
        rtn = OSCommand(cmd).run()
        log_debug("Test cmd = {0}; rtn = {1}".format(cmd, rtn.result()))
        return rtn.result() == 0
    else:
        log_error("File {0} does not exist".format(file_path))
        return False

def os_wget(fileurl, targetdir, num_tries=3, secsbetweentries=10, wgetparams=""):
    """ Perform a wget command to download a file from a url.

        Args:
            fileurl (str): URL to the file to be downloaded.
            targetdir (str): Where to download the file to
            num_tries (int): Max number of tries to download [3].
            secsbetweentries (int): Seconds between tries [10]
            wgetparams (str): Max number of tries to download.

        Returns:
            (bool) True if wget is successful.
    """
    check_param_type("fileurl", fileurl, str)
    check_param_type("targetdir", targetdir, str)
    check_param_type("num_tries", num_tries, int)
    check_param_type("secsbetweentries", secsbetweentries, int)
    check_param_type("wgetparams", wgetparams, str)

    wget_success = False

    # Make sure target dir exists, and cd into it
    if not os.path.isdir(targetdir):
        log_error("Download directory {0} does not exist".format(targetdir))
        return False

    wgetoutfile = "{0}/wget.out".format(test_output_get_tmp_dir())

    log_debug("Downloading file via wget: {0}".format(fileurl))
    cmd = "wget {0} --no-check-certificate -P {1} {2} > {3} 2>&1".\
       format(fileurl, targetdir, wgetparams, wgetoutfile)
    attemptnum = 1
    while attemptnum <= num_tries:
        log_debug("wget Attempt#{0}; cmd={1}".format(attemptnum, cmd))
        rtn_value = os.system(cmd)
        log_debug("wget rtn: {0}".format(rtn_value))
        if rtn_value == 0:
            # wget was successful, force exit of the loop
            log_debug("wget download successful")
            wget_success = True
            attemptnum = num_tries + 1
            continue

        if os.path.isfile(wgetoutfile):
            with open(wgetoutfile, "rb") as wgetfile:
                wgetoutput = "".join(wgetfile.readlines()[1:])

            log_debug("wget output:\n{0}".format(wgetoutput))
        else:
            log_debug("wget output: NOT FOUND")

        log_debug("Download Failed, waiting {0} seconds and trying again...".\
            format(secsbetweentries))

        attemptnum = attemptnum + 1
        time.sleep(secsbetweentries)

    if not wget_success:
        log_error("Failed to download via wget {0}".format(fileurl))

    return wget_success

def os_extract_tar(tarfilepath, targetdir="."):
    """ Extract directories/files from a tar file.

        Args:
            tarfilepath (str): The filepath to the tar file to be extracted.
            targetdir (str): Where to extract to [.]

        Returns:
            (bool) True if wget is successful.
    """
    if not os.path.isfile(tarfilepath):
        errmsg = "tar file{0} does not exist".format(tarfilepath)
        log_error(errmsg)
        return False
    try:
        this_tar = tarfile.open(tarfilepath)
        this_tar.extractall(targetdir)
        this_tar.close()
        return True
    except tarfile.TarError as exc_e:
        errmsg = "Failed to untar file {0} - {1}".format(tarfilepath, exc_e)
        log_error(errmsg)
        return False

################################################################################
### Platform Specific Support Functions
################################################################################

def _get_linux_distribution():
    """ Return the linux distribution info as a tuple"""
    # The method linux_distribution is depricated in deprecated in Py3.5
    _linux_distribution = getattr(platform, 'linux_distribution', None)
        # This is the easy method for Py2 - p3.7.
    if _linux_distribution is not None:
        return _linux_distribution()

    # We need to do this the hard way, NOTE: order of checking is important
    distname = "undefined"
    distver = "undefined"
    if os.path.isfile("/etc/toss-release"):
        distname = "toss"
        distver = _get_linux_version("/etc/toss-release", "-")
    elif os.path.isfile("/etc/centos-release"):
        distname = "centos"
        distver = _get_linux_version("/etc/centos-release", " ")
    elif os.path.isfile("/etc/redhat-release"):
        distname = "red hat"
        distver = _get_linux_version("/etc/redhat-release", " ")
    elif os.path.isfile("/etc/lsb-release"):
        # Until we have other OS's, this is Ubuntu.
        distname = "ubuntu"
        distver = _get_linux_version("/etc/lsb-release", " ")
    rtn_data = (distname, distver)
    return rtn_data

###

def _get_linux_version(filepath, sep):
    """ return the linux OS version as a string"""
    # Find the first digit + period in the tokenized string list
    with open(filepath, 'r') as filehandle:
        for line in filehandle:
            #print("found line = " + line)
            word_list = line.split(sep)
            for word in word_list:
                #print("word =" + word)
                m_data = re.search(r"[\d.]+", word)
                #print("found_ver = {0}".format(m_data))
                if m_data is not None:
                    found_ver = m_data.string[m_data.start():m_data.end()]
                    #print("found_ver = {0}".format(found_ver))
                    return found_ver
    return "undefined"


################################################################################
### Generic Internal Support Functions
################################################################################

def _get_sst_config_include_file_value(include_dict, include_source, define, default=None,
                                       data_type=str, disable_warning = False):
    """ Retrieve a define from an SST Configuration Include File (sst_config.h or sst-element_config.h)
       include_dict (dict): The dictionary to search for the define
       include_source (str): The name of the include file we are searching
       define (str): The define to look for
       default (str|int): Default Return if failure occurs
       data_type (str|int): The data type to return
       disable_warning (bool): Disable the warning if define not found
       :return (str|int): The returned data or default if not found in the dict
       This will raise a SSTTestCaseException if a default is not provided or type
       is incorrect
    """
    if data_type not in (int, str):
        raise SSTTestCaseException("Illegal datatype {0}".format(data_type))
    check_param_type("include_source", include_source, str)
    check_param_type("define", define, str)
    if default is not None:
        check_param_type("default", default, data_type)
    try:
        rtn_data = include_dict[define]
    except KeyError as exc_e:
        errmsg = ("Reading Config include file {0}") \
                 + (" - Cannot find #define {1}".format(include_source, exc_e))
        if disable_warning == False:
            log_warning(errmsg)
        if default is None:
            raise SSTTestCaseException(exc_e)
        rtn_data = default
    if data_type == int:
        rtn_data = int(rtn_data)
    return rtn_data

###

def _get_sstsimulator_conf_value(section, key, default=None, data_type=str):
    """ Retrieve a Section/Key from the SST Configuration File (sstsimulator.conf)
       section (str): The [section] to look for the key
       key (str): The key to find
       default (str|int|float|bool): Default Return if failure occurs
       :return (str|int|float|bool): The returned data or default if not found in file
       This will raise a SSTTestCaseException if a default is not provided
    """
    if data_type not in (int, str, float, bool):
        raise SSTTestCaseException("Illegal datatype {0}".format(data_type))
    check_param_type("section", section, str)
    check_param_type("key", key, str)
    if default is not None:
        check_param_type("default", default, data_type)
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        if data_type == str:
            return core_conf_file_parser.get(section, key)
        if data_type == int:
            return core_conf_file_parser.getint(section, key)
        if data_type == float:
            return core_conf_file_parser.getfloat(section, key)
        if data_type == bool:
            return core_conf_file_parser.getbool(section, key)
    except configparser.Error as exc_e:
        rtn_default = _handle_config_err(exc_e, default)
        if default is None:
            raise SSTTestCaseException(exc_e)
    return rtn_default

###

def _handle_config_err(exc_e, default_rtn_data):
    errmsg = "Reading SST-Core Config file sstsimulator.conf - {0}".format(exc_e)
    log_warning(errmsg)
    return default_rtn_data

###

def _remove_lines_with_string_from_file(removestring, input_filepath):
    bad_strings = [removestring]

    # Create a temp file
    filename = os.path.basename(input_filepath)
    tmp_output_filepath = "{0}/{1}.removed_lines".format(test_output_get_tmp_dir(), filename)
    bak_output_filepath = "{0}/{1}.bak".format(test_output_get_tmp_dir(), filename)

    if not os.path.exists(input_filepath):
        log_error("_remove_lines_with_string_from_file() - File {0} does not exist".\
            format(input_filepath))
        return

    # Copy the lines in the input file line by line, to a backup file and to the
    # tmp file; but skip any lines in the removestring from the tmp file
    with open(input_filepath) as oldfile, open(tmp_output_filepath, 'w') as newfile, open(bak_output_filepath, 'w') as bakfile:
        for line in oldfile:
            bakfile.write(line)
            if not any(bad_string in line for bad_string in bad_strings):
                newfile.write(line)

    # Now copy the tmp file back over the input file
    shutil.copy(tmp_output_filepath, input_filepath)
