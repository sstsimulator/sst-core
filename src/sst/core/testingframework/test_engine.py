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
import shutil
import time
import traceback
from datetime import datetime
from test_engine_support import strclass
from test_engine_support import strqual
from test_engine_junit import JUnitTestCase

# ConfigParser module changes name between Py2->Py3
try:
    import configparser
except ImportError:
    import ConfigParser as configparser

import test_engine_globals
from sst_unittest_support import *

################################################################################

REQUIRED_PY_MAJ_VER_2 = 2 # Required Major Version Min
REQUIRED_PY_MAJ_VER_MAX = 3 # Required Major Version Max
REQUIRED_PY_MAJ_VER_2_MINOR_VER = 7 # Required Minor Version

HELP_DESC = 'Run {0} Tests'
HELP_EPILOG = (("The 'testsuite_paths' argument can be defined as either as  ") +
               ("directories or specific testsuite files.  If the ") +
               ("'testsuite_paths' argument is empty, testsuites paths will be ") +
               ("populated from the sstsimulator.conf ") +
               ("file located in the <sstcore_install>/etc dir.  ") +
               ("During operation, the Test Frameworks will create a list of ") +
               ("runable testsuites.  Files specifed in the 'testsuite_paths' ") +
               ("argument will be directly added.  Directories specifed in the ") +
               ("'testsuite_paths' argument will be searched ") +
               ("and testsuites will be added as follows: ") +
               ("Files named testsuite_<scenarioname>_*.py found in a ") +
               ("searched directory(s) will be added.  Scenario Names can be set ") +
               ("using the -s (--scenario) argument.  By default ") +
               ("the scenarioname will be = 'default'; and all testsuite_default_*.py ") +
               ("files will be found.  If <scenarioname> = 'all', then all ") +
               ("testsuite_*.py files will be found.")
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
        log_info(("SST Test Engine Instantiated - Running") +
                 (" tests on {0}").format(self._test_type_str), forced=False)

        log_info(("Test Platform = {0}".format(get_host_os_distribution_type())) +
                 (" {0}".format(get_host_os_distribution_version())), forced=False)

        if 'all' in self._list_of_scenario_names:
            log_info("Test Scenario(s) to be run are: ALL TEST SCENARIOS", forced=False)
        else:
            log_info(("Test Scenario(s) to be run are: ") +
                     ("{0}").format(" ".join(self._list_of_scenario_names)), forced=False)
        log_debug("Python Version = {0}.{1}.{2}".format(ver[0], ver[1], ver[2]))

####

    def discover_and_run_tests(self):
        """ Create the output directories, then discover the tests, and then
            run them using pythons unittest module
        """
        self._create_all_output_directories()
        # Build the Config File Parser
        test_engine_globals.CORECONFFILEPARSER = self._create_core_config_parser()
        test_engine_globals.CORECONFINCLUDEFILEDICT = self._build_core_config_include_defs_dict()

        self._discover_testsuites()
        try:
            test_runner = SSTTextTestRunner(verbosity=test_engine_globals.VERBOSITY,
                                            failfast=self._fail_fast,
                                            resultclass=SSTTextTestResult)
            sst_tests_results = test_runner.run(self._sst_full_test_suite)

            if not test_runner.did_tests_pass(sst_tests_results):
                return 1
            return 0

        # Handlers of unittest.TestRunner exceptions
        except KeyboardInterrupt:
            log_fatal("TESTING TERMINATED DUE TO KEYBOARD INTERRUPT...")

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
        self._list_of_scenario_names = []
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
        run_group.add_argument('-r', '--ranks', type=int, metavar="XX",
                               nargs=1, default=[1],
                               help='Run with XX ranks [1]')
        run_group.add_argument('-t', '--threads', type=int, metavar="YY",
                               nargs=1, default=[1],
                               help='Run with YY threads [1]')
        run_group.add_argument('-a', '--sst_run_args', type=str, metavar='" --arg1 -a2"',
                               nargs=1, default=[''],
                               help=('Runtime args for all SST runs (must be ')
                                  + ('identifed as a string; Note:extra space at front)'))
        parser.add_argument('-f', '--fail_fast', action='store_true',
                            help='Stop testing on failure [true]')
        parser.add_argument('-k', '--keep_output', action='store_true',
                            help='Dont clean output directory at start [false]')
        parser.add_argument('-o', '--out_dir', type=str, metavar='dir',
                            nargs=1, default=['./sst_test_outputs'],
                            help='Set output directory [./sst_test_outputs]')
        discover_group = parser.add_argument_group('Test Discovery Arguments')
        discover_group.add_argument('-s', '--scenarios', type=str, metavar="name",
                                    nargs="+", default=['default'],
                                    help=(('Name(s) (in lowercase) of testing scenario(s)') + \
                                         (' ("all" will run all scenarios) ["default"]')))
        if self._test_mode:
            testsuite_path_str = "TestSuite Files or Dirs to SST-Core TestSuites"
        else:
            testsuite_path_str = "Testsuite Files or Dirs to Registered Elements TestSuites"
        discover_group.add_argument('-p', '--list_of_paths', metavar='path',
                                    nargs='*', default=[], help=testsuite_path_str)

        args = parser.parse_args()
        self._decode_parsed_arguments(args)

####

    def _decode_parsed_arguments(self, args):
        """ Decode the parsed arguments into their class or global variables
            :param: args = The arguments from the cmd line parser
        """
        # Extract the Arguments into the class variables
        self._fail_fast = args.fail_fast
        self._keep_output_dir = args.keep_output
        self._list_of_searchable_testsuite_paths = args.list_of_paths
        lc_scen_list = [item.lower() for item in args.scenarios]
        self._list_of_scenario_names = lc_scen_list
        test_engine_globals.VERBOSITY = test_engine_globals.VERBOSE_NORMAL
        if args.quiet:
            test_engine_globals.VERBOSITY = test_engine_globals.VERBOSE_QUIET
        if args.verbose:
            test_engine_globals.VERBOSITY = test_engine_globals.VERBOSE_LOUD
        if args.debug:
            test_engine_globals.DEBUGMODE = True
            test_engine_globals.VERBOSITY = test_engine_globals.VERBOSE_DEBUG
        test_engine_globals.SSTRUNNUMRANKS = args.ranks[0]
        test_engine_globals.SSTRUNNUMTHREADS = args.threads[0]
        test_engine_globals.SSTRUNGLOBALARGS = args.sst_run_args[0]
        test_engine_globals.TESTOUTPUTTOPDIRPATH = args.out_dir[0]
        test_engine_globals.TESTOUTPUTRUNDIRPATH = "{0}/run_data".format(args.out_dir[0])
        test_engine_globals.TESTOUTPUTTMPDIRPATH = "{0}/tmp_data".format(args.out_dir[0])
        test_engine_globals.TESTOUTPUTXMLDIRPATH = "{0}/xml_data".format(args.out_dir[0])
        if args.ranks[0] < 0:
            log_fatal("ranks must be >= 0; currently set to {0}".format(args.ranks[0]))
        if args.threads[0] < 0:
            log_fatal("threads must be >= 0; currently set to {0}".format(args.threads[0]))

####

    def _create_all_output_directories(self):
        """ Create the output directories if needed
        """
        top_dir = test_engine_globals.TESTOUTPUTTOPDIRPATH
        run_dir = test_engine_globals.TESTOUTPUTRUNDIRPATH
        tmp_dir = test_engine_globals.TESTOUTPUTTMPDIRPATH
        xml_dir = test_engine_globals.TESTOUTPUTXMLDIRPATH
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
            add all testsuites that match the identifed scenarios.
        """
        # Discover tests in each Test Path directory and add to the test suite
        # A testsuite_path may be a directory or a file
        for testsuite_path in self._list_of_searchable_testsuite_paths:
            if os.path.isdir(testsuite_path):
                # Find all testsuites that match our pattern(s)
                if 'all' in self._list_of_scenario_names:
                    testsuite_pattern = 'testsuite_*.py'
                    sst_testsuites = unittest.TestLoader().discover(start_dir=testsuite_path,
                                                                    pattern=testsuite_pattern)
                    self._sst_full_test_suite.addTests(sst_testsuites)
                else:
                    for scenario_name in self._list_of_scenario_names:
                        testsuite_pattern = 'testsuite_{0}_*.py'.format(scenario_name)
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
        core_conf_file_parser = test_engine_globals.CORECONFFILEPARSER

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

################################################################################
################################################################################
################################################################################

class SSTTextTestRunner(unittest.TextTestRunner):
    """ A superclass to support SST required testing """

    def __init__(self, stream=sys.stderr, descriptions=True, verbosity=1,
                 failfast=False, buffer=False, resultclass=None):
        super(SSTTextTestRunner, self).__init__(stream, descriptions, verbosity,
                                                failfast, buffer, resultclass)
        log(("\n=== TESTS STARTING ================") +
            ("===================================\n"))

###

    def run(self, test):
        runresults = super(SSTTextTestRunner, self).run(test)
        self._get_and_display_test_results(runresults)
        return runresults

###

    def did_tests_pass(self, run_results):
        """ Figure out if testing passed
            :param: run_results -  A unittest.TestResult object
            :return: True if all tests passing with no errors, false otherwise
        """
        return run_results.wasSuccessful and \
        len(run_results.errors) == 0 and \
        test_engine_globals.ERRORCOUNT == 0

###

    def _get_and_display_test_results(self, run_results):
        """ Figure out if testing passed, and display the test results
            :param: sst_tests_results -  A unittest.TestResult object
            :return: True if all tests passing with no errors, false otherwise
        """
        log(("\n=== TEST RESULTS ==================") +
                   ("===================================\n"))
        log("Tests Run      = {0}".format(run_results.testsRun))
        log("Tests Failures = {0}".format(len(run_results.failures)))
        log("Tests Skipped  = {0}".format(len(run_results.skipped)))
        log("Tests Errors   = {0}".format(len(run_results.errors)))
        if self.did_tests_pass(run_results):
            log_forced("\n== TESTING PASSED ==")
        else:
            if test_engine_globals.ERRORCOUNT == 0:
                log_forced("\n== TESTING FAILED ==")
            else:
                log_forced("\n== TESTING FAILED DUE TO ERRORS ==")
        log(("\n===================================") +
            ("===================================\n"))

################################################################################

class SSTTextTestResult(unittest.TextTestResult):
    """ A superclass to support SST required testing """

    def __init__(self, stream, descriptions, verbosity):
        super(SSTTextTestResult, self).__init__(stream, descriptions, verbosity)

###

    def startTest(self, test):
        super(SSTTextTestResult, self).startTest(test)
        #log_forced("DEBUG - startTest: Test = {0}\n".format(test))
        self._start_time = time.time()
        self._test_name = test.testName
        self._testcase_name = strqual(test.__class__)
        self._testsuite_name = strclass(test.__class__)
        timestamp = datetime.utcnow().strftime("%Y_%m%d_%H:%M:%S.%f utc")
        self._junit_test_case = JUnitTestCase(self._test_name,
                                              self._testcase_name,
                                              timestamp=timestamp)

    def stopTest(self, test):
        super(SSTTextTestResult, self).stopTest(test)
        #log_forced("DEBUG - stopTest: Test = {0}\n".format(test))
        self._junit_test_case.junit_add_elapsed_sec(time.time() - self._start_time)
        test_engine_globals.JUNITTESTCASELIST.append(self._junit_test_case)

###

    def addSuccess(self, test):
        #super(SSTTextTestResult, self).addSuccess(test)
        #log_forced("DEBUG - addSuccess: Test = {0}\n".format(test))
        # Override the "ok" and make it a "PASS" instead
        if self.showAll:
            self.stream.writeln("PASS")
        elif self.dots:
            self.stream.write('.')
            self.stream.flush()

    def addError(self, test, err):
        super(SSTTextTestResult, self).addError(test, err)
        #log_forced("DEBUG - addError: Test = {0}, err = {1}\n".format(test, err))
        _junit_test_case = getattr(self, '_junit_test_case', None)
        if _junit_test_case is not None:
            err_msg = self._get_err_info(err)
            _junit_test_case.junit_add_error_info(err_msg)

    def addFailure(self, test, err):
        super(SSTTextTestResult, self).addFailure(test, err)
        #log_forced("DEBUG - addFailure: Test = {0}, err = {1}\n".format(test, err))
        _junit_test_case = getattr(self, '_junit_test_case', None)
        if _junit_test_case is not None:
            err_msg = self._get_err_info(err)
            _junit_test_case.junit_add_failure_info(err_msg)

    def addSkip(self, test, reason):
        super(SSTTextTestResult, self).addSkip(test, reason)
        #log_forced("DEBUG - addSkip: Test = {0}, reason = {1}\n".format(test, reason))
        _junit_test_case = getattr(self, '_junit_test_case', None)
        if _junit_test_case is not None:
            _junit_test_case.junit_add_skipped_info(reason)

    def addExpectedFailure(self, test, err):
        super(SSTTextTestResult, self).addExpectedFailure(test, err)
        #log_forced("DEBUG - addExpectedFailure: Test = {0}, err = {1}\n".format(test, err))

    def addUnexpectedSuccess(self, test):
        super(SSTTextTestResult, self).addUnexpectedSuccess(test)
        #log_forced("DEBUG - addUnexpectedSuccess: Test = {0}\n".format(test))

###

    def printErrors(self):
        if self.dots or self.showAll:
            self.stream.writeln()
        log(("===================================") +
            ("==================================="))
        log(("=== TESTS FINISHED ================") +
            ("==================================="))
        log(("===================================") +
            ("===================================\n"))
        self.printErrorList('ERROR', self.errors)
        self.printErrorList('FAIL', self.failures)

####

    def _get_err_info(self, err):
        """Converts a sys.exc_info() into a string."""
        exctype, value, tb = err
        msg_lines = traceback.format_exception_only(exctype, value)
        msg_lines = [x.replace('\n', ' ') for x in msg_lines]
        return ''.join(msg_lines)
