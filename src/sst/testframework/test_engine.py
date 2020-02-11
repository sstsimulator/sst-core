#!/usr/bin/env python
# -*- coding: utf-8 -*-

#import sys
#import os
import unittest
import argparse

import test_globals
from test_support import *

#################################################

HELP_DESC = 'Run {0} Tests'
HELP_CORE_EPILOG = (("TODO (CLEANUP TEXT): Python files named TestScript*.py found at ") +
                    ("or below the defined test path(s) will be run."))
HELP_ELEM_EPILOG = (("TODO (CLEANUP TEXT): Python files named TestScript*.py found at ") +
                    ("or below the defined test path(s) will be run."))


#################################################

class TestEngine():
    def __init__(self, runCoreTests, parentArgParser,
                       SSTCoreBinDir, SSTCoreSSTAppPath,
                       SSTStartupTopDir):

        # Init some internal variables
        self._failfast = False
        self._parentArgParser = parentArgParser
        self._SSTCoreBinDir = SSTCoreBinDir
        self._SSTCoreSSTAppPath = SSTCoreSSTAppPath
        self._SSTStartupTopDir = SSTStartupTopDir
        self._coreTestMode = runCoreTests
        self._testTypeStr = "SST-Core" if self._coreTestMode else "Elements"
        self._sstElementsTopDir = os.path.dirname(__file__)
        self._sstFullTestSuite = unittest.TestSuite()

        # Initialize the globals & continue parsing the arguments
        test_globals.initTestGlobals()
        self._parseArguments()
        print("NOTICE: Test Engine Instantiated - Running tests on {0}\n".format(self._testTypeStr))

    def _parseArguments(self):
        epi = HELP_CORE_EPILOG if self._coreTestMode else HELP_ELEM_EPILOG
        deflistofpaths = self._SSTStartupTopDir if self._coreTestMode else self._SSTStartupTopDir

        # Build a new parser (from the bones of the orig parser passed in) and
        # populate it with more intresting stuff.
        parser = argparse.ArgumentParser(parents=[self._parentArgParser],
                                         description = HELP_DESC.format(self._testTypeStr),
                                         epilog = epi)
        parser.add_argument('listofpaths', metavar='test_path', nargs='*',
                             default=[deflistofpaths],
                             help='Directories to Test Suites [DEFAULT = .]')
        mutgroup = parser.add_mutually_exclusive_group()
        mutgroup.add_argument('-v', '--verbose', action='store_true',
                               help = 'Run tests in verbose mode')
        mutgroup.add_argument('-q', '--quiet', action='store_true',
                               help = 'Run tests in quiet mode')
        mutgroup.add_argument('-d', '--debug', action='store_true',
                               help='Run tests in test debug output mode')
        parser.add_argument('-r', '--ranks', type=int, metavar="XX",
                            nargs=1, default=0,
                            help='Run with XX ranks')
        parser.add_argument('-t', '--threads', type=int, metavar="YY",
                            nargs=1, default=0,
                            help='Run with YY threads')
        parser.add_argument('-f', '--failfast', action='store_true',
                            help = 'Stop testing on failure')
        parser.add_argument('-o', '--outdir', type=str, metavar='dir',
                            nargs='?', default='./test_outputs',
                            help = 'Set output directory')

        # We want this param to be invisible to the user
        parser.add_argument('-x', '--testappdebug', action='store_true',
                            help = argparse.SUPPRESS)

        args = parser.parse_args()

        # Extract the Arguments into the class variables
        test_globals.verbosity = 1
        if args.debug == True:
            test_globals.verbosity = 3
            test_globals.debugMode = True
        if args.quiet == True:
            test_globals.verbosity = 0
        if args.verbose == True:
            test_globals.verbosity = 2
        test_globals.numRanks = args.ranks
        test_globals.numThreads = args.threads
        test_globals.listOfSearchableTestPaths = args.listofpaths
        self._failfast = args.failfast
        self.topOutputDir = args.outdir
        test_globals.testOutputTopDirPath = args.outdir
        test_globals.__TestAppDebug = args.testappdebug

####

    def _createOutputDirectories(self):
        # Verify that the top Output Directory is valid
        os.system("mkdir -p {0}".format(test_globals.testOutputTopDirPath))
        if not os.path.isdir(test_globals.testOutputTopDirPath):
            print("FATAL:\nOutput Directory {0}\nDoes not exist and cannot be created".format(test_globals.testOutputTopDirPath))
            sys.exit(1)

        # Create the test output dir if necessary
        test_globals.testOutputRunDirPath = "{0}/run_data".format(test_globals.testOutputTopDirPath)
        test_globals.testOutputTmpDirPath = "{0}/tmp_data".format(test_globals.testOutputTopDirPath)
        os.system("mkdir -p {0}".format(test_globals.testOutputRunDirPath))
        os.system("mkdir -p {0}".format(test_globals.testOutputTmpDirPath))
        if test_globals.__TestAppDebug:
            print("\n")
            print("Test Output Run Directory = {0}".format(test_globals.testOutputRunDirPath))
            print("Test Output Tmp Directory = {0}".format(test_globals.testOutputTmpDirPath))


    def _discoverTests(self):
        print("TODO: DISCOVER TESTS")
        # Discover tests in each Test Path directory and add to the test suite
        sstPattern = 'testsuite*.py'
#        for testpath in test_globals.listOfSearchableTestPaths:
#            sstDiscoveredTests = unittest.TestLoader().discover(start_dir=testpath,
#                                                                pattern=sstPattern,
#                                                                top_level_dir=self._sstElementsTopDir)
#            self.sstFullTestSuite.addTests(sstDiscoveredTests)
#
#        if test_globals.__TestAppDebug:
#            print("\n")
#            print("Searchable Test Paths = {0}".format(test_globals.listOfSearchableTestPaths))
#            for test in self.sstFullTestSuite:
#                print("Discovered Tests = {0}".format(test._tests))
#            print("\n")
#

    def runTests(self):
        self._createOutputDirectories()
        self._discoverTests()

        # Run all the tests
        sstTestsResults = unittest.TextTestRunner(verbosity=test_globals.verbosity,
                                                  failfast=self._failfast).run(self._sstFullTestSuite)


