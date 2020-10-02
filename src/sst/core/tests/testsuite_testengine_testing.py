# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

################################################################################
# Code to support a single instance module initialize, must be called setUp method

module_init = 0
module_sema = threading.Semaphore()

def initializeTestModule_SingleInstance(class_inst):
    global module_init
    global module_sema

    module_sema.acquire()
    if module_init != 1:
        # Put your single instance Init Code Here
        module_init = 1
    module_sema.release()

################################################################################

class testcase_testengine_testing_1(SSTTestCase):

    def initializeClass(self, testName):
        super(type(self), self).initializeClass(testName)
        # Put test based setup code here. it is called before testing starts
        # NOTE: This method is called once for every test

    def setUp(self):
        super(type(self), self).setUp()
        initializeTestModule_SingleInstance(self)
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    # DEMO tests
    def test_TESTING_CASE1_assert_success(self):
        log_forced("This Test Has an Expected Pass")
        self.assertEqual(1, 1)

    @unittest.expectedFailure
    def test_TESTING_CASE1_assert_fail1(self):
        log_forced("This Test Has an Expected Assert Failure")
        self.assertEqual(2, 1)

    @unittest.expectedFailure
    def test_TESTING_CASE1_assert_fail2(self):
        log_forced("This Test Has an Expected Assert Failure")
        self.assertEqual(2, 1)

    @unittest.expectedFailure
    def test_TESTING_CASE1_assert_fail3(self):
        log_forced("This Test Has an Expected Assert Failure")
        self.assertEqual(2, 1)

    @unittest.expectedFailure
    def test_TESTING_CASE1_assert_error1(self):
        log_forced("This Test Has an Expected Error")
        self.assertEqual(1 / 0, 1)

    @unittest.expectedFailure
    def test_TESTING_CASE1_assert_error2(self):
        log_forced("This Test Has an Expected Error")
        self.assertEqual(1 / 0, 1)

    @unittest.skip("Demonstrating Skipping #1")
    def test_TESTING_CASE1_skipping(self):
        log_forced("This Test Has an Expected Skip")
        self.assertEqual(1 / 0, 1)

    def test_TESTING_CASE1_get_info_from_sstsimulator_conf_success(self):
        # This should pass as we give valid data
        log_forced("This Test Has an Expected Pass")
        sourcedir = sstsimulator_conf_get_value_str("SSTCore", "sourcedir")
        log_forced("SSTCore SourceDir = {0}; Type = {1}".format(sourcedir, type(sourcedir)))
        if testing_check_is_py_2():
            if type(sourcedir) == str:
                self.assertEqual(str, type(sourcedir))
            elif type(sourcedir) == unicode:
                self.assertEqual(unicode, type(sourcedir))
            else:
                self.assertTrue(False)
        else:
            self.assertEqual(str, type(sourcedir))

    def test_TESTING_CASE1_get_info_from_sstsimulator_conf_invalid_section_exception_success(self):
        # This should pass as we detect an expected exception
        log_forced("This Test Has an Expected Pass - AND GENERATES A WARNING - From a Detected Exception")
        with self.assertRaises(SSTTestCaseException):
            sstsimulator_conf_get_value_str("invalid_section", "invalid_key")

    @unittest.expectedFailure
    def test_TESTING_CASE1_get_info_from_sstsimulator_conf_invalid_key_exception_error(self):
        # This should give an error as we detect an exception due to invalid key
        log_forced("This Test Has an Expected ERROR - AND GENERATES A WARNING - Due to invalid Key")
        sstsimulator_conf_get_value_str("SSTCore", "invalid_key")

    def test_TESTING_CASE1_get_info_from_sstsimulator_conf_invalid_key_rtn_default_success_with_warning(self):
        # This should pass by returning a default, but should log a warning
        log_forced("This Test Has an Expected Pass - BUT GENERATES A WARNING")
        sourcedir = sstsimulator_conf_get_value_str("SSTCore", "invalid_key", "kilroy_was_here")
        log_forced("SSTCore SourceDir = {0}".format(sourcedir))
        self.assertEqual(str, type(sourcedir))
        self.assertEqual("kilroy_was_here", sourcedir)

    def test_TESTING_CASE1_get_info_from_sst_config_h_valid_key_rtn_int_success(self):
        # This should pass as we give valid data
        log_forced("This Test Has an Expected Pass")
        test_define = sst_config_include_get_file_value_int("HAVE_CLOSEDIR", 123)
        log_forced("(#define HAVE_CLOSEDIR)-returneddata={0}; type={1}".format(test_define, type(test_define)))
        self.assertEqual(1, test_define)
        self.assertEqual(int, type(test_define))

    def test_TESTING_CASE1_get_info_from_sst_config_h_valid_key_rtn_str_success(self):
        # This should pass as we give valid data
        log_forced("This Test Has an Expected Pass")
        test_define = sst_config_include_file_get_value_str("PACKAGE_BUGREPORT", "THIS_IS_DEFAULT_DATA")
        log_forced("(#define PACKAGE_BUGREPORT)-returneddata={0}; type={1}".format(test_define, type(test_define)))
        self.assertEqual("sst@sandia.gov", test_define)
        self.assertEqual(str, type(test_define))

    def test_TESTING_CASE1_get_info_from_sst_config_h_invalid_key_rtn_default_str_success(self):
        # This should pass as we give valid data
        log_forced("This Test Has an Expected Pass - BUT GENERATES A WARNING")
        # This should pass by returning a default, but should log a warning
        test_define = sst_config_include_get_file_value_str("PACKAGE_BUGREPORT_KEYINVALID", "THIS_IS_DEFAULT_DATA")
        log_forced("(#define PACKAGE_BUGREPORT_KEYINVALID)-returneddata={0}; type={1}".format(test_define, type(test_define)))
        self.assertEqual("THIS_IS_DEFAULT_DATA", test_define)
        self.assertEqual(str, type(test_define))

    def test_TESTING_CASE1_get_info_from_sst_config_h_invalid_key_exception_success(self):
        # This should pass as we detect an expected exception
        log_forced("This Test Has an Expected Pass - BUT GENERATES A WARNING - From a Detected Exception")
        with self.assertRaises(SSTTestCaseException):
            test_define = sst_config_include_file_get_value_str("PACKAGE_BUGREPORT_KEYINVALID")
            log_forced("(#define PACKAGE_BUGREPORT_KEYINVALID)-returneddata={0}; type={1}".format(test_define, type(test_define)))


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

    def initializeClass(self, testName):
        super(type(self), self).initializeClass(testName)
        # Put test based setup code here. it is called before testing starts
        # NOTE: This method is called once for every test

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    # DEMO tests
    def test_TESTING_CASE2_assert_success(self):
        log_forced("This Test Has an Expected Pass")
        self.assertEqual(1, 1)

    @unittest.expectedFailure
    def test_TESTING_CASE2_assert_fail1(self):
        log_forced("This Test Has an Expected Assert Failure")
        self.assertEqual(2, 1)

    @unittest.expectedFailure
    def test_TESTING_CASE2_assert_error1(self):
        log_forced("This Test Has an Expected Error")
        self.assertEqual(1 / 0, 1)

    @unittest.skip("Demonstrating Skipping #2")
    def test_TESTING_CASE2_unittest_skipping(self):
        log_forced("This Test Has an Expected Skip")
        self.assertEqual(1 / 0, 1)

    skipreason = "Demonstrating Skipping using a scenario - if 'skip_scenario' is defined"
    @skip_on_scenario("skip_scenario", skipreason)
    def test_TESTING_CASE2_scenario_skipping(self):
        log_forced("This Test Has an Expected Skip due to scenario")
        self.assertEqual(1, 1)

    def test_TESTING_CASE2_general_support_test_ls_cmd_success(self):
        log_forced("This Test Has an Expected Pass")
        log_forced("This Test Generates Output From a ls Cmd")
        log("\n=======================================================")

        log("")
        log("=== ls cmd")
        os_ls()

    def test_TESTING_CASE2_general_support_test_cat_cmd_success(self):
        log_forced("This Test Has an Expected Pass")
        log_forced("This Test Generates Output From a cat Cmd")
        log("\n=======================================================")
        log("")
        log("=== cat VERSION file")
        os_cat("VERSION")

    def test_TESTING_CASE2_general_support_test_run_timeout_success(self):
        log_forced("This Test Has an Expected Pass")
        log_forced("This Test Runs a command to force a timeout")
        log("\n=======================================================")
        log("")
        log("=== Run tail and force a timeout")
        cmd = "tail".format()
        rtn = OSCommand(cmd).run(timeout_sec=3)
        log("Tail (forced Timeout) Rtn = {0}".format(rtn))
        self.assertEqual(True, rtn.timeout())
        log("\n=======================================================")


