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
import os.path

#################################################

TESTFRAMEWORKSFILES = ["test_globals.py", "test_support.py", "test_engine.py"]

#################################################

def startupAndRun(SSTCoreBinDir, testCoreMode):
    """ Entry point for loading and running the SST Test Frameworks Engine
        :param: SSTCoreBinDir = The SST-Core binary directory
        :param: testCoreMode = True for Core Testing, False for Elements testing
    """
    # From the Current directory (this should be the SST-Core/bin dir)
    # set the path of the testframeworks dir
    SSTCoreTestFrameworksDir = SSTCoreBinDir + "/testframeworks/"

    _verifyTestFrameworkIsAvailable(SSTCoreTestFrameworksDir)

    # Now try to import the test framework components
    sys.path.insert(1, SSTCoreTestFrameworksDir)
    import test_engine

    # Create the Test Engine and run it
    te = test_engine.TestEngine(SSTCoreBinDir, testCoreMode)
    te.runTests()

###

def _verifyTestFrameworkIsAvailable(SSTCoreFrameworksDir):
    """ Entry point for loading and running the SST Test Frameworks
        :param: SSTCoreFrameworksDir = Dir where the test frameworks should be
    """
    # Verify that the frameworks files exist in testframeworks subdirectory
    frameworksFound = True
    for filename in TESTFRAMEWORKSFILES:
        filesearchpath =  SSTCoreFrameworksDir + filename
        if not os.path.isfile(filesearchpath):
            print (("WARNING: Missing test framework file " +
                    "{0}".format(filesearchpath)))
            frameworksFound = False

    if not frameworksFound:
        print (("ERROR: Testing cannot be performed due to missing ") +
               ("frameworks files...\n"))
        sys.exit(1)

