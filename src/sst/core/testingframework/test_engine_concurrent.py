# Copyright (c) 2009-2015 testtools developers. See LICENSE for details.

import sys
import threading
import unittest
import traceback

# Queue module changes name between Py2->Py3
try:
    import Queue
except ImportError:
    import queue as Queue

def iterate_tests(test_suite_or_case):
    """Iterate through all of the test cases in 'test_suite_or_case'."""
    try:
        suite = iter(test_suite_or_case)
    except TypeError:
        yield test_suite_or_case
    else:
        for test in suite:
            for subtest in iterate_tests(test):
                yield subtest

###

class ConcurrentTestSuite(unittest.TestSuite):
    """A TestSuite whose run() calls out to a concurrency strategy."""

    def __init__(self, suite, make_tests, wrap_result=None):
        """Create a ConcurrentTestSuite to execute suite.

        :param suite: A suite to run concurrently.
        :param make_tests: A helper function to split the tests in the
            ConcurrentTestSuite into some number of concurrently executing
            sub-suites. make_tests must take a suite, and return an iterable
            of TestCase-like object, each of which must have a run(result)
            method.
        :param wrap_result: An optional function that takes a thread-safe
            result and a thread number and must return a ``TestResult``
            object. If not provided, then ``ConcurrentTestSuite`` will just
            use a ``ThreadsafeForwardingResult`` wrapped around the result
            passed to ``run()``.
        """
        super(ConcurrentTestSuite, self).__init__([suite])
        self.make_tests = make_tests
        if wrap_result:
            self._wrap_result = wrap_result

    def _wrap_result(self, thread_safe_result, thread_number):
        """Wrap a thread-safe result before sending it test results.

        You can either override this in a subclass or pass your own
        ``wrap_result`` in to the constructor.  The latter is preferred.
        """
        return thread_safe_result

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
        tests = self.make_tests(self)
        try:
            threads = {}
            queue = Queue.Queue()
            semaphore = threading.Semaphore(1)
            for i, test in enumerate(tests):
                process_result = self._wrap_result(
                    ThreadsafeForwardingResult(result, semaphore), i)
                reader_thread = threading.Thread(
                    target=self._run_test, args=(test, process_result, queue))
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

    def _run_test(self, test, process_result, queue):
        try:
            try:
                test.run(process_result)
            except Exception:
                # The run logic itself failed.
                case = ErrorHolder(
                    "broken-runner",
                    error=sys.exc_info())
                case.run(process_result)
        finally:
            queue.put(test)


class ThreadsafeForwardingResult(unittest.TestResult):
    """A TestResult which ensures the target does not receive mixed up calls.

    Multiple ``ThreadsafeForwardingResults`` can forward to the same target
    result, and that target result will only ever receive the complete set of
    events for one test at a time.

    This is enforced using a semaphore, which further guarantees that tests
    will be sent atomically even if the ``ThreadsafeForwardingResults`` are in
    different threads.

    ``ThreadsafeForwardingResult`` is typically used by
    ``ConcurrentTestSuite``, which creates one ``ThreadsafeForwardingResult``
    per thread, each of which wraps of the TestResult that
    ``ConcurrentTestSuite.run()`` is called with.

    target.startTestRun() and target.stopTestRun() are called once for each
    ThreadsafeForwardingResult that forwards to the same target. If the target
    takes special action on these events, it should take care to accommodate
    this.

    time() and tags() calls are batched to be adjacent to the test result and
    in the case of tags() are coerced into test-local scope, avoiding the
    opportunity for bugs around global state in the target.
    """

    def __init__(self, target, semaphore):
        """Create a ThreadsafeForwardingResult forwarding to target.

        :param target: A ``TestResult``.
        :param semaphore: A ``threading.Semaphore`` with limit 1.
        """
        unittest.TestResult.__init__(self)
        self.result = ExtendedToOriginalDecorator(target)
        self.semaphore = semaphore
        self._test_start = None
        self._global_tags = set(), set()
        self._test_tags = set(), set()

    def __repr__(self):
        return '<%s %r>' % (self.__class__.__name__, self.result)

    def _any_tags(self, tags):
        return bool(tags[0] or tags[1])

    def _add_result_with_semaphore(self, method, test, *args, **kwargs):
        now = self._now()
        self.semaphore.acquire()
        try:
            self.result.time(self._test_start)
            self.result.startTest(test)
            self.result.time(now)
            if self._any_tags(self._global_tags):
                self.result.tags(*self._global_tags)
            if self._any_tags(self._test_tags):
                self.result.tags(*self._test_tags)
            self._test_tags = set(), set()
            try:
                method(test, *args, **kwargs)
            finally:
                self.result.stopTest(test)
        finally:
            self.semaphore.release()
        self._test_start = None

    def addError(self, test, err=None, details=None):
        self._add_result_with_semaphore(
            self.result.addError, test, err, details=details)

    def addExpectedFailure(self, test, err=None, details=None):
        self._add_result_with_semaphore(
            self.result.addExpectedFailure, test, err, details=details)

    def addFailure(self, test, err=None, details=None):
        self._add_result_with_semaphore(
            self.result.addFailure, test, err, details=details)

    def addSkip(self, test, reason=None, details=None):
        self._add_result_with_semaphore(
            self.result.addSkip, test, reason, details=details)

    def addSuccess(self, test, details=None):
        self._add_result_with_semaphore(
            self.result.addSuccess, test, details=details)

    def addUnexpectedSuccess(self, test, details=None):
        self._add_result_with_semaphore(
            self.result.addUnexpectedSuccess, test, details=details)

    def progress(self, offset, whence):
        pass

    def startTestRun(self):
        super(ThreadsafeForwardingResult, self).startTestRun()
        self.semaphore.acquire()
        try:
            self.result.startTestRun()
        finally:
            self.semaphore.release()

    def _get_shouldStop(self):
        self.semaphore.acquire()
        try:
            return self.result.shouldStop
        finally:
            self.semaphore.release()

    def _set_shouldStop(self, value):
        # Another case where we should not subclass TestResult
        pass
    shouldStop = property(_get_shouldStop, _set_shouldStop)

    def stop(self):
        self.semaphore.acquire()
        try:
            self.result.stop()
        finally:
            self.semaphore.release()

    def stopTestRun(self):
        self.semaphore.acquire()
        try:
            self.result.stopTestRun()
        finally:
            self.semaphore.release()

    def done(self):
        self.semaphore.acquire()
        try:
            self.result.done()
        finally:
            self.semaphore.release()

    def startTest(self, test):
        self._test_start = self._now()
        super(ThreadsafeForwardingResult, self).startTest(test)

    def wasSuccessful(self):
        return self.result.wasSuccessful()

    def tags(self, new_tags, gone_tags):
        """See `TestResult`."""
        super(ThreadsafeForwardingResult, self).tags(new_tags, gone_tags)
        if self._test_start is not None:
            self._test_tags = _merge_tags(
                self._test_tags, (new_tags, gone_tags))
        else:
            self._global_tags = _merge_tags(
                self._global_tags, (new_tags, gone_tags))


class ExtendedToOriginalDecorator(object):
    """Permit new TestResult API code to degrade gracefully with old results.

    This decorates an existing TestResult and converts missing outcomes
    such as addSkip to older outcomes such as addSuccess. It also supports
    the extended details protocol. In all cases the most recent protocol
    is attempted first, and fallbacks only occur when the decorated result
    does not support the newer style of calling.
    """

    def __init__(self, decorated):
        self.decorated = decorated
        self._tags = TagContext()
        # Only used for old TestResults that do not have failfast.
        self._failfast = False
        # Used for old TestResults that do not have stop.
        self._shouldStop = False

    def __repr__(self):
        return '<%s %r>' % (self.__class__.__name__, self.decorated)

    def __getattr__(self, name):
        return getattr(self.decorated, name)

    def addError(self, test, err=None, details=None):
        try:
            self._check_args(err, details)
            if details is not None:
                try:
                    return self.decorated.addError(test, details=details)
                except TypeError:
                    # have to convert
                    err = self._details_to_exc_info(details)
            return self.decorated.addError(test, err)
        finally:
            if self.failfast:
                self.stop()

    def addExpectedFailure(self, test, err=None, details=None):
        self._check_args(err, details)
        addExpectedFailure = getattr(
            self.decorated, 'addExpectedFailure', None)
        if addExpectedFailure is None:
            return self.addSuccess(test)
        if details is not None:
            try:
                return addExpectedFailure(test, details=details)
            except TypeError:
                # have to convert
                err = self._details_to_exc_info(details)
        return addExpectedFailure(test, err)

    def addFailure(self, test, err=None, details=None):
        try:
            self._check_args(err, details)
            if details is not None:
                try:
                    return self.decorated.addFailure(test, details=details)
                except TypeError:
                    # have to convert
                    err = self._details_to_exc_info(details)
            return self.decorated.addFailure(test, err)
        finally:
            if self.failfast:
                self.stop()

    def addSkip(self, test, reason=None, details=None):
        self._check_args(reason, details)
        addSkip = getattr(self.decorated, 'addSkip', None)
        if addSkip is None:
            return self.decorated.addSuccess(test)
        if details is not None:
            try:
                return addSkip(test, details=details)
            except TypeError:
                # extract the reason if it's available
                try:
                    reason = details['reason'].as_text()
                except KeyError:
                    reason = _details_to_str(details)
        return addSkip(test, reason)

    def addUnexpectedSuccess(self, test, details=None):
        try:
            outcome = getattr(self.decorated, 'addUnexpectedSuccess', None)
            if outcome is None:
                try:
                    test.fail("")
                except test.failureException:
                    return self.addFailure(test, sys.exc_info())
            if details is not None:
                try:
                    return outcome(test, details=details)
                except TypeError:
                    pass
            return outcome(test)
        finally:
            if self.failfast:
                self.stop()

    def addSuccess(self, test, details=None):
        if details is not None:
            try:
                return self.decorated.addSuccess(test, details=details)
            except TypeError:
                pass
        return self.decorated.addSuccess(test)

    def _check_args(self, err, details):
        param_count = 0
        if err is not None:
            param_count += 1
        if details is not None:
            param_count += 1
        if param_count != 1:
            raise ValueError(
                "Must pass only one of err '%s' and details '%s"
                % (err, details))

    def _details_to_exc_info(self, details):
        """Convert a details dict to an exc_info tuple."""
        return (
            _StringException,
            _StringException(_details_to_str(details, special='traceback')),
            None)

    @property
    def current_tags(self):
        return getattr(
            self.decorated, 'current_tags', self._tags.get_current_tags())

    def done(self):
        try:
            return self.decorated.done()
        except AttributeError:
            return

    def _get_failfast(self):
        return getattr(self.decorated, 'failfast', self._failfast)

    def _set_failfast(self, value):
        if safe_hasattr(self.decorated, 'failfast'):
            self.decorated.failfast = value
        else:
            self._failfast = value
    failfast = property(_get_failfast, _set_failfast)

    def progress(self, offset, whence):
        method = getattr(self.decorated, 'progress', None)
        if method is None:
            return
        return method(offset, whence)

    def _get_shouldStop(self):
        return getattr(self.decorated, 'shouldStop', self._shouldStop)

    def _set_shouldStop(self, value):
        if safe_hasattr(self.decorated, 'shouldStop'):
            self.decorated.shouldStop = value
        else:
            self._shouldStop = value
    shouldStop = property(_get_shouldStop, _set_shouldStop)

    def startTest(self, test):
        self._tags = TagContext(self._tags)
        return self.decorated.startTest(test)

    def startTestRun(self):
        self._tags = TagContext()
        try:
            return self.decorated.startTestRun()
        except AttributeError:
            return

    def stop(self):
        method = getattr(self.decorated, 'stop', None)
        if method:
            return method()
        self.shouldStop = True

    def stopTest(self, test):
        self._tags = self._tags.parent
        return self.decorated.stopTest(test)

    def stopTestRun(self):
        try:
            return self.decorated.stopTestRun()
        except AttributeError:
            return

    def tags(self, new_tags, gone_tags):
        method = getattr(self.decorated, 'tags', None)
        if method is not None:
            return method(new_tags, gone_tags)
        else:
            self._tags.change_tags(new_tags, gone_tags)

    def time(self, a_datetime):
        method = getattr(self.decorated, 'time', None)
        if method is None:
            return
        return method(a_datetime)

    def wasSuccessful(self):
        return self.decorated.wasSuccessful()

class TagContext(object):
    """A tag context."""

    def __init__(self, parent=None):
        """Create a new TagContext.

        :param parent: If provided, uses this as the parent context.  Any tags
            that are current on the parent at the time of construction are
            current in this context.
        """
        self.parent = parent
        self._tags = set()
        if parent:
            self._tags.update(parent.get_current_tags())

    def get_current_tags(self):
        """Return any current tags."""
        return set(self._tags)

    def change_tags(self, new_tags, gone_tags):
        """Change the tags on this context.

        :param new_tags: A set of tags to add to this context.
        :param gone_tags: A set of tags to remove from this context.
        :return: The tags now current on this context.
        """
        self._tags.update(new_tags)
        self._tags.difference_update(gone_tags)
        return self.get_current_tags()



def ErrorHolder(test_id, error, short_description=None, details=None):
    """Construct an `ErrorHolder`.

    :param test_id: The id of the test.
    :param error: The exc info tuple that will be used as the test's error.
        This is inserted into the details as 'traceback' - any existing key
        will be overridden.
    :param short_description: An optional short description of the test.
    :param details: Outcome details as accepted by addSuccess etc.
    """
    return PlaceHolder(
        test_id, short_description=short_description,
        details=details, outcome='addError', error=error)


class PlaceHolder(object):
    """A placeholder test.

    `PlaceHolder` implements much of the same interface as TestCase and is
    particularly suitable for being added to TestResults.
    """

    failureException = None

    def __init__(self, test_id, short_description=None, details=None,
        outcome='addSuccess', error=None, tags=None, timestamps=(None, None)):
        """Construct a `PlaceHolder`.

        :param test_id: The id of the placeholder test.
        :param short_description: The short description of the place holder
            test. If not provided, the id will be used instead.
        :param details: Outcome details as accepted by addSuccess etc.
        :param outcome: The outcome to call. Defaults to 'addSuccess'.
        :param tags: Tags to report for the test.
        :param timestamps: A two-tuple of timestamps for the test start and
            finish. Each timestamp may be None to indicate it is not known.
        """
        self._test_id = test_id
        self._short_description = short_description
        self._details = details or {}
        self._outcome = outcome
        if error is not None:
            self._details['traceback'] = TracebackContent(error, self)
        tags = tags or frozenset()
        self._tags = frozenset(tags)
        self._timestamps = timestamps

    def __call__(self, result=None):
        return self.run(result=result)

    def __repr__(self):
        internal = [self._outcome, self._test_id, self._details]
        if self._short_description is not None:
            internal.append(self._short_description)
        return "<%s.%s(%s)>" % (
            self.__class__.__module__,
            self.__class__.__name__,
            ", ".join(map(repr, internal)))

    def __str__(self):
        return self.id()

    def countTestCases(self):
        return 1

    def debug(self):
        pass

    def id(self):
        return self._test_id

    def _result(self, result):
        if result is None:
            return TestResult()
        else:
            return ExtendedToOriginalDecorator(result)

    def run(self, result=None):
        result = self._result(result)
        if self._timestamps[0] is not None:
            result.time(self._timestamps[0])
        result.tags(self._tags, set())
        result.startTest(self)
        if self._timestamps[1] is not None:
            result.time(self._timestamps[1])
        outcome = getattr(result, self._outcome)
        outcome(self, details=self._details)
        result.stopTest(self)
        result.tags(set(), self._tags)

    def shortDescription(self):
        if self._short_description is None:
            return self.id()
        else:
            return self._short_description

class Content(object):
    """A MIME-like Content object.

    'Content' objects can be serialised to bytes using the iter_bytes method.
    If the 'Content-Type' is recognised by other code, they are welcome to
    look for richer contents that mere byte serialisation - for example in
    memory object graphs etc. However, such code MUST be prepared to receive
    a generic 'Content' object that has been reconstructed from a byte stream.

    :ivar content_type: The content type of this Content.
    """

    def __init__(self, content_type, get_bytes):
        """Create a ContentType."""
        if None in (content_type, get_bytes):
            raise ValueError("None not permitted in %r, %r" % (
                content_type, get_bytes))
        self.content_type = content_type
        self._get_bytes = get_bytes

    def __eq__(self, other):
        return (self.content_type == other.content_type and
            _join_b(self.iter_bytes()) == _join_b(other.iter_bytes()))

    def as_text(self):
        """Return all of the content as text.

        This is only valid where ``iter_text`` is.  It will load all of the
        content into memory.  Where this is a concern, use ``iter_text``
        instead.
        """
        return _u('').join(self.iter_text())

    def iter_bytes(self):
        """Iterate over bytestrings of the serialised content."""
        return self._get_bytes()

    def iter_text(self):
        """Iterate over the text of the serialised content.

        This is only valid for text MIME types, and will use ISO-8859-1 if
        no charset parameter is present in the MIME type. (This is somewhat
        arbitrary, but consistent with RFC2617 3.7.1).

        :raises ValueError: If the content type is not text/\\*.
        """
        if self.content_type.type != "text":
            raise ValueError("Not a text type %r" % self.content_type)
        return self._iter_text()

    def _iter_text(self):
        """Worker for iter_text - does the decoding."""
        encoding = self.content_type.parameters.get('charset', 'ISO-8859-1')
        decoder = codecs.getincrementaldecoder(encoding)()
        for bytes in self.iter_bytes():
            yield decoder.decode(bytes)
        final = decoder.decode(_b(''), True)
        if final:
            yield final

    def __repr__(self):
        return "<Content type=%r, value=%r>" % (
            self.content_type, _join_b(self.iter_bytes()))

class TracebackContent(Content):
    """Content object for tracebacks.

    This adapts an exc_info tuple to the 'Content' interface.
    'text/x-traceback;language=python' is used for the mime type, in order to
    provide room for other languages to format their tracebacks differently.
    """

    def __init__(self, err, test, capture_locals=False):
        """Create a TracebackContent for ``err``.

        :param err: An exc_info error tuple.
        :param test: A test object used to obtain failureException.
        :param capture_locals: If true, show locals in the traceback.
        """
        if err is None:
            raise ValueError("err may not be None")

        exctype, value, tb = err
        # Skip test runner traceback levels
        if StackLinesContent.HIDE_INTERNAL_STACK:
            while tb and '__unittest' in tb.tb_frame.f_globals:
                tb = tb.tb_next

        limit = None
        # Disabled due to https://bugs.launchpad.net/testtools/+bug/1188420
        if (False
            and StackLinesContent.HIDE_INTERNAL_STACK
            and test.failureException
            and isinstance(value, test.failureException)):
            # Skip assert*() traceback levels
            limit = 0
            while tb and not self._is_relevant_tb_level(tb):
                limit += 1
                tb = tb.tb_next

#        stack_lines = list(traceback.TracebackException(exctype, value, tb,
#            limit=limit, capture_locals=capture_locals).format())
#        content_type = ContentType('text', 'x-traceback',
#            {"language": "python", "charset": "utf8"})
#        super(TracebackContent, self).__init__(
#            content_type, lambda: [x.encode('utf8') for x in stack_lines])

class StackLinesContent(Content):
    """Content object for stack lines.

    This adapts a list of "preprocessed" stack lines into a 'Content' object.
    The stack lines are most likely produced from ``traceback.extract_stack``
    or ``traceback.extract_tb``.

    text/x-traceback;language=python is used for the mime type, in order to
    provide room for other languages to format their tracebacks differently.
    """

    # Whether or not to hide layers of the stack trace that are
    # unittest/testtools internal code.  Defaults to True since the
    # system-under-test is rarely unittest or testtools.
    HIDE_INTERNAL_STACK = True

    def __init__(self, stack_lines, prefix_content="", postfix_content=""):
        """Create a StackLinesContent for ``stack_lines``.

        :param stack_lines: A list of preprocessed stack lines, probably
            obtained by calling ``traceback.extract_stack`` or
            ``traceback.extract_tb``.
        :param prefix_content: If specified, a unicode string to prepend to the
            text content.
        :param postfix_content: If specified, a unicode string to append to the
            text content.
        """
        content_type = ContentType('text', 'x-traceback',
            {"language": "python", "charset": "utf8"})
        value = prefix_content + \
            self._stack_lines_to_unicode(stack_lines) + \
            postfix_content
        super(StackLinesContent, self).__init__(
            content_type, lambda: [value.encode("utf8")])

    def _stack_lines_to_unicode(self, stack_lines):
        """Converts a list of pre-processed stack lines into a unicode string.
        """
        msg_lines = traceback.format_list(stack_lines)
        return _u('').join(msg_lines)



