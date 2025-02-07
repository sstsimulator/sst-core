# -*- coding: utf-8 -*-
#
# Copyright 2009-2025 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2025, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import os
import filecmp

from sst_unittest import *
from sst_unittest_support import *


class testcase_Output(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_Output_TraceFunction(self):
        os.unsetenv("SST_TRACEFUNCTION_INDENT_MARKER");    
        self.tracefunction_test_template("TraceFunction")

    def test_Output_TraceFunction_IndentMarker(self):
        os.environ["SST_TRACEFUNCTION_INDENT_MARKER"] = "";
        self.tracefunction_test_template("TraceFunction_IndentMarker")

#####
    def tracefunction_test_template(self, testtype):

        os.environ["SST_TRACEFUNCTION_ACTIVATE"] = "";

        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        sdlfile = "{0}/test_Output.py".format(testsuitedir)
        reffile = "{0}/refFiles/test_Output_{1}.out".format(testsuitedir,testtype)

        outfile = "{0}/test_Output_{1}.out".format(outdir,testtype)

        # Always run the TraceFunction test, the only difference is
        # how the environment variables are set
        options = "--model-options=\"TraceFunction\""
        # Run the simulation
        self.run_sst(sdlfile, outfile, other_args=options)

        # Check the results.  We will need to do a separate test for
        # each partition in the simulation.  We do this by filtering
        # all lines prefixed with rank info and only keeping the ones
        # that match the partition we're looking at.

        filter_warning = StartsWithFilter("WARNING:")
        filter_time = StartsWithFilter("Simulation is complete")
        filters = [filter_warning, filter_time]

        # Need to do a check for each partition in the run
        ranks = testing_check_get_num_ranks()
        threads = testing_check_get_num_threads()

        if ( ranks == 1 and threads == 1 ):
            cmp_result = testing_compare_filtered_diff("tracefunction", outfile, reffile, False, filters)
            self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))
        else:
            cmp_result = True
            tf_filter = TraceFunctionFilter("") # will set for each loop
            filters.append(tf_filter)
            for r in range(ranks):
                for t in range(threads):
                    tf_filter.setPrefix("[{0}:{1}] ".format(r,t))
                    cmp_result &= testing_compare_filtered_diff("tracefunction", outfile, reffile, False, filters)

            self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))
            
                    
class TraceFunctionFilter(LineFilter):
    def __init__(self, prefix):
        super().__init__()
        self._prefix = prefix

    def filter(self, line):
        """
        If line starts with a paren, then it will keep lines where
        the line starts with prefix and discard lines where that
        don't.  Kept lines will have the prefix removed. If it doesn't
        start with a paren, then it is passed through unchanged.

            Args:
                line (str): Line to check

            Returns:
                filtered line or None as described above

        """
        # Just return lines that don't start with [, they aren't from
        # TraceFunction
        if not line.startswith("["):
            return line

        # Throw away lines that start with [, but don't match the
        # prefix
        if not line.startswith(self._prefix):
            return None

        # Cut the prefix out of matching lines and return
        return line[len(self._prefix):]

    def setPrefix(self, prefix):
        self._prefix = prefix
