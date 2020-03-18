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

""" This module is a group of global variables that must be common to all tests
"""
# Verbose Defines
VERBOSE_QUIET = 0
VERBOSE_NORMAL = 1
VERBOSE_LOUD = 2
VERBOSE_DEBUG = 3

# Global Var Defines
TESTSUITE_NAME_STR = None
TESTSUITEDIRPATH = None
TESTRUNNINGFLAG = None
DEBUGMODE = None
VERBOSITY = None
SSTRUNNUMRANKS = None
SSTRUNNUMTHREADS = None
SSTRUNGLOBALARGS = None
TESTOUTPUTTOPDIRPATH = None
TESTOUTPUTRUNDIRPATH = None
TESTOUTPUTTMPDIRPATH = None
TESTOUTPUTXMLDIRPATH = None
JUNITTESTCASELIST = None
CORECONFFILEPARSER = None
CORECONFINCLUDEFILEDICT = None
TESTENGINEERRORCOUNT = 0
TESTSCENARIOLIST = None

# These are some globals to pass data between the top level test engine
# and the lower level testscripts
def init_test_engine_globals():
    """ Initialize the test global variables """
    global TESTSUITE_NAME_STR
    global TESTSUITEDIRPATH
    global TESTRUNNINGFLAG
    global DEBUGMODE
    global VERBOSITY
    global SSTRUNNUMRANKS
    global SSTRUNNUMTHREADS
    global SSTRUNGLOBALARGS
    global TESTOUTPUTTOPDIRPATH
    global TESTOUTPUTRUNDIRPATH
    global TESTOUTPUTTMPDIRPATH
    global TESTOUTPUTXMLDIRPATH
    global JUNITTESTCASELIST
    global CORECONFFILEPARSER
    global CORECONFINCLUDEFILEDICT
    global TESTENGINEERRORCOUNT
    global TESTSCENARIOLIST

    TESTSUITE_NAME_STR = ""
    TESTSUITEDIRPATH = ""
    TESTRUNNINGFLAG = False
    DEBUGMODE = False
    VERBOSITY = 1
    SSTRUNNUMRANKS = 1
    SSTRUNNUMTHREADS = 1
    SSTRUNGLOBALARGS = ["xxx"]
    TESTOUTPUTTOPDIRPATH = os.path.abspath("./sst_test_outputs")
    TESTOUTPUTRUNDIRPATH = os.path.abspath("{0}/run_data".format(TESTOUTPUTTOPDIRPATH))
    TESTOUTPUTTMPDIRPATH = os.path.abspath("{0}/tmp_data".format(TESTOUTPUTTOPDIRPATH))
    TESTOUTPUTXMLDIRPATH = os.path.abspath("{0}/xml_data".format(TESTOUTPUTTOPDIRPATH))
    JUNITTESTCASELIST = []
    CORECONFFILEPARSER = None
    CORECONFINCLUDEFILEDICT = {}
    TESTENGINEERRORCOUNT = 0
    TESTSCENARIOLIST = []
