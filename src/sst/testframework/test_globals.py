#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

""" This is some globals to pass data between the top level test engine
    and the lower level testscripts """
def initTestGlobals():
    global __TestAppDebug  # Only used to debug the test application
    global debugMode
    global verbosity
    global numRanks
    global numThreads
    global listOfSearchableTestPaths
    global testSuiteFilePath
    global testOutputTopDirPath
    global testOutputRunDirPath
    global testOutputTmpDirPath

    __TestAppDebug = False
    debugMode = False
    verbosity = 1
    numRanks = 0
    numThreads = 0
    listOfSearchableTestPaths = [os.path.dirname(__file__)]
    testSuiteFilePath = ""
    testOutputTopDirPath = "./test_outputs"
    testOutputRunDirPath = "{0}/run_data".format(testOutputTopDirPath)
    testOutputTmpDirPath = "{0}/tmp_data".format(testOutputTopDirPath)
