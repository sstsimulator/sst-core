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

""" This module is the main testing engine.  It will init testing variable,
    parse the cmd line vars, read the sstsimulator.conf file to find where
    testsuites should live, and then discover and run tests.
"""

import sys
import os
import unittest
import argparse
# ConfigParser module changes name between Py2->Py3
try:
    import configparser
except ImportError:
    import ConfigParser as configparser

import test_engine_globals
from sst_unittest_support import log_fatal
from sst_unittest_support import log_notice
from sst_unittest_support import log_debug
from sst_unittest_support import log_error
from sst_unittest_support import log_warning
from sst_unittest_support import log_forced

################################################################################

REQUIRED_PY_MAJ_VER_2 = 2 # Required Major Version Min
REQUIRED_PY_MAJ_VER_MAX = 3 # Required Major Version Max
REQUIRED_PY_MAJ_VER_2_MINOR_VER = 7 # Required Minor Version

HELP_DESC = 'Run {0} Tests'
HELP_EPILOG = (("Python files named TestSuite*.py found at ") +
               ("or below the defined test directory(s) will be run."))

# AVAILABLE TEST MODES
MODE_TEST_SST_CORE = 0
MODE_TEST_REGISTRED_ELEMENTS = 1

################################################################################

class TestEngine():
    """ This is the main Test Engine, it will init arguments, parsed params,
        create output directories, and then Discover and Run the tests.
    """

    def __init__(self, sst_core_bin_dir, run_core_tests):
        """ Initialize the TestEngine object, and parse the user arguments
            :param: sst_core_bin_dir = The SST-Core binary directory
            :param: run_core_tests = True for Core Tests, False for Elements tests
        """
        ver = self._validate_python_version()
        self._init_test_engine_variables(sst_core_bin_dir, run_core_tests)
        self._parse_arguments()
        log_notice(("SST Test Engine Instantiated - Running") +
                   (" tests on {0}").format(self._test_type_str))
        log_debug("Python Version = {0}.{1}.{2}".format(ver[0], ver[1], ver[2]))

####

    def discover_and_run_tests(self):
        """ Create the output directories, then discover the tests, and then
            run them using pythons unittest module
        """
        self._create_all_output_directories()
        self._discover_tests()
        log_forced(("\n=== TESTS STARTING ================") +
                   ("===================================\n"))
        sst_tests_results = unittest.TextTestRunner(verbosity=test_engine_globals.VERBOSITY,
                                                    failfast=self._fail_fast).\
                                                    run(self._sst_full_test_suite)

################################################################################

    def _validate_python_version(self):
        """ Validate that we are running on a supported Python version.
        """
        ver = sys.version_info
        # Check for Py2.x or Py3.x Versions
        if (ver[0] < REQUIRED_PY_MAJ_VER_2) or (ver[0] > REQUIRED_PY_MAJ_VER_MAX):
            log_fatal(("SST Test Engine requires Python major version {1} or {2}\n" +
                       "Found Python version is:\n{3}").format(os.path.basename(__file__),
                                                               REQUIRED_PY_MAJ_VER_2,
                                                               REQUIRED_PY_MAJ_VER_MAX,
                                                               sys.version))

        # Check to ensure minimum Py2.7
        if (ver[0] == REQUIRED_PY_MAJ_VER_2) and (ver[1] < REQUIRED_PY_MAJ_VER_2_MINOR_VER):
            log_fatal(("SST Test Engine requires Python version {1}.{2} or greater\n" +
                       "Found Python version is:\n{3}").format(os.path.basename(__file__),
                                                               REQUIRED_PY_MAJ_VER_2,
                                                               REQUIRED_PY_MAJ_VER_2_MINOR_VER,
                                                               sys.version))
        return ver

####

    def _init_test_engine_variables(self, sst_core_bin_dir, run_core_tests):
        """ Initialize the variables needed for testing.  This will also
            initialize the global variables
            :param: sst_core_bin_dir = The SST-Core binary directory
            :param: run_core_tests = True for Core Tests, False for Elements Tests
        """
        # Init some internal variables
        self._fail_fast = False
        self._list_of_searchable_testsuite_paths = ""
        self._sst_core_bin_dir = sst_core_bin_dir
        self._core_test_mode = run_core_tests
        self._sst_full_test_suite = unittest.TestSuite()
        if self._core_test_mode:
            self._test_type_str = "SST-Core"
        else:
            self._test_type_str = "Registered Elements"

        test_engine_globals.init_test_engine_globals()

####

    def _parse_arguments(self):
        """ Parse the cmd line arguments
        """
        # Build a parameter parser, adjust its help based upon the test type
        helpdesc = HELP_DESC.format(self._test_type_str)
        parser = argparse.ArgumentParser(description=helpdesc,
                                         epilog=HELP_EPILOG)
        if self._core_test_mode:
            testsuite_path_str = "Files/Dirs to SST-Core TestSuites"
        else:
            testsuite_path_str = "Files/Dirs to Registered Elements TestSuites"
        parser.add_argument('list_of_paths', metavar='testsuite_paths', nargs='*',
                            default=[], help=testsuite_path_str)
        mutgroup = parser.add_mutually_exclusive_group()
        mutgroup.add_argument('-v', '--verbose', action='store_true',
                              help='Run tests in verbose mode')
        mutgroup.add_argument('-q', '--quiet', action='store_true',
                              help='Run tests in quiet mode')
        mutgroup.add_argument('-d', '--debug', action='store_true',
                              help='Run tests in test engine debug mode')
        parser.add_argument('-r', '--ranks', type=int, metavar="XX",
                            nargs=1, default=0,
                            help='Run with XX ranks')
        parser.add_argument('-t', '--threads', type=int, metavar="YY",
                            nargs=1, default=0,
                            help='Run with YY threads')
        parser.add_argument('-f', '--fail_fast', action='store_true',
                            help='Stop testing on failure')
        parser.add_argument('-o', '--out_dir', type=str, metavar='dir',
                            nargs='?', default='./sst_test_outputs',
                            help='Set output directory')

        args = parser.parse_args()
        self._decode_parsed_arguments(args)

####

    def _decode_parsed_arguments(self, args):
        """ Decode the parsed arguments into their class or global variables
            :param: args = The arguments from the cmd line parser
        """
        # Extract the Arguments into the class variables
        self._fail_fast = args.fail_fast
        self._list_of_searchable_testsuite_paths = args.list_of_paths
        test_engine_globals.VERBOSITY = test_engine_globals.VERBOSE_NORMAL
        if args.quiet:
            test_engine_globals.VERBOSITY = test_engine_globals.VERBOSE_QUIET
        if args.verbose:
            test_engine_globals.VERBOSITY = test_engine_globals.VERBOSE_LOUD
        if args.debug:
            test_engine_globals.DEBUGMODE = True
            test_engine_globals.VERBOSITY = test_engine_globals.VERBOSE_DEBUG
        test_engine_globals.NUMRANKS = args.ranks
        test_engine_globals.NUMTHREADS = args.threads
        test_engine_globals.TESTOUTPUTTOPDIRPATH = args.out_dir
        test_engine_globals.TESTOUTPUTRUNDIRPATH = "{0}/run_data".format(args.out_dir)
        test_engine_globals.TESTOUTPUTTMPDIRPATH = "{0}/tmp_data".format(args.out_dir)

####

    def _create_output_dir(self, out_dir):
        """ Look to see if an output dir exists.  If not, try to create it
            :param: out_dir = The path to the output directory
            :return: True if output dir is created (did not exist); else False
        """
        # Is there an directory already existing
        if not os.path.isdir(out_dir):
            try:
                os.makedirs(out_dir, mode=0o744)
                return True
            except OSError as exc_e:
                log_error("os.mkdirs Exception - ({0})".format(exc_e))
            if not os.path.isdir(out_dir):
                log_fatal((("Output Directory {0} - Does not exist and ") +
                           ("cannot be created")).format(out_dir))
        return False

####

    def _create_all_output_directories(self):
        """ Create the output directories if needed
        """
        top_dir = test_engine_globals.TESTOUTPUTTOPDIRPATH
        run_dir = test_engine_globals.TESTOUTPUTRUNDIRPATH
        tmp_dir = test_engine_globals.TESTOUTPUTTMPDIRPATH
        xml_dir = test_engine_globals.TESTOUTPUTTOPDIRPATH + "/xml_data"
        if self._create_output_dir(top_dir):
            log_notice("SST Test Output Dir Created at {0}".format(top_dir))
        self._create_output_dir(run_dir)
        self._create_output_dir(tmp_dir)
        self._create_output_dir(xml_dir)
        log_notice("Test Output Directory = {0}".format(top_dir))
        log_debug(" - Test Output Run Directory = {0}".format(run_dir))
        log_debug(" - Test Output Tmp Directory = {0}".format(tmp_dir))
        log_debug(" - Test Output XML Directory = {0}".format(xml_dir))

####

    def _create_core_config_parser(self):
        """ Create an Core Configurtion (INI format) parser.  This will allow
            us to search the Core configuration looking for test file paths
            :return: An ConfParser.RawConfigParser object
        """
        core_conf_file_dir = self._sst_core_bin_dir + "/../etc/sst/"
        core_conf_file_path = core_conf_file_dir + "sstsimulator.conf"
        if not os.path.isdir(core_conf_file_dir):
            log_fatal((("SST-Core Directory {0} - Does not exist - ") +
                       ("testing cannot continue")).format(core_conf_file_dir))
        if not os.path.isfile(core_conf_file_path):
            log_fatal((("SST-Core Configuration File {0} - Does not exist - ") +
                       ("testing cannot continue")).format(core_conf_file_path))

        # Instantiate a ConfigParser to read the data in the config file
        try:
            core_conf_file_parser = configparser.RawConfigParser()
            core_conf_file_parser.read(core_conf_file_path)
        except configparser.Error as exc_e:
            log_fatal((("Test Engine: Cannot read SST-Core Configuration ") +
                       ("File:\n{0}\n- testing cannot continue") +
                       ("({1})")).format(core_conf_file_path, exc_e))

        return core_conf_file_parser

###

    def _build_list_of_testsuite_dirs(self):
        """ Using a config file parser, build a list of test Suite Dirs.
            Note: The discovery method of Test Suites is different
                  depending if we are testing core vs registered elements.
        """
        final_rtn_paths = []
        testsuite_paths = []

        # Build the Config File Parser
        core_conf_file_parser = self._create_core_config_parser()

        # Now read the appropriate type of data (Core or Elements)
        try:
            if self._core_test_mode:
                # Find the tests dir in the core
                section = "SSTCore"
                cfg_path_data = core_conf_file_parser.get(section, "testsdir")
                testsuite_paths.append(cfg_path_data)
            else:
                # Find the testsuites dir for each registered element
                section = "SST_ELEMENT_TESTS"
                cfg_path_data = core_conf_file_parser.items(section)
                for path_data in cfg_path_data:
                    testsuite_paths.append(path_data[1])

        except configparser.Error as exc_e:
            errmsg = (("Reading SST-Core Config file section ") +
                      ("{0} - {1} ")).format(section, exc_e)
            log_error(errmsg)

        # Now verify each path is valid
        for suite_path in testsuite_paths:
            if not os.path.isdir(suite_path):
                log_warning((("TestSuite Directory {0} - Does not exist; ") +
                             ("No tests will be performed...")).format(suite_path))
            else:
                final_rtn_paths.append(suite_path)

        return final_rtn_paths

####

    def _dump_testsuite(self, suite):
        """ Recursively log all tests in a TestSuite
            :param: The current suite to print
        """
        if hasattr(suite, '__iter__'):
            for sub_suite in suite:
                self._dump_testsuite(sub_suite)
        else:
            log_debug("- {0}".format(suite))

####

    def _discover_tests(self):
        """ Figure out the list of paths we are searching for testsuites.  The
            user may have given us a list via the cmd line, so that takes priority
        """
        # Did the user give us any test suite paths in the cmd line
        if len(self._list_of_searchable_testsuite_paths) == 0:
            self._list_of_searchable_testsuite_paths = self._build_list_of_testsuite_dirs()

        # Check again to see if no Test Suite Paths
        if len(self._list_of_searchable_testsuite_paths) == 0:
            log_warning("No TestSuite dirs/files have been defined/found")

        # Debug dump of search paths
        log_debug("SEARCH DIRS/FILES OF TESTSUITES:")
        for search_path in self._list_of_searchable_testsuite_paths:
            log_debug("- {0}".format(search_path))

        # Discover tests in each Test Path directory and add to the test suite
        sst_pattern = 'testsuite*.py'
        for testsuite_path in self._list_of_searchable_testsuite_paths:
            # Is this testsuite_path a testsuites dir or a testsuite file?
            if os.path.isdir(testsuite_path):
                sst_testsuites = unittest.TestLoader().discover(start_dir=testsuite_path,
                                                                pattern=sst_pattern)
                self._sst_full_test_suite.addTests(sst_testsuites)
            if os.path.isfile(testsuite_path):
                testsuite_dir = os.path.abspath(os.path.dirname(testsuite_path))
                basename = os.path.basename(testsuite_path)
                sst_testsuites = unittest.TestLoader().discover(start_dir=testsuite_dir,
                                                                pattern=basename)
                self._sst_full_test_suite.addTests(sst_testsuites)

        log_debug("DISCOVERED TESTS (FROM TESTSUITES):")
        self._dump_testsuite(self._sst_full_test_suite)

        # Warn the user if no testssuites/testcases are found
        if self._sst_full_test_suite.countTestCases() == 0:
            log_warning(("No TestSuites (with TestCases) have been found ") +
                        ("- verify the search paths"))
            log_forced("SEARCH PATHS FOR TESTSUITES:")
            for search_path in self._list_of_searchable_testsuite_paths:
                log_forced("- {0}".format(search_path))
