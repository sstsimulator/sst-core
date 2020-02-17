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

""" This module verify that the test frameworks are installed and then load
    the test_engine module.  Once loaded, tests will be discovered and run
"""

import sys
import os.path

#################################################

TESTFRAMEWORKSFILES = ["test_globals.py", "test_support.py", "test_engine.py"]

#################################################

def startup_and_run(sst_core_bin_dir, test_core_mode):
    """ Entry point for loading and running the SST Test Frameworks Engine
        :param: sst_core_bin_dir = The SST-Core binary directory
        :param: test_core_mode = True for Core Testing, False for Elements testing
    """
    # From the Current directory (this should be the SST-Core/bin dir)
    # set the path of the testframeworks dir
    sst_core_test_frameworks_dir = sst_core_bin_dir + "/../libexec/"

    _verify_test_framework_is_available(sst_core_test_frameworks_dir)

    # Now try to import the test framework components
    sys.path.insert(1, sst_core_test_frameworks_dir)
    import test_engine

    # Create the Test Engine and run it
    t_e = test_engine.TestEngine(sst_core_bin_dir, test_core_mode)
    t_e.discover_and_run_tests()

###

def _verify_test_framework_is_available(sst_core_frameworks_dir):
    """ Entry point for loading and running the SST Test Frameworks
        :param: sst_core_frameworks_dir = Dir where the test frameworks should be
                found
    """
    # Verify that the frameworks files exist in testframeworks subdirectory
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
