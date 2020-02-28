# -*- coding: utf-8 -*-

import os
import sys
import filecmp

import sst_unittest_support
from sst_unittest_support import *

################################################################################

def setUpModule():
    sst_unittest_support.setUpModule()
    # Put Module based setup code here. it is called before any testcases are run

def tearDownModule():
    # Put Module based teardown code here. it is called after all testcases are run
    sst_unittest_support.tearDownModule()

################################################################################

class testcase_Component(SSTTestCase):

    @classmethod
    def setUpClass(cls):
        super(cls, cls).setUpClass()
        # Put class based setup code here. it is called once before tests are run

    @classmethod
    def tearDownClass(cls):
        # Put class based teardown code here. it is called once after tests are run
        super(cls, cls).tearDownClass()

#####

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_Component(self):
        self.component_test_template()

#####

    def component_test_template(self):
        # Set the various file paths
        sdlfile = "{0}/test_Component.py".format(self.get_testsuite_dir())
        reffile = "{0}/refFiles/test_Component.out".format(self.get_testsuite_dir())
        outfile = "{0}/test_Component.out".format(self.get_test_output_run_dir())

        self.run_sst(sdlfile, outfile)

        # Perform the test
        cmp_result = self.compare_sorted(outfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

#####

    def compare_sorted(self, outfile, reffile):
       sorted_outfile = "{0}/coreTestComponent_sorted_outfile".format(self.get_test_output_tmp_dir())
       sorted_reffile = "{0}/coreTestComponent_sorted_reffile".format(self.get_test_output_tmp_dir())

       os.system("sort -o {0} {1}".format(sorted_outfile, outfile))
       os.system("sort -o {0} {1}".format(sorted_reffile, reffile))

       return filecmp.cmp(sorted_outfile, sorted_reffile)
