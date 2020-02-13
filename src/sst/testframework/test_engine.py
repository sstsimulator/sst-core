#!/usr/bin/env python
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

import sys
import os
import ConfigParser
import unittest
import argparse

import test_globals
from test_support import *

#################################################

HELP_DESC = 'Run {0} Tests'
HELP_EPILOG = (("Python files named TestSuite*.py found at ") +
               ("or below the defined testsuite_path(s) will be run."))


#################################################

class TestEngine():

    def __init__(self, SSTCoreBinDir, runCoreTests):
        """ Initialize the TestEngine object, and parse the user arguments
            :param: SSTCoreBinDir = The SST-Core binary directory
            :param: runCoreTests = True for Core Tests, False for Elements tests
        """
        # Perform initial checks that all is well
        validatePythonVersion()
        self._initTestEngineVariables(SSTCoreBinDir, runCoreTests)
        self._parseArguments()
        logNotice(("SST Test Engine Instantiated - Running") +
                  (" tests on {0}").format(self._testTypeStr))

####

    def runTests(self):
        """ Create the output directories, then discover the tests, and then
            run them using pythons unittest module
        """
        self._createAllOutputDirectories()
        self._discoverTests()

        # Run all the tests
        logForced(("\n=== TESTS STARTING ================") +
                  ("===================================\n"))
        sstTestsResults = unittest.TextTestRunner(verbosity=test_globals.verbosity,
                                                  failfast=self._failfast). \
                                                  run(self._sstFullTestSuite)

#################################################

    def _initTestEngineVariables(self, SSTCoreBinDir, runCoreTests):
        """ Initialize the variables needed for testing.  This will also
            initialize the global variables
            :param: SSTCoreBinDir = The SST-Core binary directory
            :param: runCoreTests = True for Core Tests, False for Elements Tests
        """
        # Init some internal variables
        self._failfast         = False
        self._SSTCoreBinDir    = SSTCoreBinDir
        self._coreTestMode     = runCoreTests
        self._sstFullTestSuite = unittest.TestSuite()
        if self._coreTestMode:
            self._testTypeStr = "SST-Core"
        else:
            self._testTypeStr = "Registered Elements"

        test_globals.initTestGlobals()

####

    def _parseArguments(self):
        """ Parse the cmd line arguments
        """
        # Build a parameter parser, adjust its help based upon the test type
        helpdesc = HELP_DESC.format(self._testTypeStr)
        parser = argparse.ArgumentParser(description = helpdesc,
                                         epilog = HELP_EPILOG)
        if self._coreTestMode:
            testSuitePathStr = "Dir(s) to SST-Core TestSuites"
        else:
            testSuitePathStr = "Dir(s) to Registered Elements TestSuites"
        parser.add_argument('listofpaths', metavar='testsuite_paths', nargs='*',
                             default=[], help=testSuitePathStr)
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

####

    def _decodeParsedArguments(self, args):
        """ Decode the parsed arguments into their class or global variables
            :param: args = The arguments from the cmd line parser
        """
        # Extract the Arguments into the class variables
        self._failfast = args.failfast
        test_globals.verbosity = 1
        if args.quiet == True:
            test_globals.verbosity = 0
        if args.verbose == True:
            test_globals.verbosity = 2
        if args.debug == True:
            test_globals.debugMode = True
            test_globals.verbosity = 3
        test_globals.numRanks = args.ranks
        test_globals.numThreads = args.threads
        test_globals.listOfSearchableTestSuitePaths = args.listofpaths
        test_globals.testOutputTopDirPath = args.outdir
        test_globals.testOutputRunDirPath = "{0}/run_data".format(args.outdir)
        test_globals.testOutputTmpDirPath = "{0}/tmp_data".format(args.outdir)

####

    def _createOutputDir(self, outDir):
        """ Look to see if an output dir exists.  If not, try to create it
            :param: outdir = The path to the output directory
            :return: True if output dir is created (did not exist); else False
        """
        # Is there an directory already existing
        if not os.path.isdir(outDir):
            try:
                os.makedirs(outDir, mode=0o744)
                return True
            except OSError as e:
                logError("os.mkdirs Exception - ({0})".format(e))
                pass
            if not os.path.isdir(outDir):
                logFatal((("Output Directory {0} - Does not exist and ") +
                          ("cannot be created")).format(outDir))
        return False

####

    def _createAllOutputDirectories(self):
        """ Create the output directories if needed
        """
        topDir = test_globals.testOutputTopDirPath
        runDir = test_globals.testOutputRunDirPath
        tmpDir = test_globals.testOutputTmpDirPath
        if self._createOutputDir(topDir):
            logNotice("SST Test Output Dir Created at {0}".format(topDir))
        self._createOutputDir(runDir)
        self._createOutputDir(tmpDir)

        # Create the test output dir if necessary
        logDebug("Test Output Top Directory = {0}".format(topDir))
        logDebug("Test Output Run Directory = {0}".format(runDir))
        logDebug("Test Output Tmp Directory = {0}".format(tmpDir))


####

    def _createCoreConfigParser(self):
        """ Create an Core Configurtion (INI format) parser.  This will allow
            us to search the Core configuration looking for test file paths
            :return: An ConfParser.RawConfigParser object
        """
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
                      ("File:\n{0}\n- testing cannot continue") +
                      ("({1})")).format(coreConfFilePath, e))

        return coreConfFileParser

###

    def _buildListOfTestSuiteDirs(self):
        """ Using a config file parser, build a list of test Suite Dirs.
            Note: The discovery method of Test Suites is different
                  depending if we are testing core vs registered elements.
        """
        finalRtnPaths = []
        testSuitePaths = []

        # Build the Config File Parser
        coreConfFileParser = self._createCoreConfigParser()

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
            errmsg = (("Reading SST-Core Config file ") +
                     ("{0} - {1} ")).format(coreConfFilePath, e)
            logError(errmsg)

        # Now verify each path is valid
        for suitePath in testSuitePaths:
            if not os.path.isdir(suitePath):
                logWarning((("TestSuite Directory {0} - Does not exist\n - ") +
                            ("No tests will be performed...")).format(suitePath))
            else:
                finalRtnPaths.append(suitePath)

        return finalRtnPaths

####

    def _discoverTests(self):
        """ Figure out the list of paths we are searching for testsuites.  The
            user may have given us a list via the cmd line, so that takes priority
        """
        # Did the user give us any test suite paths in the cmd line
        if len(test_globals.listOfSearchableTestSuitePaths) == 0:
            test_globals.listOfSearchableTestSuitePaths = self._buildListOfTestSuiteDirs()

        # Check to see if no Test Suite Paths
        if len(test_globals.listOfSearchableTestSuitePaths) == 0:
            logWarning("No TestSuites Paths have been defined/found")

        # Debug dump of search paths
        logDebug("SEARCH PATHS FOR TESTSUITES:")
        for searchPath in test_globals.listOfSearchableTestSuitePaths:
            logDebug("- {0}".format(searchPath))

        # Discover tests in each Test Path directory and add to the test suite
        sstPattern = 'testsuite*.py'
        for testSuitePath in test_globals.listOfSearchableTestSuitePaths:
            sstDiscoveredTests = unittest.TestLoader().discover(start_dir=testSuitePath,
                                                                pattern=sstPattern)
            self._sstFullTestSuite.addTests(sstDiscoveredTests)

        # Debug dump of discovered testsuites
        logDebug("DISCOVERED TESTSUITES:")
        for test in self._sstFullTestSuite:
            logDebug(" - {0}".format(test._tests))

        # Warn the user if no testssuites/testcases are found
        if self._sstFullTestSuite.countTestCases() == 0:
            logWarning(("No TestSuites (with TestCases) have been found ") +
                       ("- verify the search paths"))
            logForced("SEARCH PATHS FOR TESTSUITES:")
            for searchPath in test_globals.listOfSearchableTestSuitePaths:
                logForced("- {0}".format(searchPath))
