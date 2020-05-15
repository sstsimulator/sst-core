# -*- coding: utf-8 -*-

## Copyright 2009-2020 NTESS. Under the terms
## of Contract DE-NA0003525 with NTESS, the U.S.
## Government retains certain rights in this software.
##
## Copyright (c) 2009-2020, NTESS
## All rights reserved.
##
## This file is part of the SST software package. For license
## information, see the LICENSE file in the top level directory of the
## distribution.

""" This module provides the a large number of support functions
    that tests can call
"""

import sys
import os
import unittest
import platform
import re
import time
import multiprocessing
import tarfile

# ConfigParser module changes name between Py2->Py3
try:
    import configparser
except ImportError:
    import ConfigParser as configparser

import test_engine_globals
from test_engine_support import OSCommand
from test_engine_support import check_param_type

################################################################################

PY2 = sys.version_info[0] == 2
PY3 = sys.version_info[0] == 3

OS_DIST_OSX    = "OSX"
OS_DIST_CENTOS = "CENTOS"
OS_DIST_RHEL   = "RHEL"
OS_DIST_TOSS   = "TOSS"
OS_DIST_UBUNTU = "UBUNTU"
OS_DIST_UNDEF  = "UNDEFINED"

################################################################################

class SSTTestCaseException(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

################################################################################

################################################################################
# Commandline Information Functions
################################################################################

def is_testing_in_debug_mode():
    """ Identify if test engine is in debug mode
       :return: True if in debug mode
    """
    return test_engine_globals.TESTENGINE_DEBUGMODE

###

def is_testing_in_concurrent_mode():
    """ Identify if test engine is in debug mode
       :return: True if in debug mode
    """
    return test_engine_globals.TESTENGINE_CONCURRENTMODE

###

def get_testing_num_ranks():
    """ Get the number of ranks defined to be run during the testing
       :return: The number of requested run ranks
    """
    return test_engine_globals.TESTENGINE_SSTRUN_NUMRANKS

###

def get_testing_num_threads():
    """ Get the number of threads defined to be run during the testing
       :return: The number of requested run threads
    """
    return test_engine_globals.TESTENGINE_SSTRUN_NUMTHREADS

################################################################################
# System Information Functions
################################################################################

def is_py_2():
    """ Return True if running in Python Version 2"""
    return PY2

def is_py_3():
    """ Return True if running in Python Version 3"""
    return PY3

###

def get_system_node_name():
    """ Returns the node name of the system"""
    return platform.node()

###

def get_host_os_kernel_type():
    """ Returns the kernal type Linux | Darwin"""
    return platform.system()

def get_host_os_kernel_release():
    """ Returns the release number, this is not the same as OS version"""
    return platform.release()

def get_host_os_kernel_arch():
    """ Returns the system arch x86_64 on Linux, i386 on OSX"""
    return platform.machine()

###

def get_host_os_distribution_type():
    """ Returns the os distribution type as a uppercase string"""
    k_type = get_host_os_kernel_type()
    if k_type == 'Linux':
        lin_dist = _get_linux_distribution()
        dist_name = lin_dist[0].lower()
        if "centos" in dist_name: return OS_DIST_CENTOS
        if "red hat" in dist_name: return OS_DIST_RHEL
        if "toss" in dist_name: return OS_DIST_TOSS
        if "ubuntu" in dist_name: return OS_DIST_UBUNTU
    elif k_type == 'Darwin': return OS_DIST_OSX
    else: return "undefined"

def get_host_os_distribution_version():
    """ Returns the os distribution version as a string"""
    k_type = get_host_os_kernel_type()
    if k_type == 'Linux':
        lin_dist = _get_linux_distribution()
        return lin_dist[1]
    elif k_type == 'Darwin':
        mac_ver = platform.mac_ver()
        return mac_ver[0]
    else:
        return "undefined"

###

def is_host_os_osx():
    """ Returns true if the os distribution is OSX"""
    return get_host_os_distribution_type() == OS_DIST_OSX

def is_host_os_linux():
    """ Returns true if the os distribution is Linux"""
    return not get_host_os_distribution_type() == OS_DIST_OSX

def is_host_os_centos():
    """ Returns true if the os distribution is CentOS"""
    return get_host_os_distribution_type() == OS_DIST_CENTOS

def is_host_os_rhel():
    """ Returns true if the os distribution is Red Hat"""
    return get_host_os_distribution_type() == OS_DIST_RHEL

def is_host_os_toss():
    """ Returns true if the os distribution is Toss"""
    return get_host_os_distribution_type() == OS_DIST_TOSS

def is_host_os_ubuntu():
    """ Returns true if the os distribution is Ubuntu"""
    return get_host_os_distribution_type() == OS_DIST_UBUNTU

###

def get_num_cores_on_system():
    """ Figure out how many cores exist on the system"""
    num_cores = multiprocessing.cpu_count()
    return num_cores

################################################################################
# SST Skipping Support
################################################################################

def is_scenario_filtering_enabled(scenario_name):
    """ Detirmine if a scenario filter name is enabled
       :param: scenario_name (str): The scenario filter name to check
       :return: True if the scenario filter name is enabled
    """
    check_param_type("scenario_name", scenario_name, str)
    return scenario_name in test_engine_globals.TESTENGINE_SCENARIOSLIST

###

def skipOnScenario(scenario_name, reason):
    """ Skip a test if a scenario filter name is enabled
       :param: scenario_name (str): The scenario filter name to check
       :param: reason (str): The reason for the skip
    """
    check_param_type("scenario_name", scenario_name, str)
    check_param_type("reason", reason, str)
    if not is_scenario_filtering_enabled(scenario_name):
        return lambda func: func
    return unittest.skip(reason)

###

def skipOnSSTSimulatorConfEmptyStr(section, key, reason):
    """ Skip a test if a section/key in the sstsimulator.conf file is missing an
        entry
       :param: section (str): The section in the sstsimulator.conf to check
       :param: key (str): The key in the sstsimulator.conf to check
       :param: reason (str): The reason for the skip
    """
    check_param_type("section", section, str)
    check_param_type("key", key, str)
    check_param_type("reason", reason, str)
    rtn_str = get_sstsimulator_conf_value_str(section, key, "")
    if rtn_str != "":
        return lambda func: func
    return unittest.skip(reason)

################################################################################
# SST Configuration include file (sst_config.h.conf) Access Functions
################################################################################

def get_sst_config_include_file_value_int(define, default=None):
    """ Retrieve a define from the SST Configuration Include File (sst_config.h)
       :param: section (str): The [section] to look for the key
       :param: default (int): Default Return if failure occurs
       :return (int): The returned data or default if not found in the dict
       This will raise a SSTTestCaseException if a default is not provided or type
       is incorrect
    """
    return _get_sst_config_include_file_value(define, default, int)

###

def get_sst_config_include_file_value_str(define, default=None):
    """ Retrieve a define from the SST Configuration Include File (sst_config.h)
       :param: section (str): The [section] to look for the key
       :param: default (str): Default Return if failure occurs
       :return (str): The returned data or default if not found in the dict
       This will raise a SSTTestCaseException if a default is not provided or type
       is incorrect
    """
    return _get_sst_config_include_file_value(define, default, str)

################################################################################
# SST Configuration file (sstsimulator.conf) Access Functions
################################################################################

def get_sstsimulator_conf_value_str(section, key, default=None):
    """ Retrieve a Section/Key from the SST Configuration File (sstsimulator.conf)
       :param: section (str): The [section] to look for the key
       :param: key (str): The key to find
       :param: default (str): Default Return if failure occurs
       :return (str): The returned data or default if not found in file
       This will raise a SSTTestCaseException if a default is not provided
    """
    return _get_sstsimulator_conf_value(section, key, default, str)

###

def get_sstsimulator_conf_value_int(section, key, default=None):
    """ Retrieve a Section/Key from the SST Configuration File (sstsimulator.conf)
       :param: section (str): The [section] to look for the key
       :param: key (str): The key to find
       :param: default (int): Default Return if failure occurs
       :return (float): The returned data or default if not found in file
       This will raise a SSTTestCaseException if a default is not provided
    """
    return _get_sstsimulator_conf_value(section, key, default, int)

###

def get_sstsimulator_conf_value_float(section, key, default=None):
    """ Retrieve a Section/Key from the SST Configuration File (sstsimulator.conf)
       :param: section (str): The [section] to look for the key
       :param: key (str): The key to find
       :param: default (float): Default Return if failure occurs
       :return (float): The returned data or default if not found in file
       This will raise a SSTTestCaseException if a default is not provided
    """
    return _get_sstsimulator_conf_value(section, key, default, float)

###

def get_sstsimulator_conf_value_bool(section, key,default=None):
    """ Retrieve a Section/Key from the SST Configuration File (sstsimulator.conf)
       :param: section (str): The [section] to look for the key
       :param: key (str): The key to find
       :param: default (bool): Default Return if failure occurs
       :return (bool): The returned data or default if not found in file
       NOTE: "1", "yes", "true", and "on" return True
       This will raise a SSTTestCaseException if a default is not provided
    """
    return _get_sstsimulator_conf_value(section, key, default, bool)

###

def get_sstsimulator_conf_sections():
    """ Retrieve a list of sections that exist in the SST Configuration File (sstsimulator.conf)
       :return (list of str): The list of sections in the file
       This will raise a SSTTestCaseException if an error occurs
    """
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        return core_conf_file_parser.sections()
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def get_sstsimulator_conf_section_keys(section):
    """ Retrieve a list of keys under a section that exist in the
        SST Configuration File  (sstsimulator.conf)
       :param: section (str): The [section] to look for the key
       :return (list of str): The list of keys under a section in the file
       This will raise a SSTTestCaseException if an error occurs
    """
    check_param_type("section", section, str)
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        return core_conf_file_parser.options(section)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def get_all_sst_config_keys_values_from_section(section):
    """ Retrieve a list of tuples that contain all the key, value pairs
        under a section that exists in the SST Configuration File (sstsimulator.conf)
       :param: section (str): The [section] to look for the key
       :return (list of tuples): The list of tuples of key, value pairs
       This will raise a SSTTestCaseException if an error occurs
    """
    check_param_type("section", section, str)
    core_conf_file_parser = TESTENGINE_CORE_CONFFILE_PARSER
    try:
        return core_conf_file_parser.items(section)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def does_sst_config_have_section(section):
    """ Check if the SST Configuration File  (sstsimulator.conf) have a section
       :param: section (str): The [section] to look for the key
       :return (bool):
       This will raise a SSTTestCaseException if an error occurs
    """
    check_param_type("section", section, str)
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        return core_conf_file_parser.has_section(section)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def does_sst_config_section_have_key(section, key):
    """ Check if the SST Configuration File  (sstsimulator.conf) has a key under a section
       :param: section (str): The [section] to look for the key
       :param: key (str): The key to find
       :return (bool):
       This will raise a SSTTestCaseException if an error occurs
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
    """ Log a message, this will not output unless we are
        outputing in >= normal mode.
       :param: logstr = string to be logged
    """
    check_param_type("logstr", logstr, str)
    if test_engine_globals.TESTENGINE_VERBOSITY >= test_engine_globals.VERBOSE_NORMAL:
        log_forced(logstr)

###

def log_forced(logstr):
    """ Log a message, no matter what the verbosity is
        if in the middle of testing, it will precede with a \n to slip
        in-between unittest outputs
        :param: logstr = string to be logged
    """
    check_param_type("logstr", logstr, str)
    extra_lf = ""
    if test_engine_globals.TESTRUN_TESTRUNNINGFLAG:
        extra_lf = "\n"
    print(("{0}{1}".format(extra_lf, logstr)))

###

def log_debug(logstr):
    """ Log a DEBUG: message, only if in debug verbosity mode
        :param: logstr = string to be logged
    """
    if test_engine_globals.TESTENGINE_DEBUGMODE:
        finalstr = "DEBUG: {0}".format(logstr)
        log_forced(finalstr)

###

def log_info(logstr, forced=True):
    """ Log a INFO: message
        :param: logstr = string to be logged
        :param: forced = True to always force the logging regardless of verbosity
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
    """ Log a ERROR: message, no matter what the verbosity is
        :param: logstr = string to be logged
    """
    check_param_type("logstr", logstr, str)
    finalstr = "ERROR: {0}".format(logstr)
    log_forced(finalstr)
    test_engine_globals.TESTENGINE_ERRORCOUNT += 1

###

def log_warning(logstr):
    """ Log a WARNING: message, no matter what the verbosity is
        :param: logstr = string to be logged
    """
    check_param_type("logstr", logstr, str)
    finalstr = "WARNING: {0}".format(logstr)
    log_forced(finalstr)

###

def log_fatal(errstr):
    """ Log a FATAL: message, no matter what the verbosity is
        THIS WILL KILL THE TEST ENGINE AND RETURN FAILURE
        :param: logstr = string to be logged
    """
    check_param_type("errstr", errstr, str)
    finalstr = "FATAL: {0}".format(errstr)
    log_forced(finalstr)
    sys.exit(1)

################################################################################
### Testing Directories
################################################################################

def get_test_output_run_dir():
    """ Return the path of the output run directory
       :return: str the dir
    """
    return test_engine_globals.TESTOUTPUT_RUNDIRPATH

###

def get_test_output_tmp_dir():
    """ Return the path of the output run directory
       :return: str the dir
    """
    return test_engine_globals.TESTOUTPUT_TMPDIRPATH

################################################################################
### Testing Support
################################################################################

def compare_diff(outfile, reffile, ignore_ws=False):
   """ compare 2 files for a diff
       :param: outfile (str) Path to the output file
       :param: reffile (str) Path to the reference file
       :param: ignore_ws (bool) Path to the reference file
       :return: True if the 2 files match
   """
   # Use diff (ignore whitespace) to see if the sorted files are the same
   if not os.path.isfile(outfile):
       log_error("Cannot diff files: Out File {0} does not exist".format(outfile))
       return False

   if not os.path.isfile(reffile):
       log_error("Cannot diff files: Ref File {0} does not exist".format(reffile))
       return False

   ws_flag = ""
   if ignore_ws == True:
       ws_flag = "-b "

   cmd = "diff {0}{0} {1} > /dev/null 2>&1".format(ignore_ws, outfile, reffile)
   filesAreTheSame = (os.system(cmd) == 0)

   return filesAreTheSame

###

def compare_sorted_diff(test_name, outfile, reffile):
   """ Sort a output file along with a reference file and compare them
       :param: test_name (str) Name to prefix the sorted files
       :param: outfile (str) Path to the output file
       :param: reffile (str) Path to the reference file
       :return: True if the 2 sorted file match
   """
   if not os.path.isfile(outfile):
       log_error("Cannot diff files: Out File {0} does not exist".format(outfile))
       return False

   if not os.path.isfile(reffile):
       log_error("Cannot diff files: Ref File {0} does not exist".format(reffile))
       return False

   sorted_outfile = "{1}/{0}_sorted_outfile".format(test_name, get_test_output_tmp_dir())
   sorted_reffile = "{1}/{0}_sorted_reffile".format(test_name, get_test_output_tmp_dir())
   diff_sorted_file = "{1}/{0}_diff_sorted".format(test_name, get_test_output_tmp_dir())

   os.system("sort -o {0} {1}".format(sorted_outfile, outfile))
   os.system("sort -o {0} {1}".format(sorted_reffile, reffile))

   # Use diff (ignore whitespace) to see if the sorted files are the same
   cmd = "diff -b {0} {1} > {2}".format(sorted_outfile, sorted_reffile, diff_sorted_file)
   filesAreTheSame = (os.system(cmd) == 0)

   return filesAreTheSame

###

def merge_mpi_files(filepath_wildcard, mpiout_filename, outputfilepath):
    """ Merge a group of common files into an output file
       :param: filepath_wildcard (str) The wildcard Path to the files to be mreged
       :param: outputfilepath (str) The output file path
    """
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

################################################################################
### OS Basic Commands
################################################################################

def os_simple_command(os_cmd, rtn_value=None):
    """ Perform an simple os command and get the stdout and the return code if needed
        NOTE: Simple command cannot have pipes or redirects
    """
    rtn = OSCommand(os_cmd).run()
#    if rtn_value != None:
#        rtn_value = rtn.result()
    return rtn.output()

def os_ls(directory="."):
    """ Perform an ls -lia on a directory and dump output to screen """
    cmd = "ls -lia {0}".format(directory)
    rtn = OSCommand(cmd).run()
    log("{0}".format(rtn.output()))

def os_cat(filepath):
    """ Perform an cat cmd on a file and dump output to screen """
    cmd = "cat {0}".format(filepath)
    rtn = OSCommand(cmd).run()
    log("{0}".format(rtn.output()))

def os_file_symlink(srcdir, destdir, filename):
    """ Create a simlink of a file """
    srcfilepath = "{0}/{1}".format(srcdir, filename)
    dstfilepath = "{0}/{1}".format(destdir, filename)
    os.symlink(srcfilepath, dstfilepath)

def os_dir_symlink(srcdir, destdir):
    """ Create a simlink of a directory """
    os.symlink(srcdir, destdir)

def os_wget(fileurl, targetdir, num_tries=3, secsbetweentries=10, wgetparams=""):
    """ wget Download a file with retries
        :return: True on success
    """
    rtn = False

    # Make sure target dir exists, and cd into it
    if not os.path.isdir(targetdir):
        log_error("Download directory {0} does not exist".format(targetdir))
        return false

    savedir = os.getcwd()
    os.chdir(targetdir)

    wgetoutfile = "{0}/wget.out".format(get_test_output_tmp_dir())

    log_debug("Downloading file via wget: {0}".format(fileurl))
    cmd = "wget {0} --no-check-certificate {1} > {2} 2>&1".format(fileurl, wgetparams, wgetoutfile)
    attemptnum = 1
    while attemptnum <= num_tries:
        log_debug("Attempt#{0}; cmd={1}".format(attemptnum, cmd))
        rtn_value = os.system(cmd)
        if rtn_value == 0:
            rtn = True
            attemptnum = num_tries + 1
            continue
        log_debug("wget rtn: {0}".format(rtn_value))
        if os.path.isfile(wgetoutfile):
            with open(wgetoutfile, "rb") as wgetfile:
                 wgetoutput = "".join(wgetfile.readlines()[1:])

            log_debug("wget output:\n{0}".format(wgetoutput))
        else:
            log_debug("wget output: NOT FOUND")
        log_debug("Download Failed, waiting {0} seconds and trying again...".format(secsbetweentries))
        attemptnum = attemptnum + 1
        time.sleep(secsbetweentries)

    if rtn == False:
        log_error("Failed to download via wget {0}".format(fileurl))

    # Restore the saved dir and return the results
    os.chdir(savedir)
    return rtn

def os_extract_tar(tarfilepath, targetdir = "."):
    """ Untar a file
        :return: True on success
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
    else:
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
        rtn_data=(distname, distver)
        return rtn_data

###

def _get_linux_version(filepath, sep):
    """ return the linux OS version as a string"""
    # Find the first digit + period in the tokenized string list
    with open(filepath, 'r') as f:
        for line in f:
            #print("found line = " + line)
            word_list = line.split(sep)
            for word in word_list:
                #print("word =" + word)
                m = re.search(r"[\d.]+", word)
                #print("found_ver = {0}".format(m))
                if m is not None:
                    found_ver = m.string[m.start():m.end()]
                    #print("found_ver = {0}".format(found_ver))
                    return found_ver
    return "undefined"


################################################################################
### Generic Internal Support Functions
################################################################################

def _get_sst_config_include_file_value(define, default=None, data_type=str):
    """ Retrieve a define from the SST Configuration Include File (sst_config.h)
       :param: section (str): The [section] to look for the key
       :param: default (str|int): Default Return if failure occurs
       :param: data_type (str|int): The data type to return
       :return (str|int): The returned data or default if not found in the dict
       This will raise a SSTTestCaseException if a default is not provided or type
       is incorrect
    """
    if not data_type == int and not data_type == str:
        raise SSTTestCaseException("Illegal datatype {0}".format(data_type))
    check_param_type("define", define, str)
    if default != None:
        check_param_type("default", default, data_type)
    core_conf_inc_dict = test_engine_globals.TESTENGINE_CORE_CONFINCLUDE_DICT
    try:
        rtn_data =  core_conf_inc_dict[define]
    except KeyError as exc_e:
        errmsg = "Reading SST-Core Config include file sst_config.h - Cannot find #define {0}".format(exc_e)
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
       :param: section (str): The [section] to look for the key
       :param: key (str): The key to find
       :param: default (str|int|float|bool): Default Return if failure occurs
       :return (str|int|float|bool): The returned data or default if not found in file
       This will raise a SSTTestCaseException if a default is not provided
    """
    if not data_type == int and not data_type == str and \
       not data_type == float and not data_type == bool:
        raise SSTTestCaseException("Illegal datatype {0}".format(data_type))
    check_param_type("section", section, str)
    check_param_type("key", key, str)
    if default != None:
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

