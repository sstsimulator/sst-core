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
JUNITTESTCASELIST = None
TESTSUITE_NAME_STR = None
TESTSUITEDIRPATH = None
TESTRUNNINGFLAG = None

TESTOUTPUT_TOPDIRPATH = None
TESTOUTPUT_RUNDIRPATH = None
TESTOUTPUT_TMPDIRPATH = None
TESTOUTPUT_XMLDIRPATH = None
TESTENGINE_CONCURRENTMODE = None
TESTENGINE_DEBUGMODE = None
TESTENGINE_VERBOSITY = None
TESTENGINE_SSTRUN_NUMRANKS = None
TESTENGINE_SSTRUN_NUMTHREADS = None
TESTENGINE_SSTRUN_GLOBALARGS = None
TESTENGINE_CORE_CONFFILE_PARSER = None
TESTENGINE_CORE_CONFINCLUDE_DICT = None
TESTENGINE_ERRORCOUNT = 0
TESTENGINE_SCENARIOSLIST = None

# These are some globals to pass data between the top level test engine
# and the lower level testscripts
def init_test_engine_globals():
    """ Initialize the test global variables """
    global JUNITTESTCASELIST
    global TESTSUITE_NAME_STR
    global TESTSUITEDIRPATH
    global TESTRUNNINGFLAG

    global TESTOUTPUT_TOPDIRPATH
    global TESTOUTPUT_RUNDIRPATH
    global TESTOUTPUT_TMPDIRPATH
    global TESTOUTPUT_XMLDIRPATH
    global TESTENGINE_CONCURRENTMODE
    global TESTENGINE_DEBUGMODE
    global TESTENGINE_VERBOSITY
    global TESTENGINE_SSTRUN_NUMRANKS
    global TESTENGINE_SSTRUN_NUMTHREADS
    global TESTENGINE_SSTRUN_GLOBALARGS
    global TESTENGINE_CORE_CONFFILE_PARSER
    global TESTENGINE_CORE_CONFINCLUDE_DICT
    global TESTENGINE_ERRORCOUNT
    global TESTENGINE_SCENARIOSLIST

#    JUNITTESTCASELIST = []
    JUNITTESTCASELIST = {}
    TESTSUITE_NAME_STR = ""
    TESTSUITEDIRPATH = ""
    TESTRUNNINGFLAG = False

    TESTOUTPUT_TOPDIRPATH = os.path.abspath("./sst_test_outputs")
    TESTOUTPUT_RUNDIRPATH = os.path.abspath("{0}/run_data".format(TESTOUTPUT_TOPDIRPATH))
    TESTOUTPUT_TMPDIRPATH = os.path.abspath("{0}/tmp_data".format(TESTOUTPUT_TOPDIRPATH))
    TESTOUTPUT_XMLDIRPATH = os.path.abspath("{0}/xml_data".format(TESTOUTPUT_TOPDIRPATH))
    TESTENGINE_CONCURRENTMODE = False
    TESTENGINE_DEBUGMODE = False
    TESTENGINE_VERBOSITY = 1
    TESTENGINE_SSTRUN_NUMRANKS = 1
    TESTENGINE_SSTRUN_NUMTHREADS = 1
    TESTENGINE_SSTRUN_GLOBALARGS = ["xxx"]
    TESTENGINE_CORE_CONFFILE_PARSER = None
    TESTENGINE_CORE_CONFINCLUDE_DICT = {}
    TESTENGINE_ERRORCOUNT = 0
    TESTENGINE_SCENARIOSLIST = []