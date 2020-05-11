# -*- coding: utf-8 -*-

## Copyright 2009-2020 NTESS. Under the terms
## of Contract DE-NA0003525 with NTESS, the U.S.
## Government retains certain rights in this software.
##
## Copyright (c) 2009-2020, NTESS
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
import logging
from logging.handlers import RotatingFileHandler

################################################################################

TESTFRAMEWORKSFILES = ["sst_unittest.py", "sst_unittest_support.py", "test_engine.py",
                       "test_engine_globals.py", "test_engine_support.py", 
                       "test_engine_junit.py", "test_engine_unittest.py"]

# AVAILABLE TEST MODES
TEST_ELEMENTS = 0
TEST_SST_CORE = 1

################################################################################

def startup_and_run(sst_core_bin_dir, test_mode):
    """ Entry point for loading and running the SST Test Frameworks Engine.
        This will first verify that the frameworks files are available, and
        then load the test_engine.  Then it will tell the test_engine to discover
        and run the tests.
        :param: sst_core_bin_dir = The SST-Core binary directory
        :param: test_mode = 1 for Core Testing, 0 for Elements testing
    """
    try:
        if test_mode not in (TEST_SST_CORE, TEST_ELEMENTS):
            print((("FATAL: Unsupported test_mode {0} in ") +
                   ("startup_and_run()")).format(test_mode))
            sys.exit(1)

        # Locate the test frameworks dir and verify files are available,
        # then import the test engine
        sst_core_test_frameworks_dir = sst_core_bin_dir + "/../libexec/"
        _verify_test_frameworks_is_available(sst_core_test_frameworks_dir)

        sys.path.insert(1, sst_core_test_frameworks_dir)
        try:
            import test_engine
        except ImportError as exc_e:
            print("FATAL: Failed to load test_engine.py ({0})".format(exc_e))
            sys.exit(1)

        t_e = test_engine.TestEngine(sst_core_bin_dir, test_mode)
        t_e.discover_and_run_tests()

    except Exception as exc_e:
        # NOTE: This is a generic catchall handler for any unhandled exception
        _generic_exception_handler(exc_e)

###############

def _generic_exception_handler(exc_e):

    # Dump Exception info to the a log file
    log_filename = "./sst_test_framesworks_crashreport.log"
    logfile_available = True
    try:
        crashlogger = logging.getLogger("SST_TEST_FRAMEWORKS_CRASHLOGGER")
        log_formatter = logging.Formatter("%(asctime)s: %(message)s", "%Y/%m/%d %H:%M:%S")
        log_handler = RotatingFileHandler(log_filename, mode='a',
                                          maxBytes=10*1024*1024,
                                          backupCount=10,
                                          encoding=None, delay=0)
        log_handler.setFormatter(log_formatter)
        crashlogger.addHandler(log_handler)
        crashlogger.setLevel(logging.DEBUG)
    except IOError:
        logfile_available = False

    # Get the crash trace into a string
    err_str = (("FATAL: SST Test Frameworks encountered an ") +
               ("unexpected exception ({0}))".format(exc_e)))
    trc_str = ("\n{0}".format(traceback.format_exc()))

    if logfile_available:
        # Send the data to the generic logger
        crashlogger.error("")
        crashlogger.error("")
        crashlogger.error("==============================================================")
        crashlogger.error("==== SST TEST FRAMEWORKS CRASH REPORT ========================")
        crashlogger.error("==============================================================")
        crashlogger.error(err_str)
        crashlogger.error("==== TRACEBACK ===============================================")
        crashlogger.error(trc_str)
        crashlogger.error("==== CRASH REPORT FINISHED ===================================")

        # Dump Exception info to the Console
        print(("FATAL: SST Test Frameworks encountered ") +
              ("an unexpected exception ({0}))".format(exc_e)))
        print("SEE FILE {0} FOR TRACE INFORMATION".format(log_filename))
    else:
        # Cannot send crash report to file, so send to console as last resort
        print("")
        print("")
        print("==============================================================")
        print("==== SST TEST FRAMEWORKS CRASH REPORT ========================")
        print("==============================================================")
        print("NOTE: Unable to create/access crash log file {0}".format(log_filename))
        print("==============================================================")
        print(err_str)
        print("==== TRACEBACK ===============================================")
        print(trc_str)
        print("==== CRASH REPORT FINISHED ===================================")

    sys.exit(1)

####

def _verify_test_frameworks_is_available(sst_core_frameworks_dir):
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
