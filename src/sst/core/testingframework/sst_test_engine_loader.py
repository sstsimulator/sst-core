# -*- coding: utf-8 -*-

# Copyright 2009-2023 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2023, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

""" This module is the primary loader for the SST Testing Frameworks Engine
    it will verify that the test frameworks files are installed, and then will
    load the test_engine module.  Once loaded, tests will be discovered and run.
"""

import sys
import os.path
import traceback
import logging
from logging.handlers import RotatingFileHandler

################################################################################

TESTFRAMEWORKSFILES = ["sst_unittest.py", "sst_unittest_support.py",
                       "sst_unittest_parameterized.py", "test_engine.py",
                       "test_engine_globals.py", "test_engine_support.py",
                       "test_engine_junit.py", "test_engine_unittest.py"]

# AVAILABLE TEST MODES
TEST_ELEMENTS = 0
TEST_SST_CORE = 1

REQUIRED_PY_MAJ_VER_2 = 2               # Required Python2 Major Version
REQUIRED_PY_MAJ_VER_2_MINOR_VER = 7     # Required PY2 Minor Version
REQUIRED_PY_MAJ_VER_2_SUB_MINOR_VER = 5 # Required PY2 Sub-Minor Version
REQUIRED_PY_MAJ_VER_3 = 3               # Required Python 3 Major Version
REQUIRED_PY_MAJ_VER_3_MINOR_VER = 4     # Required PY3 Minor Version
REQUIRED_PY_MAJ_VER_3_SUB_MINOR_VER = 0 # Required PY3 Sub-Minor Version
REQUIRED_PY_MAJ_VER_MAX = REQUIRED_PY_MAJ_VER_3 # Highest supported Major Version

################################################################################

def check_python_version():
    # Validate Python Versions
    ver = sys.version_info

    # Check for Py2.x or Py3.x Versions
    if (ver[0] < REQUIRED_PY_MAJ_VER_2) or (ver[0] > REQUIRED_PY_MAJ_VER_3):
        print(("SST Test Engine requires Python major version {0} or {1}\n" +
               "Found Python version is:\n{2}").format(REQUIRED_PY_MAJ_VER_2,
                                                       REQUIRED_PY_MAJ_VER_MAX,
                                                       sys.version))
        sys.exit(1)

    # Check to ensure minimum Py2 version
    if ((ver[0] == REQUIRED_PY_MAJ_VER_2) and (ver[1] < REQUIRED_PY_MAJ_VER_2_MINOR_VER)) or \
       ((ver[0] == REQUIRED_PY_MAJ_VER_2) and (ver[1] == REQUIRED_PY_MAJ_VER_2_MINOR_VER)  and
                                              (ver[2] < REQUIRED_PY_MAJ_VER_2_SUB_MINOR_VER)):
        print(("SST Test Engine requires Python 2 version {0}.{1}.{2} or greater\n" +
               "Found Python version is:\n{3}").format(REQUIRED_PY_MAJ_VER_2,
                                                       REQUIRED_PY_MAJ_VER_2_MINOR_VER,
                                                       REQUIRED_PY_MAJ_VER_2_SUB_MINOR_VER,
                                                       sys.version))
        sys.exit(1)

    # Check to ensure minimum Py3 version
    if ((ver[0] == REQUIRED_PY_MAJ_VER_3) and (ver[1] < REQUIRED_PY_MAJ_VER_3_MINOR_VER)) or \
       ((ver[0] == REQUIRED_PY_MAJ_VER_3) and (ver[1] == REQUIRED_PY_MAJ_VER_3_MINOR_VER)  and
                                              (ver[2] < REQUIRED_PY_MAJ_VER_3_SUB_MINOR_VER)):
        print(("SST Test Engine requires Python 3 version {0}.{1}.{2} or greater\n" +
               "Found Python version is:\n{3}").format(REQUIRED_PY_MAJ_VER_3,
                                                       REQUIRED_PY_MAJ_VER_3_MINOR_VER,
                                                       REQUIRED_PY_MAJ_VER_3_SUB_MINOR_VER,
                                                       sys.version))
        sys.exit(1)
####

def startup_and_run(sst_core_bin_dir, test_mode):
    """ This is the main entry point for loading and running the SST Test Frameworks
        Engine.

        The function will first verify that the frameworks files are available, and
        then load the test engine.  After loading the test engine, it will then
        start discovery of testsuites and then run the tests.

        If there is an error loading the test engine the script will error exit.

        Any unhandled expections that occur during the run of the test engine
        will be handled by a generic exception handler which will log the failure
        across multiple log files.

        Args:
            sst_core_bin_dir: The SST-Core installed binary directory.
            test_mode: 1 for Core Testing, 0 for Elements testing.
    """
    try:
        # Check the python version and make sure we can run
        check_python_version()

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
    log_filename = "./sst_test_frameworks_crashreport.log"
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
        print("{0} ".format(traceback.format_exc()))
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
