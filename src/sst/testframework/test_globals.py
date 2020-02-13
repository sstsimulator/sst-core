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

import os

""" This is some globals to pass data between the top level test engine
    and the lower level testscripts """
def initTestGlobals():
    global debugMode
    global verbosity
    global numRanks
    global numThreads
    global listOfSearchableTestSuitePaths
    global testSuiteFilePath
    global testOutputTopDirPath
    global testOutputRunDirPath
    global testOutputTmpDirPath

    debugMode = False
    verbosity = 1
    numRanks = 0
    numThreads = 0
    listOfSearchableTestPaths = [os.path.dirname(__file__)]
    testSuiteFilePath = ""
    testOutputTopDirPath = "./test_outputs"
    testOutputRunDirPath = "{0}/run_data".format(testOutputTopDirPath)
    testOutputTmpDirPath = "{0}/tmp_data".format(testOutputTopDirPath)
