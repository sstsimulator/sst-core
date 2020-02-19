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

""" This module is the primary loader for the SST Frameworks Test Engine
    it will verify that the test frameworks files are installed.  Then will
    load the test_engine module.  Once loaded, tests will be discovered and run.
"""

import sys
import os.path
import traceback

################################################################################

TESTFRAMEWORKSFILES = ["sst_unittest_support.py", "test_engine.py",
                       "test_engine_globals.py", "test_engine_support.py"]

# AVAILABLE TEST MODES
MODE_TEST_SST_CORE = 0
MODE_TEST_REGISTRED_ELEMENTS = 1

################################################################################

def startup_and_run(sst_core_bin_dir, test_core_mode):
    """ Entry point for loading and running the SST Test Frameworks Engine.
        This will first verify that the frameworks files are available, and
        then load the test_engine.  Then it will tell the test_engine to discover
        and run the tests.
        :param: sst_core_bin_dir = The SST-Core binary directory
        :param: test_core_mode = True for Core Testing, False for Elements testing
    """
    try:
        sst_core_test_frameworks_dir = sst_core_bin_dir + "/../libexec/"
        _verify_test_framework_is_available(sst_core_test_frameworks_dir)

        sys.path.insert(1, sst_core_test_frameworks_dir)
        try:
            import test_engine
        except ImportError as exc_e:
            print("FATAL: Failed to load test_engine.py ({0})".format(exc_e))
            sys.exit(1)

        te = test_engine.TestEngine(sst_core_bin_dir, test_core_mode)
        te.discover_and_run_tests()

    except Exception as exc_e:
        # NOTE: This is a generic catchall handler for any unhandled exception
        print(("FATAL: SST Test Frameworks encountered ") +
              ("an unexpected exception ({0}))".format(exc_e)))
        print("\n=============================================================")
        print("==== TRACEBACK ===============================================")
        print("==============================================================")
        traceback.print_exc(file=sys.stdout)
        print("==============================================================")
        sys.exit(1)

###############

def _verify_test_framework_is_available(sst_core_frameworks_dir):
    """ Ensure that all test framework files are available.
        :param: sst_core_frameworks_dir = Dir of the test frameworks
    """
    frameworks_found = True
    for filename in TESTFRAMEWORKSFILES:
        filesearchpath = sst_core_frameworks_dir + filename
        if not os.path.isfile(filesearchpath):
            print(("WARNING: Missing test framework file " +
                   "{0}".format(filesearchpath)))
            frameworks_found = False

    if not frameworks_found:
        print((("ERROR: Testing cannot be performed due to missing ") +
               ("frameworks files...\n")))
        sys.exit(1)
