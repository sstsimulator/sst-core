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

from sst_unittest import *
from sst_unittest_support import *


class testcase_ParamComponent(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_ParamComponent(self):
        self.param_component_test_template("param_component")

#####

    def param_component_test_template(self, testtype):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        sdlfile = "{0}/test_ParamComponent.py".format(testsuitedir)
        reffile = "{0}/refFiles/test_ParamComponent.out".format(testsuitedir)
        outfile = "{0}/test_ParamComponent.out".format(outdir)

        self.run_sst(sdlfile, outfile)

        # Perform the test
        filter1 = StartsWithFilter("WARNING: No components are")
        filter2 = StartsWithFilter("#")
        ws_filter = IgnoreWhiteSpaceFilter()
        cmp_result = testing_compare_filtered_diff(testtype, outfile, reffile, True, [filter1, filter2, ws_filter])
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))
