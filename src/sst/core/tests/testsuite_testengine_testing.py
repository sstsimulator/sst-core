# -*- coding: utf-8 -*-

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

class testcase_testengine_testing_1(SSTTestCase):

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

    # DEMO tests
    def test_DEBUG_CASE1_success(self):
        self.assertEqual(1, 1)

    def test_DEBUG_CASE1_fail1(self):
        self.assertEqual(2, 1)

    def test_DEBUG_CASE1_fail2(self):
        self.assertEqual(2, 1)

    def test_DEBUG_CASE1_fail3(self):
        self.assertEqual(2, 1)

    def test_DEBUG_CASE1_error1(self):
        self.assertEqual(1 / 0, 1)

    def test_DEBUG_CASE1_error2(self):
        self.assertEqual(1 / 0, 1)

    @unittest.skip("Demonstrating Skipping #1")
    def test_DEBUG_CASE1_skipping(self):
        self.assertEqual(1 / 0, 1)

################################################################################
################################################################################
################################################################################

class testcase_testengine_testing_2(SSTTestCase):

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

    # DEMO tests
    def test_DEBUG_CASE2_success(self):
        self.assertEqual(1, 1)

    def test_DEBUG_CASE2_fail1(self):
        self.assertEqual(2, 1)

    def test_DEBUG_CASE2_error1(self):
        self.assertEqual(1 / 0, 1)

    @unittest.skip("Demonstrating Skipping #2")
    def test_DEBUG_CASE2_skipping(self):
        self.assertEqual(1 / 0, 1)

    def test_DEBUG_CASE2_general_support(self):
        log("\n=======================================================")

        log("")
        log("=== ls cmd")
        os_ls()

        log("")
        log("=== cat VERSION file")
        os_cat("VERSION")

        log("")
        log("=== Run tail and force a timeout")
        cmd = "tail".format()
        rtn = OSCommand(cmd).run(timeout_sec=5)
        log("Tail (forced Timeout) Rtn = {0}".format(rtn))

        log("\n=======================================================")


