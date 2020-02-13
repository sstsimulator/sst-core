#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os.path

#################################################

TESTFRAMEWORKSFILES = ["test_globals.py", "test_support.py", "test_engine.py"]

#################################################

def testEntry(SSTCoreBinDir, testCoreMode):
    # From the Current directory (this should be the SST-Core/bin dir)
    # set the path of the testframeworks dir
    SSTCoreTestFrameworksDir = SSTCoreBinDir + "/testframeworks/"

    _verifyTestFrameworkIsAvailable(SSTCoreTestFrameworksDir)

    # Now try to import the test framework components
    sys.path.insert(1, SSTCoreTestFrameworksDir)
    import test_engine

    # Create the Test Engine and run it
    te = test_engine.TestEngine(testCoreMode,
                                SSTCoreBinDir,
                                SSTCoreTestFrameworksDir)
    te.runTests()

###

def _verifyTestFrameworkIsAvailable(SSTCoreFrameworksDir):
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

