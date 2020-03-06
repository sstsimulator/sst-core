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
import platform
import re

# ConfigParser module changes name between Py2->Py3
try:
    import configparser
except ImportError:
    import ConfigParser as configparser

import test_engine_globals
from test_engine_support import OSCommand
from test_engine_support import check_param_type
from test_engine_support import strclass

from test_engine_junit import JUnitTestCase
from test_engine_junit import JUnitTestSuite
from test_engine_junit import junit_to_xml_report_string
from test_engine_junit import junit_to_xml_report_file

################################################################################

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

class SSTTestCase(unittest.TestCase):
    """ This class is the SST TestCase class """

    def __init__(self, methodName):
        # NOTE: __init__ is called for all testscripts before any setUpModules
        #       and the like are called.
        super(SSTTestCase, self).__init__(methodName)
        #log_forced("DEBUG SSTTestCase __init__")
        self.testName = methodName

###

    def setUp(self):
        """ Called when the TestCase is starting up """
        #log_forced("DEBUG SSTTestCase setUp()")
        test_engine_globals.TESTSUITE_NAME_STR = ("{0}".format(strclass(self.__class__)))

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

    def run_sst(self, sdl_file, out_file, mpi_out_files="", other_args="",
               num_ranks=None, num_threads=None, global_args=None,
               timeout_sec=60):
        """ TODO: Launch sst with with the command line and send output to the
            output file.  Other parameters can also be passed in.
           :param: sdl_file (str): The FilePath to the sdl file
           :param: other_args (str): Any other arguments used in the SST cmd
           :param: out_file (str): The FilePath to the finalized output file
           :param: mpi_out_files (str): The FilePath to the mpi run output files
                                        These will be merged into the out_file
           :param: num_ranks (int): The number of ranks to run SST with
           :param: num_threads (int): The number of threads to run SST with
           :param: global_args (str): Global Arguments provided from test engine args
           :param: timeout_sec (int|float): Allowed runtime in seconds
        """
        # We cannot set the default of param to the global variable due to
        # oddities on how this class loads.
        if num_ranks == None:
            num_ranks=test_engine_globals.SSTRUNNUMRANKS
        if num_threads == None:
            num_threads=test_engine_globals.SSTRUNNUMTHREADS
        if global_args == None:
            global_args=test_engine_globals.SSTRUNGLOBALARGS

        check_param_type("sdl_file", sdl_file, str)
        check_param_type("out_file", out_file, str)
        check_param_type("mpi_out_files", mpi_out_files, str)
        check_param_type("other_args", other_args, str)
        check_param_type("global_args", global_args, str)
        check_param_type("num_ranks", num_ranks, int)
        check_param_type("num_threads", num_threads, int)
        if not (isinstance(timeout_sec, (int, float)) and not isinstance(timeout_sec, bool)):
            raise ValueError("ERROR: Timeout_sec must be a postive int or a float")

        if not os.path.exists(sdl_file) or not os.path.isfile(sdl_file):
            log_error("sdl_file {0} does not exist".format(sdl_file))

        if mpi_out_files == "":
            mpiout_filename = out_file
        else:
            mpiout_filename = mpi_out_files

        if num_threads > 1:
            oscmd = "sst -n {0} {1} {2} {3}".format(num_threads,
                                                    global_args,
                                                    other_args,
                                                    sdl_file)
        else:
            oscmd = "sst {0} {1} {2}".format(global_args,
                                             other_args,
                                             sdl_file)

        num_cores = _get_num_cores_on_system()
        if num_ranks > 1:
            numa_param=""
            numa_param = ""
            if num_cores >= 2 and num_cores <= 4:
                numa_param = "-map-by numa:pe=2 -oversubscribe"
            elif num_cores >= 4:
                numa_param = "-map-by numa:pe=2"

            oscmd = "mpirun -np {0} {1} -output-filename {2} {3}".format(num_ranks,
                                                                         numa_param,
                                                                         mpiout_filename,
                                                                         oscmd)
        log_debug((("--SST Launch Command (Ranks:{0}; Threads:{1};") +
                   (" Num Cores:{2}) = {3}")).format(num_ranks, num_threads,
                                                     num_cores, oscmd))

        if num_ranks > 1:
            rtn = OSCommand(oscmd).run(timeout_sec=timeout_sec)
            merge_files("{0}*".format(mpiout_filename), out_file)
        else:
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
# Commandline Information Functions
################################################################################

def is_testing_in_debug_mode():
    """ Identify if test engine is in debug mode
       :return: True if in debug mode
    """
    return test_engine_globals.DEBUGMODE

###

def get_testing_num_ranks():
    """ Get the number of ranks defined to be run during the testing
       :return: The number of requested run ranks
    """
    return test_engine_globals.SSTRUNNUMRANKS

###

def get_testing_num_threads():
    """ Get the number of threads defined to be run during the testing
       :return: The number of requested run threads
    """
    return test_engine_globals.SSTRUNNUMTHREADS

################################################################################
# System Information Functions
################################################################################

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

def _get_linux_distribution():
    """ Return the linux distribution info as a tuple"""
    # The method linux_distribution is depricated in depricated in Py3.5
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
# SST Configuration file (sstsimulator.conf) Access Functions
################################################################################

def get_sst_config_value_str(section, key, default=None):
    """ Retrieve a Section/Key from the SST Configuration File
       :param: section (str): The [section] to look for the key
       :param: key (str): The key to find
       :param: default (str): Default Return if failure occurs
       :return (str): The returned data or default if not found in file
       This will raise a SSTTestCaseException if a default is not provided
    """
    check_param_type("section", section, str)
    check_param_type("key", key, str)
    if default != None:
        check_param_type("default", default, str)
    core_conf_file_parser = test_engine_globals.CORECONFFILEPARSER
    try:
        return core_conf_file_parser.get(section, key)
    except configparser.Error as exc_e:
        rtn = _handle_config_err(exc_e, default)
        if default is None:
            raise SSTTestCaseException(exc_e)
        return rtn

###

def get_sst_config_value_int(section, key, default=None):
    """ Retrieve a Section/Key from the SST Configuration File
       :param: section (str): The [section] to look for the key
       :param: key (str): The key to find
       :param: default (int): Default Return if failure occurs
       :return (float): The returned data or default if not found in file
       This will raise a SSTTestCaseException if a default is not provided
    """
    check_param_type("section", section, str)
    check_param_type("key", key, str)
    if default != None:
        check_param_type("default", default, int)
    core_conf_file_parser = test_engine_globals.CORECONFFILEPARSER
    try:
        return core_conf_file_parser.getint(section, key)
    except configparser.Error as exc_e:
        rtn = _handle_config_err(exc_e, default)
        if default is None:
            raise SSTTestCaseException(exc_e)
        return rtn
###

def get_sst_config_value_float(section, key, default=None):
    """ Retrieve a Section/Key from the SST Configuration File
       :param: section (str): The [section] to look for the key
       :param: key (str): The key to find
       :param: default (float): Default Return if failure occurs
       :return (float): The returned data or default if not found in file
       This will raise a SSTTestCaseException if a default is not provided
    """
    check_param_type("section", section, str)
    check_param_type("key", key, str)
    if default != None:
        check_param_type("default", default, float)
    core_conf_file_parser = test_engine_globals.CORECONFFILEPARSER
    try:
        return core_conf_file_parser.getfloat(section, key)
    except configparser.Error as exc_e:
        rtn = _handle_config_err(exc_e, default)
        if default is None:
            raise SSTTestCaseException(exc_e)
        return rtn

###

def get_sst_config_value_bool(section, key,default=None):
    """ Retrieve a Section/Key from the SST Configuration File
       :param: section (str): The [section] to look for the key
       :param: key (str): The key to find
       :param: default (bool): Default Return if failure occurs
       :return (bool): The returned data or default if not found in file
       NOTE: "1", "yes", "true", and "on" return True
       This will raise a SSTTestCaseException if a default is not provided
    """
    check_param_type("section", section, str)
    check_param_type("key", key, str)
    if default != None:
        check_param_type("default", default, bool)
    core_conf_file_parser = test_engine_globals.CORECONFFILEPARSER
    try:
        return core_conf_file_parser.getbool(section, key)
    except configparser.Error as exc_e:
        rtn = _handle_config_err(exc_e, default)
        if default is None:
            raise SSTTestCaseException(exc_e)
        return rtn

###

def get_sst_config_sections():
    """ Retrieve a list of sections that exist in the SST Configuration File
       :return (list of str): The list of sections in the file
       This will raise a SSTTestCaseException if an error occurs
    """
    core_conf_file_parser = test_engine_globals.CORECONFFILEPARSER
    try:
        return core_conf_file_parser.sections()
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def get_sst_config_section_keys(section):
    """ Retrieve a list of keys under a section that exist in the SST Configuration File
       :param: section (str): The [section] to look for the key
       :return (list of str): The list of keys under a section in the file
       This will raise a SSTTestCaseException if an error occurs
    """
    check_param_type("section", section, str)
    core_conf_file_parser = test_engine_globals.CORECONFFILEPARSER
    try:
        return core_conf_file_parser.options(section)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def get_all_sst_config_keys_values_from_section(section):
    """ Retrieve a list of tuples that contain all the key, value pairs
        under a section that exists in the SST Configuration File
       :param: section (str): The [section] to look for the key
       :return (list of tuples): The list of tuples of key, value pairs
       This will raise a SSTTestCaseException if an error occurs
    """
    check_param_type("section", section, str)
    core_conf_file_parser = test_engine_globals.CORECONFFILEPARSER
    try:
        return core_conf_file_parser.items(section)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def does_sst_config_have_section(section):
    """ Check if the SST Configuration File has a section
       :param: section (str): The [section] to look for the key
       :return (bool):
       This will raise a SSTTestCaseException if an error occurs
    """
    check_param_type("section", section, str)
    core_conf_file_parser = test_engine_globals.CORECONFFILEPARSER
    try:
        return core_conf_file_parser.has_section(section)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def does_sst_config_section_have_key(section, key):
    """ Check if the SST Configuration File has a key under a section
       :param: section (str): The [section] to look for the key
       :param: key (str): The key to find
       :return (bool):
       This will raise a SSTTestCaseException if an error occurs
    """
    check_param_type("section", section, str)
    check_param_type("key", key, str)
    core_conf_file_parser = test_engine_globals.CORECONFFILEPARSER
    try:
        return core_conf_file_parser.has_option(section, key)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def _handle_config_err(exc_e, default_rtn_data):
    errmsg = "Reading SST-Core Config file sstsimulator.conf - {0}".format(exc_e)
    log_warning(errmsg)
    return default_rtn_data

################################################################################
# Logging Functions
################################################################################

def log(logstr):
    """ Log a message, this will not output unless we are
        outputing in verbose mode.
       :param: logstr = string to be logged
    """
    check_param_type("logstr", logstr, str)
    if test_engine_globals.VERBOSITY >= test_engine_globals.VERBOSE_LOUD:
        log_forced(logstr)

###

def log_forced(logstr):
    """ Log a message, no matter what the verbosity is
        if in the middle of testing, precede with a \n to slip in-between
        unittest outputs
        :param: logstr = string to be logged
    """
    check_param_type("logstr", logstr, str)
    extra_lf = ""
    if test_engine_globals.TESTRUNNINGFLAG:
        extra_lf = "\n"
    print(("{0}{1}".format(extra_lf, logstr)))

###

def log_info(logstr):
    """ Log a INFO: message, no matter what the verbosity is
        :param: logstr = string to be logged
    """
    check_param_type("logstr", logstr, str)
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
    check_param_type("logstr", logstr, str)
    finalstr = "ERROR: {0}".format(logstr)
    log_forced(finalstr)

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
   """ Sort a output file along with a reference file and compare them
       :param: outfile (str) Path to the output file
       :param: reffile (str) Path to the reference file
       :return: True if the 2 sorted file match
   """
   sorted_outfile = "{1}/{0}_sorted_outfile".format(test_name, get_test_output_tmp_dir())
   sorted_reffile = "{1}/{0}_sorted_reffile".format(test_name, get_test_output_tmp_dir())

   os.system("sort -o {0} {1}".format(sorted_outfile, outfile))
   os.system("sort -o {0} {1}".format(sorted_reffile, reffile))

   return filecmp.cmp(sorted_outfile, sorted_reffile)

###

def merge_files(filepath_wildcard, outputfilepath):
    """ Merge a group of common files into an output file
       :param: filepath_wildcard (str) The wildcard Path to the files to be mreged
       :param: outputfilepath (str) The output file path
    """
    cmd = "cat {0} > {1}".format(filepath_wildcard, outputfilepath)
    os.system(cmd)

################################################################################
### OS Basic Commands
################################################################################

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

################################################################################
### Platform Specific Support Functions
################################################################################
def _get_num_cores_on_system():
    """ Figure out how many cores exist on the system"""
    num_cores = 1
    if is_host_os_osx():
        cmd = "sysctl -n hw.ncpu"
        rtn = OSCommand(cmd).run()
        num_cores = int(rtn.output())
    else:
        cmd = "cat /proc/cpuinfo |grep processor | wc -l"
        rtn = OSCommand(cmd).run()
        num_cores = int(rtn.output())

    return num_cores
