# -*- coding: utf-8 -*-
#
# Copyright 2009-2026 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2026, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

from sst_unittest import *
from sst_unittest_support import *


# Tests of SparseVectorMap.  Each test runs the
# coreTestElement.coreTestSparseVectorMap component in init mode with the test
# name as a model option; the component reports failures as "ERROR:"
# lines, so a passing run matches the default reference file.
class testcase_SparseVectorMap(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_SparseVectorMap_lookup(self):
        self.sparsevectormap_test_template("sparsevectormap_lookup")

    def test_SparseVectorMap_reference_stability(self):
        self.sparsevectormap_test_template("sparsevectormap_reference_stability")

    def test_SparseVectorMap_find_erase_clear(self):
        self.sparsevectormap_test_template("sparsevectormap_find_erase_clear")

    def test_SparseVectorMap_iteration(self):
        self.sparsevectormap_test_template("sparsevectormap_iteration")

    def test_SparseVectorMap_vector_constructor(self):
        self.sparsevectormap_test_template("sparsevectormap_vector_constructor")

    def test_SparseVectorMap_pointer_filter(self):
        self.sparsevectormap_test_template("sparsevectormap_pointer_filter")

    def test_SparseVectorMap_set(self):
        self.sparsevectormap_test_template("sparsevectormap_set")

    def test_SparseVectorMap_keyextract(self):
        self.sparsevectormap_test_template("sparsevectormap_keyextract")

    def test_SparseVectorMap_interleaved(self):
        self.sparsevectormap_test_template("sparsevectormap_interleaved")

    def test_SparseVectorMap_compact(self):
        self.sparsevectormap_test_template("sparsevectormap_compact")

    def test_SparseVectorMap_pair(self):
        self.sparsevectormap_test_template("sparsevectormap_pair")

#####

    def sparsevectormap_test_template(self, testtype):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        sdlfile = "{0}/test_SparseVectorMap.py".format(testsuitedir)
        reffile = "{0}/refFiles/test_SparseVectorMap_default.out".format(testsuitedir)
        outfile = "{0}/test_SparseVectorMap_{1}.out".format(outdir, testtype)

        options = "--run-mode=init --model-options=\"{0}\"".format(testtype)
        # The tests run entirely within the component constructor, so a
        # single serial run is sufficient
        self.run_sst(sdlfile, outfile, num_ranks=1, num_threads=1, other_args=options)

        filter1 = StartsWithFilter("WARNING: No components are")
        cmp_result = testing_compare_filtered_diff("sparsevectormap_" + testtype, outfile, reffile, True, [filter1])
        if not cmp_result:
            diffdata = testing_get_diff_data("sparsevectormap_" + testtype)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))
