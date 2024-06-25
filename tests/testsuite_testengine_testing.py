# -*- coding: utf-8 -*-
#
# Copyright 2009-2024 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2024, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

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

####################################################################
# These tests verify general operation of the SST Testing Frameworks
####################################################################

class testcase_testengine_testing_frameworks_operation(SSTTestCase):

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

######

    def test_frameworks_operation_assert_success(self):
        log_forced("NOTE: This Test Has an Expected Pass (1 = 1) and should show as 'PASS'")
        self.assertEqual(1, 1)

    @unittest.expectedFailure
    def test_frameworks_operation_assert_fail1(self):
        log_forced("NOTE: This Test Has an Expected Failure (2 != 1) and should show as 'EXPECTED FAILURE'")
        self.assertEqual(2, 1)

    def test_frameworks_operation_assert_fail2(self):
        log_forced("NOTE: This Test Has an Expected Failure (2 != 1) and should show as 'FAIL'")
        self.assertEqual(2, 1)

    @unittest.expectedFailure
    def test_frameworks_operation_assert_error1(self):
        log_forced("NOTE: This Test Has an Expected Error (x = 1 / 0) and should show as 'EXPECTED FAILURE'")
        x = 1/0
        self.assertEqual(1,  1)

    def test_frameworks_operation_assert_error2(self):
        log_forced("NOTE: This Test Has an Expected Error (x = 1 / 0) and should show as 'ERROR'")
        x = 1/0
        self.assertEqual(1,  1)

    @unittest.expectedFailure
    def test_frameworks_operation_assert_error3(self):
        log_forced("NOTE: This Test is Marked as an 'Expected Error', but should PASS (1 = 1);  and should show as 'UNEXPECTED SUCCESS'")
        self.assertEqual(1,  1)

    @unittest.skip("NOTE: Skip Test 1 - This Test Has an Expected Skip and should show as 'SKIPPED'")
    def test_frameworks_operation_skipping1(self):
        self.assertEqual(1 / 0, 1)

    def test_frameworks_operation_skipping2(self):
        self.skipTest("NOTE: Skip Test 2 - This Test Has an Expected Skip and should show as 'SKIPPED'")
        self.assertEqual(1 / 0, 1)

    def test_frameworks_operation_run_timeout_success(self):
        log_forced("NOTE: This Test Has an Expected Pass and should show as 'PASS'")
        cmd = "tail".format()
        rtn = OSCommand(cmd).run(timeout_sec=3)
        log("Tail (forced Timeout) Rtn = {0}".format(rtn))
        self.assertEqual(True, rtn.timeout())

################################################################################
################################################################################
################################################################################

############################################################################
# These tests verify operation of the SST UunitTest Support Functions
############################################################################

class testcase_testengine_testing_support_functions(SSTTestCase):

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

    ############################################################################
    # Test read access of the sstsimulator configuration file
    # Note: We can only test the sstsimulator_conf_get_value_str(); as we have
    #       No generic entries for _int, _float, _bool
    ############################################################################

    def test_support_functions_get_info_from_sstsimulator_conf_success(self):
        # This should pass as we give valid data
        log_forced("NOTE: This Test Has an Expected Pass and should show as 'PASS'")
        sourcedir = sstsimulator_conf_get_value_str("SSTCore", "sourcedir")
        log_forced("SSTCore SourceDir = {0}; Type = {1}".format(sourcedir, type(sourcedir)))
        self.assertIsInstance(sourcedir, str)

    def test_support_functions_get_info_from_sstsimulator_conf_invalid_section_exception_success(self):
        # This should pass as we detect an expected exception due to invalid section
        log_forced("NOTE: This Test Has an Expected Pass (BUT GENERATES A WARNING) and should show as 'PASS'")
        with self.assertRaises(SSTTestCaseException):
            sstsimulator_conf_get_value_str("invalid_section", "invalid_key")

    @unittest.expectedFailure
    def test_support_functions_get_info_from_sstsimulator_conf_invalid_key_exception_error(self):
        # This should give an error as we detect an exception due to invalid key
        log_forced("NOTE: This Test Has an Expected ERROR (BUT GENERATES A WARNING) and should show as 'EXPECTED FAILURE'")
        sstsimulator_conf_get_value_str("SSTCore", "invalid_key")

    def test_support_functions_get_info_from_sstsimulator_conf_invalid_key_rtn_default_success_with_warning(self):
        # This should pass by failing to find a valid key and returning a default, but should log a warning
        log_forced("NOTE: This Test Has an Expected Pass (BUT GENERATES A WARNING) and should show as 'PASS'")
        sourcedir = sstsimulator_conf_get_value_str("SSTCore", "invalid_key", "kilroy_was_here")
        log_forced("SSTCore SourceDir = {0}".format(sourcedir))
        self.assertEqual(str, type(sourcedir))
        self.assertEqual("kilroy_was_here", sourcedir)

    def test_support_functions_find_section_from_sstsimulator_conf_success(self):
        log_forced("NOTE: This Test Has an Expected Pass and should show as 'PASS'")
        section = "SSTCore"
        find_result = sstsimulator_conf_does_have_section(section)
        log_forced("sstsimulator config file has section {0} = {1}".format(section, find_result))
        self.assertTrue(find_result)

    def test_support_functions_no_find_section_from_sstsimulator_conf_success(self):
        log_forced("NOTE: This Test Has an Expected Pass and should show as 'PASS'")
        section = "SSTCore_INVALID"
        find_result = sstsimulator_conf_does_have_section(section)
        log_forced("sstsimulator config file has section {0} = {1}".format(section, find_result))
        self.assertFalse(find_result)

    def test_support_functions_find_key_from_sstsimulator_conf_success(self):
        log_forced("NOTE: This Test Has an Expected Pass and should show as 'PASS'")
        section = "SSTCore"
        key = "sourcedir"
        find_result = sstsimulator_conf_does_have_key(section, key)
        log_forced("sstsimulator config file has section {0}; key {1}   = {2}".format(section, key, find_result))
        self.assertTrue(find_result)

    def test_support_functions_no_find_key_from_sstsimulator_conf_success(self):
        log_forced("NOTE: This Test Has an Expected Pass and should show as 'PASS'")
        section = "SSTCore"
        key = "sourcedir_INVALID"
        find_result = sstsimulator_conf_does_have_key(section, key)
        log_forced("sstsimulator config file has section {0}; key {1} = {2}".format(section, key, find_result))
        self.assertFalse(find_result)

    def test_support_functions_get_all_sections_from_sstsimulator_conf_success(self):
        log_forced("NOTE: This Test Has an Expected Pass and should show as 'PASS'")
        result = sstsimulator_conf_get_sections()
        log_forced("sstsimulator config file has sections {0}".format(result))
        self.assertTrue(list, type(result))

    def test_support_functions_get_all_keys_from_section_from_sstsimulator_conf_success(self):
        log_forced("NOTE: This Test Has an Expected Pass and should show as 'PASS'")
        section = "SSTCore"
        result = sstsimulator_conf_get_section_keys(section)
        log_forced("sstsimulator config file section {0} has keys {1}".format(section, result))
        self.assertTrue(list, type(result))

    def test_support_functions_get_all_key_vaules_from_section_from_sstsimulator_conf_success(self):
        log_forced("NOTE: This Test Has an Expected Pass and should show as 'PASS'")
        section = "SSTCore"
        result = sstsimulator_conf_get_all_keys_values_from_section(section)
        log_forced("sstsimulator config file section {0} has key/values {1}".format(section, result))
        self.assertTrue(list, type(result))

    ############################################################################
    # Test read access of the sst_config.h file
    ############################################################################

    def test_support_functions_get_info_from_sst_config_h_valid_key_rtn_int_success(self):
        # This should pass as we give/get valid data
        log_forced("NOTE: This Test Has an Expected Pass and should show as 'PASS'")
        test_define = sst_config_include_file_get_value_int("HAVE_CLOSEDIR", 123)
        log_forced("#define HAVE_CLOSEDIR={0}; type={1}".format(test_define, type(test_define)))
        self.assertEqual(1, test_define)
        self.assertEqual(int, type(test_define))

    def test_support_functions_get_info_from_sst_config_h_valid_key_rtn_str_success(self):
        # This should pass as we give/get valid data
        log_forced("NOTE: This Test Has an Expected Pass and should show as 'PASS'")
        test_define = sst_config_include_file_get_value_str("PACKAGE_BUGREPORT", "THIS_IS_DEFAULT_DATA")
        log_forced("#define PACKAGE_BUGREPORT={0}; type={1}".format(test_define, type(test_define)))
        self.assertEqual("sst@sandia.gov", test_define)
        self.assertEqual(str, type(test_define))

    def test_support_functions_get_info_from_sst_config_h_invalid_key_rtn_default_str_success(self):
        # This should pass as we give valid data
        log_forced("NOTE: This Test Has an Expected Pass (BUT GENERATES A WARNING) and should show as 'PASS'")
        # This should pass by returning a default, but should log a warning
        test_define = sst_config_include_file_get_value_str("PACKAGE_BUGREPORT_KEYINVALID", "THIS_IS_DEFAULT_DATA")
        log_forced("#define PACKAGE_BUGREPORT_KEYINVALID={0}; type={1}".format(test_define, type(test_define)))
        self.assertEqual("THIS_IS_DEFAULT_DATA", test_define)
        self.assertEqual(str, type(test_define))

    def test_support_functions_get_info_from_sst_config_h_invalid_key_exception_success(self):
        # This should pass as we detect an expected exception
        log_forced("NOTE: This Test Has an Expected Pass (BUT GENERATES A WARNING) and should show as 'PASS'")
        with self.assertRaises(SSTTestCaseException):
            test_define = sst_config_include_file_get_value_str("PACKAGE_BUGREPORT_KEYINVALID")
            log_forced("#define PACKAGE_BUGREPORT_KEYINVALID={0}; type={1}".format(test_define, type(test_define)))

    ############################################################################
    # Test Skipping Functions
    ############################################################################

    skipreason1 = "This Test Has an Expected Skip (skip_on_scenario) and should show as 'SKIPPED'"
    test_engine_globals.TESTENGINE_SCENARIOSLIST.append("testing_scenario")
    @skip_on_scenario("testing_scenario", skipreason1)
    def test_support_functions_scenario_skipping(self):
        log_forced("This Test Has an Expected Skip due to scenario 'testing_scenario'")
        self.assertEqual(1, 1)

    skipreason2 = "This Test Has an Expected Skip (skip_on_sstsimulator_conf_empty_str) and should show as 'SKIPPED'"
    @skip_on_sstsimulator_conf_empty_str("SSTCore", "invalid_key", skipreason2)
    def test_support_functions_sstsimulator_conf_skipping(self):
        log_forced("This Test Has an Expected Skip due to invalid key in section SSTCore in the sstsimulator config file")
        self.assertEqual(1, 1)


    ############################################################################
    # Test Other Support Functions
    ############################################################################

    def test_support_functions_test_os_pwd_cmd_success(self):
        log_forced("NOTE: This Test Has an Expected Pass and should show as 'PASS'")
        output = os_pwd(echo_out = False)
        log_forced("pwd = {0}".format(output))
        self.assertIsInstance(output, str)

    def test_support_functions_test_os_ls_cmd_success(self):
        log_forced("NOTE: This Test Has an Expected Pass and should show as 'PASS'")
        output = os_ls(self.get_testsuite_dir(), echo_out = False)
        log_forced("ls -lia =\n{0}".format(output))
        self.assertIsInstance(output, str)

    def test_support_functions_test_os_cat_cmd_success(self):
        log_forced("NOTE: This Test Has an Expected Pass and should show as 'PASS'")
        catfile = "{0}/{1}".format(self.get_testsuite_dir(), "testsuite_default_UnitAlgebra.py")
        output = os_cat(catfile, echo_out = False)
        log_forced("cat cmd output =\n{0}".format(output))
        self.assertIsInstance(output, str)

