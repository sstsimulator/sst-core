#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os
import ConfigParser
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
        logNotice(("Test Engine Instantiated - Running") +
                  (" tests on {0}\n").format(self._testTypeStr))

####

    def runTests(self):
        self._createOutputDirectories()
        self._discoverTests()

        # Run all the tests
        logForced(("\n=== TESTS STARTING ================") +
                  ("===================================\n"))
        sstTestsResults = unittest.TextTestRunner(verbosity=test_globals.verbosity,
                                                  failfast=self._failfast). \
                                                  run(self._sstFullTestSuite)
####
    def _parseArguments(self):

        # Build a new parser (from the bones of the orig parser passed in) and
        # populate it with more intresting stuff.
        epilog = HELP_CORE_EPILOG if self._coreTestMode else HELP_ELEM_EPILOG
        helpdesc = HELP_DESC.format(self._testTypeStr)
        parser = argparse.ArgumentParser(parents=[self._parentArgParser],
                                         description = helpdesc,
                                         epilog = epilog)

        testPathStr = "Dir to SST-Core Testscripts" if \
                      self._coreTestMode else \
                      "Dir to Registered Elements TestScripts"
        parser.add_argument('listofpaths', metavar='test_path', nargs='*',
                             default=[],
                             help=testPathStr)
        mutgroup = parser.add_mutually_exclusive_group()
        mutgroup.add_argument('-v', '--verbose', action='store_true',
                               help = 'Run tests in verbose mode')
        mutgroup.add_argument('-q', '--quiet', action='store_true',
                               help = 'Run tests in quiet mode')
        mutgroup.add_argument('-d', '--debug', action='store_true',
                               help='Run tests in test engine debug mode')
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

        args = parser.parse_args()
        self._decodeParsedArguments(args)

    def _decodeParsedArguments(self, args):
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

####

    def _createOutputDirectories(self):
        # Verify that the top Output Directory is valid
        os.system("mkdir -p {0}".format(test_globals.testOutputTopDirPath))
        if not os.path.isdir(test_globals.testOutputTopDirPath):
            logFatal((("Output Directory {0} - Does not exist and cannot ") +
                      ("be created")).format(test_globals.testOutputTopDirPath))

        # Create the test output dir if necessary
        test_globals.testOutputRunDirPath = "{0}/run_data".format(test_globals.testOutputTopDirPath)
        test_globals.testOutputTmpDirPath = "{0}/tmp_data".format(test_globals.testOutputTopDirPath)
        os.system("mkdir -p {0}".format(test_globals.testOutputRunDirPath))
        os.system("mkdir -p {0}".format(test_globals.testOutputTmpDirPath))
        logDebug("Test Output Run Directory = {0}".format(test_globals.testOutputRunDirPath))
        logDebug("Test Output Tmp Directory = {0}".format(test_globals.testOutputTmpDirPath))

####

    def _discoverTests(self):
        # Get the list of paths we are searching for testscripts
        if len(test_globals.listOfSearchableTestPaths) == 0:
            test_globals.listOfSearchableTestPaths = self._buildListOfTestSuiteDirs()

        logDebug("SEARCH PATHS FOR TESTSCRIPTS:")
        for searchPath in test_globals.listOfSearchableTestPaths:
            logDebug("- {0}".format(searchPath))

        # Discover tests in each Test Path directory and add to the test suite
        sstPattern = 'testsuite*.py'
        for testpath in test_globals.listOfSearchableTestPaths:
            sstDiscoveredTests = unittest.TestLoader().discover(start_dir=testpath,
                                                                pattern=sstPattern)
            self._sstFullTestSuite.addTests(sstDiscoveredTests)

        logDebug("DISCOVERED TESTSUITES:")
        for test in self._sstFullTestSuite:
            logDebug(" - {0}".format(test._tests))

####

    def _buildListOfTestSuiteDirs(self):
        finalRtnPaths = []
        testSuitePaths = []

        # Find the Installed SST-Core Configuration file
        # Note: This file is configured by installing SST-Core and also
        #       installing (and registering) various Elements
        coreConfFileDir = self._SSTCoreBinDir + "/../etc/sst/"
        coreConfFilePath = coreConfFileDir + "sstsimulator.conf"
        if not os.path.isdir(coreConfFileDir):
            logFatal((("SST-Core Directory {0} - Does not exist - ") +
                      ("testing cannot continue")).format(coreConfFileDir))
        if not os.path.isfile(coreConfFilePath):
            logFatal((("SST-Core Configuration File {0} - Does not exist - ") +
                      ("testing cannot continue")).format(coreConfFilePath))

        # Instantiate a ConfigParser to read the data in the config file
        try:
            coreConfFileParser = ConfigParser.RawConfigParser()
            coreConfFileParser.read(coreConfFilePath)
        except ConfigParser.Error as e:
            logFatal((("Test Engine: Cannot read SST-Core Configuration ") +
                      ("File:\n{0}\n") +
                      ("- testing cannot continue ({1})")).format(coreConfFilePath, e))

        # Now read the appropriate type of data (Core or Elements)
        try:
            if self._coreTestMode:
                # Find the testsuites dir in the core
                cfgPathData = coreConfFileParser.get("SSTCore", "testsuitesdir")
                testSuitePaths.append(cfgPathData)
            else:
                # Find the testsuites dir for each registered element
                cfgPathData = coreConfFileParser.items("SST_ELEMENT_TESTSUITES")
                for pathData in cfgPathData:
                    testSuitePaths.append(pathData[1])

        except ConfigParser.Error as e:
            testSuitePaths.append(self._SSTStartupTopDir)
            errmsg = "Reading SST-Core Config file {0} - {1} ".format(coreConfFilePath, e)
            logError(errmsg)

        # Now verify each path is valid
        for suitePath in testSuitePaths:
            if not os.path.isdir(suitePath):
                logWarning((("TestSuite Directory {0} - Does not exist\n - ") +
                            ("No tests will be performed...")).format(suitePath))
            else:
                finalRtnPaths.append(suitePath)

        return finalRtnPaths

