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

""" This module is the derived python unittest and testtools classes to support
    the requirements for SST Testing.
"""

import sys
import unittest
import time
import traceback
import threading
from datetime import datetime

# Queue module changes name between Py2->Py3
try:
    import Queue
    Queue = Queue.Queue
except ImportError:
    import queue
    Queue = queue.Queue

# Try to import testtools (this may not be installed on system)
try:
    import testtools
    from testtools.testsuite import ConcurrentTestSuite
    from testtools.testsuite import iterate_tests
    from testtools.testsuite import FixtureSuite
    test_suite_base_class = ConcurrentTestSuite
except ImportError:
    # If we fail to import, just trick the system to use unittest.TestSuite
    # This allows us to continue, but not support concurrent testing
    test_suite_base_class = unittest.TestSuite

import test_engine_globals
from sst_unittest_support import *
from test_engine_support import strclass
from test_engine_support import strqual
from test_engine_junit import JUnitTestCase

################################################################################

def verify_concurrent_test_engine_available():
    # Check to see if we can load testtools if the user wants to run
    # in concurrent mode.
    if test_engine_globals.TESTENGINE_CONCURRENTMODE:
        try:
            import testtools
        except ImportError:
            errmsg = ("Test Frameworks Cannot Run Concurrently - ") + \
                     ("User must perform 'pip install testtools'")
            log_fatal(errmsg)

################################################################################

class SSTTextTestRunner(unittest.TextTestRunner):
    """ A superclass to support SST required testing """

    def __init__(self, stream=sys.stderr, descriptions=True, verbosity=1,
                 failfast=False, buffer=False, resultclass=None):
        super(SSTTextTestRunner, self).__init__(stream, descriptions, verbosity,
                                                failfast, buffer, resultclass)
        log(("\n=== TESTS STARTING ================") +
            ("===================================\n"))

###

    def run(self, test):
        runresults = super(SSTTextTestRunner, self).run(test)
        self._get_and_display_test_results(runresults)
        return runresults

###

    def did_tests_pass(self, run_results):
        """ Figure out if testing passed
            :param: run_results -  A unittest.TestResult object
            :return: True if all tests passing with no errors, false otherwise
        """
        return run_results.wasSuccessful and \
        len(run_results.errors) == 0 and \
        test_engine_globals.TESTENGINE_ERRORCOUNT == 0

###

    def _get_and_display_test_results(self, run_results):
        """ Figure out if testing passed, and display the test results
            :param: sst_tests_results -  A unittest.TestResult object
            :return: True if all tests passing with no errors, false otherwise
        """
        log(("\n=== TEST RESULTS ==================") +
                   ("===================================\n"))
        log("Tests Run      = {0}".format(run_results.testsRun))
        log("Tests Failures = {0}".format(len(run_results.failures)))
        log("Tests Skipped  = {0}".format(len(run_results.skipped)))
        log("Tests Errors   = {0}".format(len(run_results.errors)))
        if self.did_tests_pass(run_results):
            log_forced("\n== TESTING PASSED ==")
        else:
            if test_engine_globals.TESTENGINE_ERRORCOUNT == 0:
                log_forced("\n== TESTING FAILED ==")
            else:
                log_forced("\n== TESTING FAILED DUE TO ERRORS ==")
        log(("\n===================================") +
            ("===================================\n"))

################################################################################

class SSTTextTestResult(unittest.TextTestResult):
    """ A superclass to support SST required testing """

    def __init__(self, stream, descriptions, verbosity):
        super(SSTTextTestResult, self).__init__(stream, descriptions, verbosity)

###

    def startTest(self, test):
        super(SSTTextTestResult, self).startTest(test)
        log_forced("DEBUG - startTest: Test = {0}\n".format(test))
        self._start_time = time.time()
        self._test_name = "undefined_testname"
        _testName = getattr(test, 'testName', None)
        if _testName is not None:
            self._test_name = test.testName
        self._testcase_name = strqual(test.__class__)
        self._testsuite_name = strclass(test.__class__)
        timestamp = datetime.utcnow().strftime("%Y_%m%d_%H:%M:%S.%f utc")
        self._junit_test_case = JUnitTestCase(self._test_name,
                                              self._testcase_name,
                                              timestamp=timestamp)

    def stopTest(self, test):
        super(SSTTextTestResult, self).stopTest(test)
        log_forced("DEBUG - stopTest: Test = {0}\n".format(test))
        self._junit_test_case.junit_add_elapsed_sec(time.time() - self._start_time)
        if not test_engine_globals.TESTENGINE_CONCURRENTMODE:
            test_engine_globals.JUNITTESTCASELIST['singlethread'].append(self._junit_test_case)
        else:
            test_engine_globals.JUNITTESTCASELIST[self._testsuite_name].append(self._junit_test_case)

###

    def addSuccess(self, test):
        #super(SSTTextTestResult, self).addSuccess(test)
        #log_forced("DEBUG - addSuccess: Test = {0}\n".format(test))
        # Override the "ok" and make it a "PASS" instead
        if self.showAll:
            self.stream.writeln("PASS")
        elif self.dots:
            self.stream.write('.')
            self.stream.flush()

    def addError(self, test, err):
        super(SSTTextTestResult, self).addError(test, err)
        #log_forced("DEBUG - addError: Test = {0}, err = {1}\n".format(test, err))
        _junit_test_case = getattr(self, '_junit_test_case', None)
        if _junit_test_case is not None:
            err_msg = self._get_err_info(err)
            _junit_test_case.junit_add_error_info(err_msg)

    def addFailure(self, test, err):
        super(SSTTextTestResult, self).addFailure(test, err)
        #log_forced("DEBUG - addFailure: Test = {0}, err = {1}\n".format(test, err))
        _junit_test_case = getattr(self, '_junit_test_case', None)
        if _junit_test_case is not None:
            err_msg = self._get_err_info(err)
            _junit_test_case.junit_add_failure_info(err_msg)

    def addSkip(self, test, reason):
        super(SSTTextTestResult, self).addSkip(test, reason)
        #log_forced("DEBUG - addSkip: Test = {0}, reason = {1}\n".format(test, reason))
        _junit_test_case = getattr(self, '_junit_test_case', None)
        if _junit_test_case is not None:
            _junit_test_case.junit_add_skipped_info(reason)

    def addExpectedFailure(self, test, err):
        super(SSTTextTestResult, self).addExpectedFailure(test, err)
        #log_forced("DEBUG - addExpectedFailure: Test = {0}, err = {1}\n".format(test, err))

    def addUnexpectedSuccess(self, test):
        super(SSTTextTestResult, self).addUnexpectedSuccess(test)
        #log_forced("DEBUG - addUnexpectedSuccess: Test = {0}\n".format(test))

###

    def printErrors(self):
        if self.dots or self.showAll:
            self.stream.writeln()
        log(("===================================") +
            ("==================================="))
        log(("=== TESTS FINISHED ================") +
            ("==================================="))
        log(("===================================") +
            ("===================================\n"))
        self.printErrorList('ERROR', self.errors)
        self.printErrorList('FAIL', self.failures)

####

    def _get_err_info(self, err):
        """Converts a sys.exc_info() into a string."""
        exctype, value, tb = err
        msg_lines = traceback.format_exception_only(exctype, value)
        msg_lines = [x.replace('\n', ' ') for x in msg_lines]
        return ''.join(msg_lines)

################################################################################

################################################################################
# NOTE: The problem is that setUpClass(), setUpModule() functions are not being
#       called by the ConcurrentTestSuite.  This prevent us from tracking
#       The test name and path along with the teardown call to Junit to write
#       out the XML
################################################################################

class SSTTestSuite(test_suite_base_class):
    """A TestSuite whose run() calls out to a concurrency strategy
       but also supports the base unittest.TestSuite functionality
       Note: This is a highly modified version of testtools.ConcurrentTestSuite class
             This may be derived from testtools.ConcurrentTestSuite class, or
             if the import failed, it will be derived from unittest.TestSuite
    """

    def __init__(self, suite, make_tests, wrap_result=None):
        """Create a ConcurrentTestSuite or unittest.TestSuite to execute suite.

        :param suite: A suite to run concurrently.
        :param make_tests: A helper function to split the tests in the
            ConcurrentTestSuite into some number of concurrently executing
            sub-suites. make_tests must take a suite, and return an iterable
            of TestCase-like object, each of which must have a run(result)
            method.  NOT USED IN unittest.TestSuite
        :param wrap_result: An optional function that takes a thread-safe
            result and a thread number and must return a ``TestResult``
            object. If not provided, then ``ConcurrentTestSuite`` will just
            use a ``ThreadsafeForwardingResult`` wrapped around the result
            passed to ``run()``.  NOT USED IN unittest.TestSuite
        """
        if not test_engine_globals.TESTENGINE_CONCURRENTMODE:
            # Ignore make_tests and wrap_results
            return super(unittest.TestSuite, self).__init__(suite)
        else:
            return super(SSTTestSuite, self).__init__(suite, make_tests, wrap_result)

####

#    def run_orig(self, result):
    def run(self, result):
        """Run the tests concurrently.

        This calls out to the provided make_tests helper, and then serialises
        the results so that result only sees activity from one TestCase at
        a time.

        ConcurrentTestSuite provides no special mechanism to stop the tests
        returned by make_tests, it is up to the make_tests to honour the
        shouldStop attribute on the result object they are run with, which will
        be set if an exception is raised in the thread which
        ConcurrentTestSuite.run is called in.
        """
        # Check to verify if we are NOT in concurrent mode, if so, then
        # just call the run (this will be unittest.TestSuite's run())
        if not test_engine_globals.TESTENGINE_CONCURRENTMODE:
            return super(unittest.TestSuite, self).run(result)

        # Perform the Concurrent Run
        tests = self.make_tests(self)
        try:
            threads = {}
            queue = Queue()
            semaphore = threading.Semaphore(1)
            for i, test in enumerate(tests):
                process_result = self._wrap_result(testtools.ThreadsafeForwardingResult(result, semaphore), i)
                reader_thread = threading.Thread(target=self._run_test, args=(test, process_result, queue))
                threads[test] = reader_thread, process_result
                reader_thread.start()
            while threads:
                finished_test = queue.get()
                threads[finished_test][0].join()
                del threads[finished_test]
        except:
            for thread, process_result in threads.values():
                process_result.stop()
            raise

###

#    def _run_test_orig(self, test, process_result, queue):
    def _run_test(self, test, process_result, queue):
        try:
            try:
                testcase_name = strqual(test.__class__)
                testsuite_name = strclass(test.__class__)
                #log_forced("\nDEBUG - PRE-RUN suite={0}; case={1}; test={2}".format(testsuite_name, testcase_name, test))
                setUpConcurrentModule(test)
                test.run(process_result)
                tearDownConcurrentModule(test)
                #log_forced("\nDEBUG - PRE-RUN suite={0}; case={1}; test={2}".format(testsuite_name, testcase_name, test))
            except Exception:
                # The run logic itself failed.
                case = testtools.ErrorHolder("broken-runner", error=sys.exc_info())
                case.run(process_result)
        finally:
            queue.put(test)


####

#    def run_SUITES_SEPARATE_SUITES_RUN(self, result):
#    def run(self, result):
#        """Run the tests concurrently.
#
#        This calls out to the provided make_tests helper, and then serialises
#        the results so that result only sees activity from one TestCase at
#        a time.
#
#        ConcurrentTestSuite provides no special mechanism to stop the tests
#        returned by make_tests, it is up to the make_tests to honour the
#        shouldStop attribute on the result object they are run with, which will
#        be set if an exception is raised in the thread which
#        ConcurrentTestSuite.run is called in.
#        """
#        # Check to verify if we are NOT in concurrent mode, if so, then
#        # just call the run (this will be unittest.TestSuite's run())
#        if not test_engine_globals.TESTENGINE_CONCURRENTMODE:
#            return super(unittest.TestSuite, self).run(result)
#
#        # Perform the Concurrent Run
##        tests = self.make_tests(self)
##        log_forced("\n\nAARON - SSTTestSuite tests = {0}".format(tests))
#        testsuites = self.make_tests(self)
#        log_forced("\n\nAARON - SSTTestSuite testsuites = {0}".format(testsuites))
#        try:
#            threads = {}
#            queue = Queue()
#            semaphore = threading.Semaphore(1)
##            for i, test in enumerate(tests):
#            for i, testsuite in enumerate(testsuites):
#                # Create a thread safe `result` object for the process
#                process_result = self._wrap_result(testtools.ThreadsafeForwardingResult(result, semaphore), i)
#                # Create a thread to run the object
##                reader_thread = threading.Thread(target=self._run_test, args=(test, process_result, queue))
##                reader_thread = threading.Thread(target=self._run_test, args=(testsuite, process_result, queue))
#                reader_thread = threading.Thread(target=self._run_testsuite, args=(testsuite, process_result, queue))
#                # Add the thread and result to the dict of threads
##                log_forced("AARON ADDING TEST TO DICT = {0}".format(test))
#                log_forced("\n\nAARON - Preping name of Suite = {0}".format(testsuite))
#                testsuite_name = self.get_testsuitename(testsuite)
#                log_forced("AARON ADDING TESTSUITE TO DICT = {0}".format(testsuite_name))
##                threads[test] = reader_thread, process_result
#                threads[testsuite_name] = reader_thread, process_result
#                # Start the thread
#                reader_thread.start()
#            while threads:
#                finished_test = queue.get()
#                log_forced("AARON ADDING finished_test = {0}".format(finished_test))
#                threads[finished_test][0].join()
#                del threads[finished_test]
#        except:
#            for thread, process_result in threads.values():
#                process_result.stop()
#            raise
#
#####
#    def get_testsuitename(self, testsuite):
#        first_test = next(iter(testsuite))
#        testsuite_name = type(first_test).__name__
#        log_forced("\n\nAARON - get_testsuitename first_test = {0}".format(first_test))
#        log_forced("AARON get_testsuitename = {0}".format(testsuite_name))
#        return testsuite_name
#
#####
#    def _run_testsuite(self, testsuite, process_result, queue):
#        try:
#            try:
#                testsuite.run(process_result)
#            except Exception:
#                # The run logic itself failed.
#                case = testtools.ErrorHolder("broken-runner", error=sys.exc_info())
#                case.run(process_result)
#        finally:
#            queue.put(self.get_testsuitename(testsuite))

################################################################################

#    def run_orig(self, result):
#        """Run the tests concurrently.
#
#        This calls out to the provided make_tests helper, and then serialises
#        the results so that result only sees activity from one TestCase at
#        a time.
#
#        ConcurrentTestSuite provides no special mechanism to stop the tests
#        returned by make_tests, it is up to the make_tests to honour the
#        shouldStop attribute on the result object they are run with, which will
#        be set if an exception is raised in the thread which
#        ConcurrentTestSuite.run is called in.
#        """
#        # Check to verify if we are NOT in concurrent mode, if so, then
#        # just call the run (this will be unittest.TestSuite's run())
#        if not test_engine_globals.TESTENGINE_CONCURRENTMODE:
#            return super(unittest.TestSuite, self).run(result)
#
#        # Perform the Concurrent Run
#        tests = self.make_tests(self)
#        try:
#            threads = {}
#            queue = Queue()
#            semaphore = threading.Semaphore(1)
#            for i, test in enumerate(tests):
#                process_result = self._wrap_result(testtools.ThreadsafeForwardingResult(result, semaphore), i)
#                reader_thread = threading.Thread(target=self._run_test, args=(test, process_result, queue))
#                threads[test] = reader_thread, process_result
#                reader_thread.start()
#            while threads:
#                finished_test = queue.get()
#                threads[finished_test][0].join()
#                del threads[finished_test]
#        except:
#            for thread, process_result in threads.values():
#                process_result.stop()
#            raise

####

#    def _run_test_orig(self, test, process_result, queue):
#        try:
#            try:
#                test.run(process_result)
#            except Exception:
#                # The run logic itself failed.
#                case = testtools.ErrorHolder("broken-runner", error=sys.exc_info())
#                case.run(process_result)
#        finally:
#            queue.put(test)

################################################################################

class SSTTestFixture():
    def setUp(self):
        log_forced("DEBUG IN TestFixture.setUp()")

    def cleanUp(self):
        log_forced("DEBUG IN TestFixture.cleanUp()")
