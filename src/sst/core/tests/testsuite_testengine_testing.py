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
    def test_TESTING_CASE1_success(self):
        self.assertEqual(1, 1)

    def test_TESTING_CASE1_fail1(self):
        self.assertEqual(2, 1)

    def test_TESTING_CASE1_fail2(self):
        self.assertEqual(2, 1)

    def test_TESTING_CASE1_fail3(self):
        self.assertEqual(2, 1)

    def test_TESTING_CASE1_error1(self):
        self.assertEqual(1 / 0, 1)

    def test_TESTING_CASE1_error2(self):
        self.assertEqual(1 / 0, 1)

    @unittest.skip("Demonstrating Skipping #1")
    def test_TESTING_CASE1_skipping(self):
        self.assertEqual(1 / 0, 1)

    def test_TESTING_CASE1_get_info_from_sstsimulator_conf_success(self):
        # This should pass as we give valid data
        sourcedir = get_sst_config_value_str("SSTCore", "sourcedir")
        log_forced("SSTCore SourceDir = {0}".format(sourcedir))

    def test_TESTING_CASE1_get_info_from_sstsimulator_conf_section_failure(self):
        # This should pass as we detect an expected exception
        with self.assertRaises(SSTTestCaseException):
            get_sst_config_value_str("invalid_section", "invalid_key")

    def test_TESTING_CASE1_get_info_from_sstsimulator_conf_key_failure(self):
        # This should give an error as we detect an exception due to invalid key
        get_sst_config_value_str("SSTCore", "invalid_key")

    def test_TESTING_CASE1_get_info_from_sstsimulator_conf_key_failure(self):
        # This should pass by returning a default, but should log a warning
        sourcedir = get_sst_config_value_str("SSTCore", "invalid_key", "kilroy_was_here")
        log_forced("SSTCore SourceDir = {0}".format(sourcedir))

    def test_TESTING_CASE1_get_info_from_sst_config_h_success(self):
        # This should pass as we give valid data
        test_define = get_sst_config_include_file_value_int("HAVE_CLOSEDIR", 123)
        log_forced("#define HAVE_CLOSEDIR={0}; type={1}".format(test_define, type(test_define)))

        test_define = get_sst_config_include_file_value_str("PACKAGE_BUGREPORT", "THIS_IS_DEFAULT_DATA")
        log_forced("#define PACKAGE_BUGREPORT={0}; type={1}".format(test_define, type(test_define)))

        # This should pass by returning a default, but should log a warning
        test_define = get_sst_config_include_file_value_str("PACKAGE_BUGREPORT_NOTFOUND", "THIS_IS_DEFAULT_DATA")
        log_forced("#define PACKAGE_BUGREPORT={0}; type={1}".format(test_define, type(test_define)))

    def test_TESTING_CASE1_get_info_from_sst_config_h_failure(self):
        # This should pass as we detect an expected exception
        with self.assertRaises(SSTTestCaseException):
            test_define = get_sst_config_include_file_value_str("PACKAGE_BUGREPORT")
            log_forced("#define PACKAGE_BUGREPORT={0}; type={1}".format(test_define, type(test_define)))


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
    def test_TESTING_CASE2_success(self):
        self.assertEqual(1, 1)

    def test_TESTING_CASE2_fail1(self):
        self.assertEqual(2, 1)

    def test_TESTING_CASE2_error1(self):
        self.assertEqual(1 / 0, 1)

    @unittest.skip("Demonstrating Skipping #2")
    def test_TESTING_CASE2_skipping(self):
        self.assertEqual(1 / 0, 1)

    def test_TESTING_CASE2_general_support(self):
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
        rtn = OSCommand(cmd).run(timeout_sec=3)
        log("Tail (forced Timeout) Rtn = {0}".format(rtn))

        log("\n=======================================================")


