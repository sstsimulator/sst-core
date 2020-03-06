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

""" This module is a group of global variables that must be common to all tests
"""
# Verbose Defines
VERBOSE_QUIET = 0
VERBOSE_NORMAL = 1
VERBOSE_LOUD = 2
VERBOSE_DEBUG = 3

# Global Var Defines
DEBUGMODE = None
VERBOSITY = None
SSTRUNNUMRANKS = None
SSTRUNNUMTHREADS = None
SSTRUNGLOBALARGS = None
TESTSUITEDIRPATH = None
TESTOUTPUTTOPDIRPATH = None
TESTOUTPUTRUNDIRPATH = None
TESTOUTPUTTMPDIRPATH = None
TESTOUTPUTXMLDIRPATH = None
TESTRUNNINGFLAG = None
JUNITTESTCASELIST = None
TESTSUITE_NAME_STR = None
CORECONFFILEPARSER = None

# These are some globals to pass data between the top level test engine
# and the lower level testscripts
def init_test_engine_globals():
    """ Initialize the test global variables """
    global DEBUGMODE
    global VERBOSITY
    global SSTRUNNUMRANKS
    global SSTRUNNUMTHREADS
    global SSTRUNGLOBALARGS
    global TESTSUITEDIRPATH
    global TESTOUTPUTTOPDIRPATH
    global TESTOUTPUTRUNDIRPATH
    global TESTOUTPUTTMPDIRPATH
    global TESTOUTPUTXMLDIRPATH
    global TESTRUNNINGFLAG
    global JUNITTESTCASELIST
    global TESTSUITE_NAME_STR
    global CORECONFFILEPARSER

    DEBUGMODE = False
    VERBOSITY = 1
    SSTRUNNUMRANKS = 1
    SSTRUNNUMTHREADS = 1
    SSTRUNGLOBALARGS = ["xxx"]
    TESTSUITEDIRPATH = ""
    TESTOUTPUTTOPDIRPATH = "./sst_test_outputs"
    TESTOUTPUTRUNDIRPATH = "{0}/run_data".format(TESTOUTPUTTOPDIRPATH)
    TESTOUTPUTTMPDIRPATH = "{0}/tmp_data".format(TESTOUTPUTTOPDIRPATH)
    TESTOUTPUTXMLDIRPATH = "{0}/xml_data".format(TESTOUTPUTTOPDIRPATH)
    TESTRUNNINGFLAG = False
    JUNITTESTCASELIST = []
    TESTSUITE_NAME_STR = ""
    CORECONFFILEPARSER = None
