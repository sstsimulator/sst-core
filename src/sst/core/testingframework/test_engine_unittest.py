# -*- coding: utf-8 -*-

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

""" This module is the derived python unittest and testtools classes to support
    the requirements for SST Testing.
"""

import sys
import traceback
import threading
import time
import datetime
from typing import Any, Callable, Dict, Iterable, List, Optional, Tuple, Type, Union, TYPE_CHECKING
from unittest import TestCase, TestResult, TestSuite, TextTestResult, TextTestRunner

if TYPE_CHECKING:
    from types import TracebackType

    from sst_unittest import SSTTestCase
    from test_engine import TestEngine

if sys.version_info.minor >= 11:
    def get_current_time() -> datetime.datetime:
        return datetime.datetime.now(datetime.UTC)  # type: ignore [attr-defined]
else:
    def get_current_time() -> datetime.datetime:
        return datetime.datetime.utcnow()

################################################################################

def check_module_conditional_import(module_name: str) -> bool:
    """ Test to see if we can import a module

        See: https://stackoverflow.com/questions/14050281/how-to-check-if-a-python-module-exists-without-importing-it

        Args:
            module_name (str): Module to be imported

        Returns:
            True if module is loadable
    """
    import importlib.util
    avail = importlib.util.find_spec(module_name)
    return avail is not None

################################################################################

# See if we can import some optional modules
blessings_loaded = False
if check_module_conditional_import('blessings'):
    import blessings
    from blessings import Terminal
    blessings_loaded = True

pygments_loaded = False
if check_module_conditional_import('pygments'):
    import pygments
    from pygments import formatters, highlight
    from pygments.lexers import PythonTracebackLexer as Lexer
    pygments_loaded = True

import queue
Queue = queue.Queue

# Try to import testtools (this may not be installed on system)
if check_module_conditional_import('testtools'):
    import testtools
    from testtools.testsuite import ConcurrentTestSuite
    from testtools.testsuite import iterate_tests
    TestSuiteBaseClass = ConcurrentTestSuite
else:
    # If testtools not available, just trick the system to use TestSuite
    # This allows us to continue, but not support concurrent testing
    TestSuiteBaseClass = TestSuite

import test_engine_globals
from sst_unittest import *
from sst_unittest_support import *
from test_engine_support import strclass
from test_engine_support import strqual
from test_engine_junit import JUnitTestCase

################################################################################

def verify_concurrent_test_engine_available() -> None:
    """ Check to see if we can load testtools if the user wants to run
        in concurrent mode.

        Will generate a Fatal error if system configuration does not support
        concurrent testing.
    """
    if test_engine_globals.TESTENGINE_CONCURRENTMODE:
        if not check_module_conditional_import('testtools'):
            errmsg = ("Test Frameworks Cannot Run Concurrently - ") + \
                     ("User must perform 'pip install testtools'")
            log_fatal(errmsg)

################################################################################

class SSTTextTestRunner(TextTestRunner):
    """ A superclass to support SST required testing """

    if blessings_loaded:
        _terminal = Terminal()
        colours = {
            None: str,
            'failed': _terminal.bold_red,
            'passed': _terminal.green,
            'notes': _terminal.bold_yellow,
        }
    else:
        colours = {
            None: str
        }

    def __init__(
        self,
        stream: Any = sys.stderr,
        descriptions: bool = True,
        verbosity: int = 1,
        failfast: bool = False,
        buffer: bool = False,
        resultclass: Optional[Callable[[Any, bool, int], TextTestResult]] = None,
        no_colour_output: bool = False,
        **kwargs: Any,
    ) -> None:
        super().__init__(stream, descriptions, verbosity, failfast, buffer, resultclass)

        if not blessings_loaded or not pygments_loaded:
            log_info(("Full colorized output can be obtained by running") +
                     (" 'pip install blessings pygments'"), forced=False)

        if blessings_loaded:
            self.no_colour_output = no_colour_output
        else:
            self.no_colour_output = True

        log("\n=== TESTS STARTING " + ("=" * 51))

###

    def run(self, test: Union[TestSuite, TestCase]) -> TestResult:
        """ Run the tests."""
        testing_start_time = time.time()
        runresults = super().run(test)
        testing_stop_time = time.time()
        total_testing_time = testing_stop_time - testing_start_time
        self._get_and_display_test_results(runresults, total_testing_time)  # type: ignore [arg-type]
        return runresults

###

    def did_tests_pass(self, run_results: TestResult) -> bool:
        """ Figure out if testing passed.

            Args:
                run_results -  A TestResult object

            Returns:
                True if all tests passing with no errors, false otherwise
        """
        return run_results.wasSuccessful() and \
        len(run_results.failures) == 0 and \
        len(run_results.errors) == 0 and \
        len(run_results.unexpectedSuccesses) == 0 and \
        test_engine_globals.TESTENGINE_ERRORCOUNT == 0

###

    def _get_and_display_test_results(
        self,
        run_results: "SSTTextTestResult",
        total_testing_time: float,
    ) -> None:
        """ Figure out if testing passed, and display the test results.

            Args:
                sst_tests_results -  A TestResult object

            Returns:
                True if all tests passing with no errors, false otherwise
        """
        numpassingtests = run_results.testsRun - len(run_results.failures) \
                                               - len(run_results.skipped) \
                                               - len(run_results.errors) \
                                               - len(run_results.expectedFailures) \
                                               - len(run_results.unexpectedSuccesses)

        if not self.did_tests_pass(run_results):
            log(("\n=== TEST RESULTS BREAKDOWN ========") +
                ("==================================="))
            run_results.get_testsuites_results_dict().log_fail_error_skip_unexpeced_results()

        log(("\n=== TEST RESULTS SUMMARY ==========") +
            ("===================================\n"))
        log("Tests Run            = {0}".format(run_results.testsRun))
        log(40 * "-")
        log("Passing              = {0}".format(numpassingtests))
        log("Failures             = {0}".format(len(run_results.failures)))
        log("Skipped              = {0}".format(len(run_results.skipped)))
        log("Errors               = {0}".format(len(run_results.errors)))
        log("Expected Failures    = {0}".format(len(run_results.expectedFailures)))
        log("Unexpected Successes = {0}".format(len(run_results.unexpectedSuccesses)))
        log("Testing Notes        = {0}".format(len(test_engine_globals.TESTENGINE_TESTNOTESLIST)))
        log(("-----------------------------------") +
            ("-----------------------------------"))
        t_min, t_sec = divmod(total_testing_time, 60)
        t_hr, t_min = divmod(t_min, 60)
        log("-- Total Test Time = {0:d} Hours, {1:d} Minutes, {2:2.3f} Seconds --".format(int(t_hr), int(t_min), t_sec))

        if self.did_tests_pass(run_results):
            if self.no_colour_output:
                color_type = None
            else:
                color_type = 'passed'
            log_forced(str(self.colours[color_type]("\n====================")))
            log_forced(str(self.colours[color_type]("== TESTING PASSED ==")))
            log_forced(str(self.colours[color_type]("====================")))
        else:
            if self.no_colour_output:
                color_type = None
            else:
                color_type = 'failed'
            if test_engine_globals.TESTENGINE_ERRORCOUNT == 0:
                log_forced(str(self.colours[color_type]("\n====================")))
                log_forced(str(self.colours[color_type]("== TESTING FAILED ==")))
                log_forced(str(self.colours[color_type]("====================")))
            else:
                log_forced(str(self.colours[color_type]("\n==================================")))
                log_forced(str(self.colours[color_type]("== TESTING FAILED DUE TO ERRORS ==")))
                log_forced(str(self.colours[color_type]("==================================")))

        if test_engine_globals.TESTENGINE_TESTNOTESLIST:
            if self.no_colour_output:
                color_type = None
            else:
                color_type = 'notes'
            log_forced(str(self.colours[color_type](("\n=== TESTING NOTES =================") +
                       ("==================================="))))
            for note in test_engine_globals.TESTENGINE_TESTNOTESLIST:
                log_forced(str(self.colours[color_type](" - {0}".format(note))))

        log(("\n===================================") +
            ("===================================\n"))

################################################################################

class SSTTextTestResult(TextTestResult):
    """ A superclass to support SST required testing, this is a modified version
        of unittestTextTestResult from python 2.7 modified for SST's needs.
    """
    separator1 = '=' * 70
    separator2 = '-' * 70
    indent = ' ' * 4

    _test_class = None

    if blessings_loaded:
        _terminal = Terminal()
        colours = {
            None: str,
            'error': _terminal.bold_yellow,
            'expected': _terminal.green,
            'fail': _terminal.bold_red,
            'skip': _terminal.bold_blue,
            'success': _terminal.green,
            'title': _terminal.magenta,
            'unexpected': _terminal.bold_red,
        }
    else:
        colours = {
            None: str
        }

    if pygments_loaded:
        formatter = formatters.Terminal256Formatter()
        lexer = Lexer()


    def __init__(
        self,
        stream: Any,
        descriptions: bool,
        verbosity: int,
        no_colour_output: bool = False,
    ) -> None:
        super().__init__(stream, descriptions, verbosity)
        self.testsuitesresultsdict = SSTTestSuitesResultsDict()
        self._test_name = "undefined_testname"
        self._testcase_name = "undefined_testcasename"
        self._testsuite_name = "undefined_testsuitename"
        self._junit_test_case: JUnitTestCase = None  # type: ignore [assignment]
        self.stream = stream
        self.showAll = verbosity > 1
        self.dots = verbosity == 1
        self.descriptions = descriptions
        if blessings_loaded:
            self.no_colour_output = no_colour_output
        else:
            self.no_colour_output = True

    def getShortDescription(self, test: TestCase) -> str:
        doc_first_line = test.shortDescription()
        if self.descriptions and doc_first_line:
            return '\n'.join((str(test), doc_first_line))
        else:
            return str(test)

    def getLongDescription(self, test: TestCase) -> str:
        doc_first_line = test.shortDescription()
        if self.descriptions and doc_first_line:
            return '\n'.join((str(test), doc_first_line))
        return str(test)

    def getClassDescription(self, test: TestCase) -> str:
        test_class = test.__class__
        doc = test_class.__doc__
        if self.descriptions and doc:
            return doc.strip().split('\n')[0].strip()
        return strclass(test_class)

###

    def startTest(self, test: TestCase) -> None:
        super().startTest(test)
        if self.showAll:
            if not test_engine_globals.TESTENGINE_CONCURRENTMODE:
                if self._test_class != test.__class__:
                    self._test_class = test.__class__
                    title = self.getClassDescription(test)
                    if self.no_colour_output:
                        self.stream.writeln(self.colours[None](title))
                    else:
                        self.stream.writeln(self.colours['title'](title))
            self.stream.flush()

        if self._is_test_of_type_ssttestcase(test):
            self._test_name = test.testname  # type: ignore [attr-defined]
            self._testcase_name = test.get_testcase_name()  # type: ignore [attr-defined]
            self._testsuite_name = test.get_testsuite_name()  # type: ignore [attr-defined]
        else:
            self._test_name = "undefined_testname"
            self._testcase_name = "FailedTest"
            self._testsuite_name = "FailedTest"
        timestamp = get_current_time().strftime("%Y_%m%d_%H:%M:%S.%f utc")
        self._junit_test_case = JUnitTestCase(self._test_name,
                                              self._testcase_name,
                                              timestamp=timestamp)

    def stopTest(self, test: TestCase) -> None:
        super().stopTest(test)
        testruntime = 0.0
        if self._is_test_of_type_ssttestcase(test):
            testruntime = test.get_test_runtime_sec()  # type: ignore [attr-defined]
        self._junit_test_case.junit_add_elapsed_sec(testruntime)

        if not self._is_test_of_type_ssttestcase(test):
            return

        if not test_engine_globals.TESTENGINE_CONCURRENTMODE:
            test_engine_globals.TESTRUN_JUNIT_TESTCASE_DICTLISTS['singlethread'].\
            append(self._junit_test_case)
        else:
            test_engine_globals.TESTRUN_JUNIT_TESTCASE_DICTLISTS[self._testsuite_name].\
            append(self._junit_test_case)

###

    def get_testsuites_results_dict(self) -> "SSTTestSuitesResultsDict":
        """ Return the test suites results dict """
        return self.testsuitesresultsdict

###

    def printResult(
        self,
        test: TestCase,
        short: str,
        extended: str,
        colour_key: Optional[str] = None,
        showruntime: bool = True,
    ) -> None:
        if self.no_colour_output:
            colour = self.colours[None]
        else:
            colour = self.colours[colour_key]
        if self.showAll:
            self.stream.write(self.indent)
            self.stream.write(colour(extended))
            self.stream.write(" -- ")
            self.stream.write(self.getShortDescription(test))
            testruntime = 0.0
            if self._is_test_of_type_ssttestcase(test):
                testruntime = test.get_test_runtime_sec()  # type: ignore [attr-defined]
            if showruntime:
                self.stream.writeln(" [{0:.3f}s]".format(testruntime))
            else:
                self.stream.writeln(" ")
            self.stream.flush()
        elif self.dots:
            self.stream.write(colour(short))
            self.stream.flush()

###

    def addSuccess(self, test: TestCase) -> None:
        super().addSuccess(test)
        self.printResult(test, '.', 'PASS', 'success')

        if not self._is_test_of_type_ssttestcase(test):
            return
        self.testsuitesresultsdict.add_success(test)

    def addError(self, test: TestCase, err: "ErrorType") -> None:
        super().addError(test, err)
        self.printResult(test, 'E', 'ERROR', 'error')

        if not self._is_test_of_type_ssttestcase(test):
            return
        self.testsuitesresultsdict.add_error(test)
        _junit_test_case = getattr(self, '_junit_test_case', None)
        if _junit_test_case is not None:
            err_msg = self._get_err_info(err)
            _junit_test_case.junit_add_error_info(err_msg)

    def addFailure(self, test: TestCase, err: "ErrorType") -> None:
        super().addFailure(test, err)
        self.printResult(test, 'F', 'FAIL', 'fail')

        if not self._is_test_of_type_ssttestcase(test):
            return
        self.testsuitesresultsdict.add_failure(test)
        _junit_test_case = getattr(self, '_junit_test_case', None)
        if _junit_test_case is not None:
            err_msg = self._get_err_info(err)
            _junit_test_case.junit_add_failure_info(err_msg)

    def addSkip(self, test: TestCase, reason: str) -> None:
        super().addSkip(test, reason)
        if not test_engine_globals.TESTENGINE_IGNORESKIPS:
            self.printResult(test, 's', 'SKIPPED', 'skip', showruntime=False)

        if not self._is_test_of_type_ssttestcase(test):
            return
        self.testsuitesresultsdict.add_skip(test)
        _junit_test_case = getattr(self, '_junit_test_case', None)
        if _junit_test_case is not None:
            _junit_test_case.junit_add_skipped_info(reason)

    def addExpectedFailure(self, test: TestCase, err: "ErrorType") -> None:
        # NOTE: This is not a failure, but an identified pass
        #       since we are expecting a failure
        super().addExpectedFailure(test, err)
        self.printResult(test, 'x', 'EXPECTED FAILURE', 'expected')
        if not self._is_test_of_type_ssttestcase(test):
            return
        self.testsuitesresultsdict.add_expected_failure(test)

    def addUnexpectedSuccess(self, test: TestCase) -> None:
        # NOTE: This is a failure, since we passed, but were expecting a failure
        super().addUnexpectedSuccess(test)
        self.printResult(test, 'u', 'UNEXPECTED SUCCESS', 'unexpected')

        if not self._is_test_of_type_ssttestcase(test):
            return
        self.testsuitesresultsdict.add_unexpected_success(test)
        _junit_test_case = getattr(self, '_junit_test_case', None)
        if _junit_test_case is not None:
            _junit_test_case.junit_add_failure_info("RECEIVED SUCCESS WHEN EXPECTING A FAILURE")

###

    def printErrors(self) -> None:
        if self.dots or self.showAll:
            self.stream.writeln()
        log("=" * 70)
        log("=== TESTS FINISHED " + ("=" * 51))
        log("=" * 70 + "\n")
        if not test_engine_globals.TESTENGINE_IGNORESKIPS:
            self.printSkipList('SKIPPED', self.skipped)
        self.printErrorList('ERROR', self.errors)
        self.printErrorList('FAIL', self.failures)

    def printErrorList(self, flavour: str, errors: "ErrorsType") -> None:
        if self.no_colour_output:
            colour = self.colours[None]
        else:
            colour = self.colours[flavour.lower()]

        for test, err in errors:
            self.stream.writeln(self.separator1)
            title = '%s: %s' % (flavour, self.getLongDescription(test))
            self.stream.writeln(colour(title))
            self.stream.writeln(self.separator2)
            if pygments_loaded:
                self.stream.writeln(highlight(err, self.lexer, self.formatter))
            else:
                self.stream.writeln(err)

    def printSkipList(self, flavour: str, errors: "ErrorsType") -> None:
        if self.no_colour_output:
            colour = self.colours[None]
        else:
            colour = self.colours["skip"]

        for test, err in errors:
            title = '%s: %s' % (flavour, self.getLongDescription(test))
            self.stream.writeln(colour(title))
            if pygments_loaded:
                self.stream.writeln(highlight(err, self.lexer, self.formatter))
            else:
                self.stream.writeln(err)

####

    def _get_err_info(self, err: "ErrorType") -> str:
        """Converts a sys.exc_info() into a string."""
        exctype, value, tback = err
        msg_lines = traceback.format_exception_only(exctype, value)
        msg_lines = [x.replace('\n', ' ') for x in msg_lines]
        return ''.join(msg_lines)

####

    def _is_test_of_type_ssttestcase(self, test: TestCase) -> bool:
        """Determine if this is within a valid SSTTestCase object."""
        # This originally checked for a special method on the instance,
        # but the type check is faster.
        return isinstance(test, SSTTestCase)

################################################################################

# TestSuiteBaseClass will be either unitest.TestSuite or testtools.ConcurrentTestSuite
# and is defined at the top of this file.
class SSTTestSuite(TestSuiteBaseClass):  # type: ignore [misc, valid-type]
    """A TestSuite whose run() method can execute tests concurrently,
       but also supports the Python base TestSuite functionality.

       This is a highly modified version of testtools.ConcurrentTestSuite
       class to support startUpModuleConcurrent() & tearDownModuleConcurrent()
       and to also support the limiting of parallel threads in flight.

       This object will normally be derived from testtools.ConcurrentTestSuite
       class, however, if the import of testtools failed, it will be derived from
       TestSuite.

       If the user selected concurrent mode is false, then it will always make
       calls to the TestSuite class EVEN IF it is derived from
       testtools.ConcurrentTestSuite, which is itself derived from TestSuite.
    """

    def __init__(
        self,
        suite: TestSuite,
        make_tests: Callable[["SSTTestSuite"], List[Any]],
        wrap_result: Optional[
            Callable[["testtools.ThreadsafeForwardingResults", int], TestResult]
        ] = None,
    ) -> None:
        """Create a ConcurrentTestSuite or TestSuite to execute the suite.

        Note: If concurrent mode is false, then it will always make calls to the
              TestSuite class EVEN IF the class is derived from
              testtools.ConcurrentTestSuite.

        Args:
            suite: A suite to run concurrently.
            make_tests: A helper function to split the tests in the
                        ConcurrentTestSuite into some number of concurrently executing
                        sub-suites. make_tests must take a suite, and return an iterable
                        of TestCase-like object, each of which must have a run(result)
                        method.  NOT USED IN TestSuite.
            wrap_result: An optional function that takes a thread-safe
                         result and a thread number and must return a ``TestResult``
                         object. If not provided, then ``ConcurrentTestSuite`` will just
                         use a ``ThreadsafeForwardingResult`` wrapped around the result
                         passed to ``run()``.  NOT USED IN TestSuite

        """
        # This separation is required for the case when `testtools` is not
        # installed, regardless of whether or not testing is performed
        # concurrently.
        if not test_engine_globals.TESTENGINE_CONCURRENTMODE:
            # Ignore make_tests and wrap_results
            super().__init__(suite)
        else:
            super().__init__(suite, make_tests, wrap_result)

####

    def run(self, result: "SSTTextTestResult") -> Optional["SSTTextTestResult"]:
        """Run the tests (possibly concurrently).

            This calls out to the provided make_tests helper, and then serialises
            the results so that result only sees activity from one TestCase at
            a time.

            ConcurrentTestSuite provides no special mechanism to stop the tests
            returned by make_tests, it is up to the make_tests to honour the
            shouldStop attribute on the result object they are run with, which will
            be set if an exception is raised in the thread which
            ConcurrentTestSuite.run() is called in.

            NOTE: This is a highly modified version of the
                  testtools.ConcurrentTestSuite.run() method.  It was changed to
                  support running a limited number of concurrent threads.

            If concurrent mode is false, then it will always make calls to the
            TestSuite class EVEN IF it is derived from
            testtools.ConcurrentTestSuite.
        """
        # Check to verify if we are NOT in concurrent mode, if so, then
        # just call the run (this will be TestSuite's run())
        if not test_engine_globals.TESTENGINE_CONCURRENTMODE:
            return super(TestSuite, self).run(result)  # type: ignore [misc, no-any-return]

        # Perform the Concurrent Run
        tests = self.make_tests(self)
        thread_limit = test_engine_globals.TESTENGINE_THREADLIMIT
        test_index = -1
        try:
            threads: Dict[TestCase, Tuple[threading.Thread, Any]] = {}
            testqueue: Queue[TestCase] = Queue()
            semaphore = threading.Semaphore(1)
            test_iter = iter(tests)
            test: Optional[SSTTestCase] = "startup_placeholder"  # type: ignore [assignment]
            tests_finished = False
            while not tests_finished:
                while len(threads) < thread_limit and test is not None:
                    test = next(test_iter, None)
                    if result.shouldStop:
                        tests_finished = True
                    test_index += 1
                    if test is not None:
                        process_result = self._wrap_result(testtools.ThreadsafeForwardingResult(result, semaphore), test_index)
                        reader_thread = threading.Thread(target=self._run_test, args=(test, process_result, testqueue))
                        threads[test] = reader_thread, process_result
                        reader_thread.start()
                if threads:
                    finished_test = testqueue.get()
                    threads[finished_test][0].join()
                    del threads[finished_test]
                else:
                    tests_finished = True
            test_engine_globals.TESTRUN_TESTRUNNINGFLAG = False

        except:
            for thread, process_result in threads.values():
                process_result.stop()
            raise

        return None

###

    def _run_test(
        self,
        test: SSTTestCase,
        process_result: "testtools.testresult.real.ThreadsafeForwardingResult",
        testqueue: "Queue[TestCase]",
    ) -> None:
        """Support running a single test concurrently

        NOTE: This is a slightly modified version of the
              testtools.ConcurrentTestSuite._run_test() method.  It was changed
              to support running a limited number of calling the functions
              setUpModuleConcurrent() and tearDownModuleConcurrent()
        """
        try:
            try:
                setUpModuleConcurrent(test)
                test_engine_globals.TESTRUN_TESTRUNNINGFLAG = True
                test.run(process_result)
                tearDownModuleConcurrent(test)
            except Exception:
                # The run logic itself failed.
                testcase = testtools.ErrorHolder("broken-runner", error=sys.exc_info())
                testcase.run(process_result)
        finally:
            testqueue.put(test)

################################################################################

class SSTTestSuiteResultData:
    """ Support class to hold result data for a specific testsuite
        Results are stored as lists of test names
    """
    def __init__(self) -> None:
        self._tests_passing: List[TestCase] = []
        self._tests_failing: List[TestCase] = []
        self._tests_errored: List[TestCase] = []
        self._tests_skipped: List[TestCase] = []
        self._tests_expectedfailed: List[TestCase] = []
        self._tests_unexpectedsuccess: List[TestCase] = []

    def add_success(self, test: TestCase) -> None:
        """ Add a test to the success record"""
        self._tests_passing.append(test)

    def add_failure(self, test: TestCase) -> None:
        """ Add a test to the failure record"""
        self._tests_failing.append(test)

    def add_error(self, test: TestCase) -> None:
        """ Add a test to the error record"""
        self._tests_errored.append(test)

    def add_skip(self, test: TestCase) -> None:
        """ Add a test to the skip record"""
        self._tests_skipped.append(test)

    def add_expected_failure(self, test: TestCase) -> None:
        """ Add a test to the expected failure record"""
        self._tests_expectedfailed.append(test)

    def add_unexpected_success(self, test: TestCase) -> None:
        """ Add a test to the unexpected success record"""
        self._tests_unexpectedsuccess.append(test)

    def get_passing(self) -> List[TestCase]:
        """ Return the tests passing list"""
        return self._tests_passing

    def get_failed(self) -> List[TestCase]:
        """ Return the tests failed list"""
        return self._tests_failing

    def get_errored(self) -> List[TestCase]:
        """ Return the tests errored list"""
        return self._tests_errored

    def get_skipped(self) -> List[TestCase]:
        """ Return the tests skipped list"""
        return self._tests_skipped

    def get_expectedfailed(self) -> List[TestCase]:
        """ Return the expected failed list"""
        return self._tests_expectedfailed

    def get_unexpectedsuccess(self) -> List[TestCase]:
        """ Return the tests unexpected success list"""
        return self._tests_unexpectedsuccess

###

class SSTTestSuitesResultsDict:
    """ Support class handle of dict of result data for all testsuites
    """
    def __init__(self) -> None:
        self.testsuitesresultsdict: Dict[str, SSTTestSuiteResultData] = {}

    def add_success(self, test: TestCase) -> None:
        """ Add a testsuite and test to the success record"""
        self._get_testresult_from_testmodulecase(test).add_success(test)

    def add_failure(self, test: TestCase) -> None:
        """ Add a testsuite and test to the failure record"""
        self._get_testresult_from_testmodulecase(test).add_failure(test)

    def add_error(self, test: TestCase) -> None:
        """ Add a testsuite and test to the error record"""
        self._get_testresult_from_testmodulecase(test).add_error(test)

    def add_skip(self, test: TestCase) -> None:
        """ Add a testsuite and test to the skip record"""
        self._get_testresult_from_testmodulecase(test).add_skip(test)

    def add_expected_failure(self, test: TestCase) -> None:
        """ Add a testsuite and test to the expected failure record"""
        self._get_testresult_from_testmodulecase(test).add_expected_failure(test)

    def add_unexpected_success(self, test: TestCase) -> None:
        """ Add a testsuite and test to the unexpected success record"""
        self._get_testresult_from_testmodulecase(test).add_unexpected_success(test)

    def log_all_results(self) -> None:
        """ Log all result catagories by testsuite  """
        # Log the data by key
        for tmtc_name in self.testsuitesresultsdict:
            log("\n{0}".format(tmtc_name))
            for testname in self.testsuitesresultsdict[tmtc_name].get_passing():
                log(" - PASSED  : {0}".format(testname))
            for testname in self.testsuitesresultsdict[tmtc_name].get_failed():
                log(" - FAILED  : {0}".format(testname))
            for testname in self.testsuitesresultsdict[tmtc_name].get_errored():
                log(" - ERROR   : {0}".format(testname))
            for testname in self.testsuitesresultsdict[tmtc_name].get_skipped():
                log(" - SKIPPED : {0}".format(testname))
            for testname in self.testsuitesresultsdict[tmtc_name].get_expectedfailed():
                log(" - EXPECTED FAILED    : {0}".format(testname))
            for testname in self.testsuitesresultsdict[tmtc_name].get_unexpectedsuccess():
                log(" - UNEXPECTED SUCCESS : {0}".format(testname))

    def log_fail_error_skip_unexpeced_results(self) -> None:
        """ Log non-success result catagories by testsuite  """
        # Log the data by key
        for tmtc_name in self.testsuitesresultsdict:
            # Dont log if everything passes
            if len(self.testsuitesresultsdict[tmtc_name].get_failed()) == 0 and \
               len(self.testsuitesresultsdict[tmtc_name].get_errored()) == 0 and \
               len(self.testsuitesresultsdict[tmtc_name].get_expectedfailed()) == 0 and \
               len(self.testsuitesresultsdict[tmtc_name].get_unexpectedsuccess()) == 0:
                pass
            else:
                log("\n{0}".format(tmtc_name))
                for testname in self.testsuitesresultsdict[tmtc_name].get_failed():
                    log(" - FAILED  : {0}".format(testname))
                for testname in self.testsuitesresultsdict[tmtc_name].get_errored():
                    log(" - ERROR   : {0}".format(testname))
                for testname in self.testsuitesresultsdict[tmtc_name].get_expectedfailed():
                    log(" - EXPECTED FAILED    : {0}".format(testname))
                for testname in self.testsuitesresultsdict[tmtc_name].get_unexpectedsuccess():
                    log(" - UNEXPECTED SUCCESS : {0}".format(testname))

    def _get_testresult_from_testmodulecase(self, test: TestCase) -> SSTTestSuiteResultData:
        tm_tc = self._get_test_module_test_case_name(test)
        if tm_tc not in self.testsuitesresultsdict.keys():
            self.testsuitesresultsdict[tm_tc] = SSTTestSuiteResultData()
        return self.testsuitesresultsdict[tm_tc]

    def _get_test_module_test_case_name(self, test: TestCase) -> str:
        return "{0}.{1}".format(self._get_test_module_name(test),
                                self._get_test_case_name(test))

    def _get_test_case_name(self, test: TestCase) -> str:
        return strqual(test.__class__)

    def _get_test_module_name(self, test: TestCase) -> str:
        return strclass(test.__class__)


ErrorType = Union[
    Tuple[Type[BaseException], BaseException, "TracebackType"],
    Tuple[None, None, None]
]
ErrorsType = Iterable[Tuple[TestCase, str]]
