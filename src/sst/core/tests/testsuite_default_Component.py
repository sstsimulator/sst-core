# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

################################################################################

class testcase_Component(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_Component(self):
        self.component_test_template("component")

#####

    def component_test_template(self, testtype):
        testsuitedir = self.get_testsuite_dir()
        outdir = get_test_output_run_dir()

        sdlfile = "{0}/test_Component.py".format(testsuitedir)
        reffile = "{0}/refFiles/test_Component.out".format(testsuitedir)
        outfile = "{0}/test_Component.out".format(outdir)

        self.run_sst(sdlfile, outfile)

        # Perform the test
        cmp_result = compare_sorted(testtype, outfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

