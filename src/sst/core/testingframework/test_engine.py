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

""" This module is the main testing engine.  It will init testing variables,
    parse the cmd line vars, read the sstsimulator.conf file to find where
    testsuites should live, and then discover and run tests.
"""

import sys
import os
import unittest
import argparse
import shutil

# ConfigParser module changes name between Py2->Py3
try:
    import configparser
except ImportError:
    import ConfigParser as configparser

import test_engine_globals
from sst_unittest import *
from sst_unittest_support import *
from test_engine_unittest import *

################################################################################

REQUIRED_PY_MAJ_VER_2 = 2 # Required Major Version Min
REQUIRED_PY_MAJ_VER_MAX = 3 # Required Major Version Max
REQUIRED_PY_MAJ_VER_2_MINOR_VER = 7 # Required Minor Version
DEF_THREAD_LIMIT = 8

HELP_DESC = 'Run {0} Tests'
HELP_EPILOG = (
               ("Finding TestSuites:\n") +
               ("During Startup, the 'list_of_paths' and 'testsuite_types' arguments are used\n") +
               ("to create a list of testsuites to be run.\n") +
               (" - If the 'list_of_paths' argument includes a testsuite file, that testsuite\n") +
               ("   file will be directly added to the list of testsuites to be run.\n") +
               (" - If the 'list_of_paths' argument includes a directory (containing 1 or more \n") +
               ("   testsuites), that directory will be searched for specific testsuites types as\n") +
               ("   described below.\n") +
               (" - If the 'list_of_paths' argument is empty (default), testsuites paths found \n") +
               ("   in the sstsimulator.conf file (located in the <sstcore_install>/etc directory)\n") +
               ("   will be searched for specific testsuite types as described below.\n") +
               ("\n") +
               ("Searching for testsuites types:\n") +
               ("Each directory identified by the 'list_of_paths' argument will search for \n") +
               ("testsuites based upon the 'testsuite_type' argument as follows:\n") +
               (" - Files named 'testsuite_default_*.py' will be added to the list of\n") +
               ("   testsuites to be run if argument --testsuite_type is NOT specified.\n") +
               ("   Note: This will run only the 'default' set of testsuites in the directory.\n") +
               (" - Files named 'testsuite_<type_name>_*.py' will be added to the list of\n") +
               ("   testsuites to be run when <typename> is specifed using the \n") +
               ("   --testsuite_type=<type_name> argument.\n") +
               ("   Note: This will run user selected set of testsuites in the directory.\n") +
               (" - Files named 'testsuite_*.py' will be added to the list of testsuites to\n") +
               ("   be run when argument --testsuite_type=all is specified.\n") +
               ("   Note: This will run ALL of the testsuites in the directory.\n") +
               ("\n") +
               ("Tests Execution:\n") +
               ("All tests identified inside of the testsuites to be given an opportunity to\n") +
               ("run.  There will be situations where a tests may not be able to run and will be \n") +
               ("skipped (cannot run on the OS, or build configuration does not support the\n") +
               ("test).\n") +
               (" - The decision to skip is made in the testsuite source code.\n") +
               ("\n") +
               ("Test Scenarios:\n") +
               ("Tests and TestCases identified in testsuites can be skipped from running\n") +
               ("by using the '--scenarios' argument.  1 or more scenarios can be defined\n") +
               ("concurrently.\n") +
               (" - The decision to skip is made in the testsuite source code.\n") +
               (" \n")
              )

# AVAILABLE TEST MODES
MODE_TEST_ELEMENTS = 0
MODE_TEST_SST_CORE = 1

################################################################################

class TestEngine():
    """ This is the main Test Engine, it will init arguments, parsed params,
        create output directories, and then Discover and Run the tests.
    """

    def __init__(self, sst_core_bin_dir, test_mode):
        """ Initialize the TestEngine object, and parse the user arguments
            :param: sst_core_bin_dir - The SST-Core binary directory
            :param: test_mode = 1 for Core Testing, 0 for Elements testingt
        """
        ver = self._validate_python_version()
        self._init_test_engine_variables(sst_core_bin_dir, test_mode)
        self._parse_arguments()
        verify_concurrent_test_engine_available()
        self._display_startup_info()

####

    def discover_and_run_tests(self):
        """ Create the output directories, then discover the tests, and then
            run them using pythons unittest module
        """
        self._create_all_output_directories()
        # Build the Config File Parser
        test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER = self._create_core_config_parser()
        test_engine_globals.TESTENGINE_CORE_CONFINCLUDE_DICT = self._build_core_config_include_defs_dict()

        self._discover_testsuites()
        try:
            # Convert test suites to a Concurrent Test Suite
            self._sst_full_test_suite = SSTTestSuite(self._sst_full_test_suite,
                                                     self._built_tests_list)

            test_runner = SSTTextTestRunner(verbosity=test_engine_globals.TESTENGINE_VERBOSITY,
                                            failfast=self._fail_fast,
                                            resultclass=SSTTextTestResult)
            sst_tests_results = test_runner.run(self._sst_full_test_suite)

            if not test_runner.did_tests_pass(sst_tests_results):
                return 1
            return 0

        # Handlers of unittest.TestRunner exceptions
        except KeyboardInterrupt:
            log_fatal("TESTING TERMINATED DUE TO KEYBOARD INTERRUPT...")


####

    def _built_tests_list(self, suite):
        tests = list(iterate_tests(suite))
        return tests

################################################################################
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

    def _init_test_engine_variables(self, sst_core_bin_dir, test_mode):
        """ Initialize the variables needed for testing.  This will also
            initialize the global variables
            :param: sst_core_bin_dir = The SST-Core binary directory
            :param: test_mode = 1 for Core Testing, 0 for Elements testing
        """
        # Init some internal variables
        self._fail_fast = False
        self._keep_output_dir = False
        self._list_of_searchable_testsuite_paths = []
        self._testsuite_types_list = []
        self._sst_core_bin_dir = sst_core_bin_dir
        self._test_mode = test_mode
        self._sst_full_test_suite = unittest.TestSuite()
        if self._test_mode:
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
                                         formatter_class=argparse.RawTextHelpFormatter,
                                         epilog=HELP_EPILOG)

        out_mode_group = parser.add_argument_group('Output Mode Arguments')
        mutgroup = out_mode_group.add_mutually_exclusive_group()
        mutgroup.add_argument('-v', '--verbose', action='store_true',
                              help='Run tests in verbose mode')
        mutgroup.add_argument('-q', '--quiet', action='store_true',
                              help='Run tests in quiet mode')
        mutgroup.add_argument('-d', '--debug', action='store_true',
                              help='Run tests in debug mode')

        run_group = parser.add_argument_group('SST Run Options')
        run_group.add_argument('-s', '--scenarios', type=str, metavar="name",
                               nargs="+", default=[],
                               help=(('Names of test scenarios that filter') + \
                                     (' tests\nto be run [NONE]')))
        run_group.add_argument('-r', '--ranks', type=int, metavar="XX",
                               nargs=1, default=[1],
                               help='Run with XX ranks [1]')
        run_group.add_argument('-t', '--threads', type=int, metavar="YY",
                               nargs=1, default=[1],
                               help='Run with YY threads [1]')
        run_group.add_argument('-a', '--sst_run_args', type=str, metavar='" --arg1 -a2"',
                               nargs=1, default=[''],
                               help=('Runtime args for all SST runs (must be\n')
                                  + ('identifed as a string; Note:extra space at front)'))

        parser.add_argument('-f', '--fail_fast', action='store_true',
                            help='Stop testing on failure [true]')
        parser.add_argument('-k', '--keep_output', action='store_true',
                            help='Dont clean output directory at start [False]')
        parser.add_argument('-o', '--out_dir', type=str, metavar='dir',
                            nargs=1, default=['./sst_test_outputs'],
                            help='Set output directory [./sst_test_outputs]')
        parser.add_argument('-c', '--concurrent', type=int, metavar="TT",
                            nargs='?', const=DEF_THREAD_LIMIT,
                            help=('Run Test Suites concurrently using threads')
                               + (' TT = thread limit [default {0}]').format(DEF_THREAD_LIMIT))

        discover_group = parser.add_argument_group('Test Discovery Arguments')
        discover_group.add_argument('-y', '--testsuite_types', type=str, metavar="name",
                                    nargs="+", default=['default'],
                                    help=(('Name (in lowercase) of testsuite types to') + \
                                          (' run\n("all" will run all types) ["default"]')))
        if self._test_mode:
            testsuite_path_str = "TestSuite Files or Dirs to SST-Core TestSuites"
        else:
            testsuite_path_str = "Testsuite Files or Dirs to Registered Elements TestSuites"
        discover_group.add_argument('-p', '--list_of_paths', metavar='path',
                                    nargs='*', default=[], help=testsuite_path_str)

        args = parser.parse_args()
        self._decode_parsed_arguments(args, parser)

####

    def _decode_parsed_arguments(self, args, parser):
        """ Decode the parsed arguments into their class or global variables
            :param: args = The arguments from the cmd line parser
            :param: parser = The parser in case help need to be printed.
        """
        # Extract the Arguments into the class variables
        self._fail_fast = args.fail_fast
        self._keep_output_dir = args.keep_output
        self._list_of_searchable_testsuite_paths = args.list_of_paths
        lc_testscenario_list = [item.lower() for item in args.scenarios]
        test_engine_globals.TESTENGINE_SCENARIOSLIST = lc_testscenario_list
        lc_testsuitetype_list = [item.lower() for item in args.testsuite_types]
        self._testsuite_types_list = lc_testsuitetype_list
        test_engine_globals.TESTENGINE_VERBOSITY = test_engine_globals.VERBOSE_NORMAL
        if args.quiet:
            test_engine_globals.TESTENGINE_VERBOSITY = test_engine_globals.VERBOSE_QUIET
        if args.verbose:
            test_engine_globals.TESTENGINE_VERBOSITY = test_engine_globals.VERBOSE_LOUD
        if args.debug:
            test_engine_globals.TESTENGINE_DEBUGMODE = True
            test_engine_globals.TESTENGINE_VERBOSITY = test_engine_globals.VERBOSE_DEBUG
        test_engine_globals.TESTENGINE_CONCURRENTMODE = False
        test_engine_globals.TESTENGINE_THREADLIMIT = DEF_THREAD_LIMIT
        if args.concurrent != None:
            if args.concurrent > 0:
                test_engine_globals.TESTENGINE_CONCURRENTMODE = True
                test_engine_globals.TESTENGINE_THREADLIMIT = args.concurrent
            else:
                parser.print_help()
                log_fatal("Thread limit must be > 0; you provided {0}".format(args.concurrent))
        test_engine_globals.TESTENGINE_SSTRUNNUMRANKS = args.ranks[0]
        test_engine_globals.TESTENGINE_SSTRUNNUMTHREADS = args.threads[0]
        test_engine_globals.SSTRUNGLOBALARGS = args.sst_run_args[0]
        test_engine_globals.TESTOUTPUT_TOPDIRPATH = os.path.abspath(args.out_dir[0])
        test_engine_globals.TESTOUTPUT_RUNDIRPATH = os.path.abspath("{0}/run_data".format(args.out_dir[0]))
        test_engine_globals.TESTOUTPUT_TMPDIRPATH = os.path.abspath("{0}/tmp_data".format(args.out_dir[0]))
        test_engine_globals.TESTOUTPUT_XMLDIRPATH = os.path.abspath("{0}/xml_data".format(args.out_dir[0]))
        if args.ranks[0] < 0:
            log_fatal("ranks must be >= 0; currently set to {0}".format(args.ranks[0]))
        if args.threads[0] < 0:
            log_fatal("threads must be >= 0; currently set to {0}".format(args.threads[0]))

####

    def _display_startup_info(self):

        ver = sys.version_info
        concurrent_txt = ""

        if test_engine_globals.TESTENGINE_CONCURRENTMODE:
            concurrent_txt = "[CONCURRENTLY ({0} Threads)]".format(test_engine_globals.TESTENGINE_THREADLIMIT)

        # Display operations info if we are unning in a verbose mode
        log_info(("SST Test Engine Instantiated - Running") +
                 (" tests on {0} {1}").format(self._test_type_str, concurrent_txt),
                 forced=False)

        log_info(("Test Platform = {0}".format(get_host_os_distribution_type())) +
                 (" {0}".format(get_host_os_distribution_version())), forced=False)

        if 'all' in self._testsuite_types_list:
            log_info("TestSuite Types to be run are: ALL TESTSUITE TYPES",
                     forced=False)
        else:
            log_info(("TestSuite Types to be run are: ") +
                     ("{0}").format(" ".join(self._testsuite_types_list)),
                     forced=False)

        if len(test_engine_globals.TESTENGINE_SCENARIOSLIST) == 0:
            log_info("Test scenarios to filter tests: NONE", forced=False)
        else:
            log_info(("Test Scenarios to filter tests: ") +
                     ("{0}").format(" ".join(test_engine_globals.TESTENGINE_SCENARIOSLIST)),
                     forced=False)

        log_debug("Python Version = {0}.{1}.{2}".format(ver[0], ver[1], ver[2]))

####

    def _create_all_output_directories(self):
        """ Create the output directories if needed
        """
        top_dir = test_engine_globals.TESTOUTPUT_TOPDIRPATH
        run_dir = test_engine_globals.TESTOUTPUT_RUNDIRPATH
        tmp_dir = test_engine_globals.TESTOUTPUT_TMPDIRPATH
        xml_dir = test_engine_globals.TESTOUTPUT_XMLDIRPATH
        if not self._keep_output_dir:
            log_debug("Deleting output directory {0}".format(top_dir))
            shutil.rmtree(top_dir, True)
        if self._create_output_dir(top_dir):
            log_debug("SST Test Output Dir Created at {0}".format(top_dir))
        self._create_output_dir(run_dir)
        self._create_output_dir(tmp_dir)
        self._create_output_dir(xml_dir)
        log_info("SST Test Output Directory = {0}".format(top_dir), forced=False)
        log_debug(" - Test Output Run Directory = {0}".format(run_dir))
        log_debug(" - Test Output Tmp Directory = {0}".format(tmp_dir))
        log_debug(" - Test Output XML Directory = {0}".format(xml_dir))

####

    def _discover_testsuites(self):
        """ Figure out the list of paths we are searching for testsuites.  The
            user may have given us a list via the cmd line, so that takes priority
        """
        # Did the user give us any test suite paths in the cmd line, if not
        # build a list of testsuites from the core config file
        if len(self._list_of_searchable_testsuite_paths) == 0:
            self._list_of_searchable_testsuite_paths = self._build_list_of_testsuite_dirs()

        # Check again to see if no Test Suite Paths
        if len(self._list_of_searchable_testsuite_paths) == 0:
            log_error("No TestSuite dirs/files have been found or defined")

        # Debug dump of search paths
        log_debug("SEARCH LOCATIONS OF TESTSUITES:")
        for search_path in self._list_of_searchable_testsuite_paths:
            log_debug("- {0}".format(search_path))

        self._add_testsuites_from_identifed_paths()

        log_debug("DISCOVERED TESTS (FROM TESTSUITES):")
        self._dump_testsuite_list(self._sst_full_test_suite)

        # Warn the user if no testssuites/testcases are found
        if self._sst_full_test_suite.countTestCases() == 0:
            log_error(("No TestSuites (with TestCases) have been found ") +
                      ("- verify the search paths"))
            log_forced("SEARCH LOCATIONS FOR TESTSUITES:")
            for search_path in self._list_of_searchable_testsuite_paths:
                log_forced("- {0}".format(search_path))

####

    def _add_testsuites_from_identifed_paths(self):
        """ Look at all the searchable testsuite paths in the list.  If its
            a file, try to add that testsuite directly.  If its a directory;
            add all testsuites that match the identifed testsuite types.
        """
        # Discover tests in each Test Path directory and add to the test suite
        # A testsuite_path may be a directory or a file
        for testsuite_path in self._list_of_searchable_testsuite_paths:
            if os.path.isdir(testsuite_path):
                # Find all testsuites that match our pattern(s)
                if 'all' in self._testsuite_types_list:
                    testsuite_pattern = 'testsuite_*.py'
                    sst_testsuites = unittest.TestLoader().discover(start_dir=testsuite_path,
                                                                    pattern=testsuite_pattern)
                    self._sst_full_test_suite.addTests(sst_testsuites)
                else:
                    for testsuite_type in self._testsuite_types_list:
                        testsuite_pattern = 'testsuite_{0}_*.py'.format(testsuite_type)
                        sst_testsuites = unittest.TestLoader().discover(start_dir=testsuite_path,
                                                                        pattern=testsuite_pattern)
                        self._sst_full_test_suite.addTests(sst_testsuites)

            if os.path.isfile(testsuite_path):
                # Add a specific testsuite from a filepath
                testsuite_dir = os.path.abspath(os.path.dirname(testsuite_path))
                basename = os.path.basename(testsuite_path)
                sst_testsuites = unittest.TestLoader().discover(start_dir=testsuite_dir,
                                                                pattern=basename)
                self._sst_full_test_suite.addTests(sst_testsuites)

####

    def _create_core_config_parser(self):
        """ Create an Core Configurtion (INI format) parser.  This will allow
            us to search the Core configuration looking for test file paths
            :return: An ConfParser.RawConfigParser object
        """
        # ID the path to the sst configuration file
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

    def _build_core_config_include_defs_dict(self):
        """ Create a dictionary of settings fromt he sst_config.h.  This will
            allow us to search the includes that the core provides
            :return: A dict object of defines from the
        """
        # ID the path to the sst configuration file
        core_conf_include_dir = self._sst_core_bin_dir + "/../include/sst/core/"
        core_conf_include_path = core_conf_include_dir + "sst_config.h"
        if not os.path.isdir(core_conf_include_dir):
            log_fatal((("SST-Core Directory {0} - Does not exist - ") +
                       ("testing cannot continue")).format(core_conf_include_dir))
        if not os.path.isfile(core_conf_include_path):
            log_fatal((("SST-Core Configuration Include File {0} - Does not exist - ") +
                       ("testing cannot continue")).format(core_conf_include_path))

        # Read in the file line by line and discard any lines
        # that do not start with "#define "
        rtn_dict = {}
        with open(core_conf_include_path, 'r') as f:
            for read_line in f:
                line = read_line.rstrip()
                if "#define " in line[0:8]:
                    value = "undefined"
                    key = "undefined"
                    line = line[8:]
                    #find the first space
                    index = line.find(" ")
                    if index == -1:
                        key = line
                    else:
                        key = line[0:index]
                        value = line[index+1:].strip('"')
                    rtn_dict[key] = value
        return rtn_dict

###

    def _build_list_of_testsuite_dirs(self):
        """ Using a config file parser, build a list of test Suite Dirs.
            Note: The discovery method of Test Suites is different
                  depending if we are testing core vs registered elements.
        """
        final_rtn_paths = []
        testsuite_paths = []

        # Get the Config File Parser
        core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER

        # Now read the appropriate type of data (Core or Elements)
        try:
            if self._test_mode == MODE_TEST_SST_CORE:
                # Find the tests dir in the core
                section = "SSTCore"
                cfg_path_data = core_conf_file_parser.get(section, "testsdir")
                testsuite_paths.append(cfg_path_data)
            elif self._test_mode == MODE_TEST_ELEMENTS:
                # Find the testsuites dir for each registered element
                section = "SST_ELEMENT_TESTS"
                cfg_path_data = core_conf_file_parser.items(section)
                for path_data in cfg_path_data:
                    testsuite_paths.append(path_data[1])
            else:
                log_fatal("Unsupported test_mode {0}".format(self._test_mode))

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

    def _dump_testsuite_list(self, suite):
        """ Recursively log all tests in a TestSuite
            :param: The current suite to print
        """
        if hasattr(suite, '__iter__'):
            for sub_suite in suite:
                self._dump_testsuite_list(sub_suite)
        else:
            log_debug("- {0}".format(suite))
