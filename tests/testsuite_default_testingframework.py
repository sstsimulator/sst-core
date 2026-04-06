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

from sst_unittest import *
from sst_unittest_support import *


class TestCategorizeDecorator(SSTTestCase):
    """Tests for the ``categorize`` decorator defined in sst_unittest_support.py."""

    def setUp(self):
        super().setUp()
        # These are the defaults for categories, with the allowed set
        # initialized in init_test_engine_globals(), recreated here as that
        # function cannot be called more than once, but we still need clean
        # category variables for each test.
        test_engine_globals.TESTENGINE_ALLOWED_TEST_CATEGORIES = set(("pr", "nightly", "weekly"))
        test_engine_globals.TESTENGINE_EXTRA_ALLOWED_TEST_CATEGORIES = set()
        test_engine_globals.TESTENGINE_CATEGORIES = set()

    _MOCK_TEST_SENTINEL = "executed"

    def _mock_test(self) -> str:
        return self._MOCK_TEST_SENTINEL

    def test_invalid_category_raises_value_error(self) -> None:
        """Providing a category not in the allowed set should raise ValueError."""
        with self.assertRaises(ValueError) as cm:
            categorize("unsupported")
        self.assertIn("Invalid test category", str(cm.exception))

    def test_extra_allowed_category_is_accepted(self) -> None:
        """Categories listed in ``TESTENGINE_EXTRA_ALLOWED_TEST_CATEGORIES`` are valid."""
        test_engine_globals.TESTENGINE_EXTRA_ALLOWED_TEST_CATEGORIES = set(["integration", "performance"])
        # No exception should be raised for an extra category
        try:
            categorize("integration")
        except ValueError:
            self.fail("categorize raised ValueError for a permitted extra category")

    def _apply_decorator(self, category: str):
        """
        Helper that decorates ``_mock_test`` with the requested category and
        returns the wrapped callable.
        """
        decorator = categorize(category)
        return decorator(self._mock_test)

    def test_wrapper_executes_when_category_matches(self) -> None:
        """When TESTENGINE_CATEGORIES contains the decorator’s category, the test runs."""
        test_engine_globals.TESTENGINE_CATEGORIES = set(["nightly"])
        wrapped = self._apply_decorator("nightly")

        self.assertFalse(getattr(wrapped, "__unittest_skip__", False))
        # The reason is always formed and set, even if the test is not
        # skipped.
        expected_why = "Test requires category 'nightly' but the current categories are '{'nightly'}'."
        self.assertEqual(getattr(wrapped, "__unittest_skip_why__", ""), expected_why)

        self.assertEqual(wrapped(), self._MOCK_TEST_SENTINEL)

    def test_wrapper_skips_when_category_mismatches(self) -> None:
        """When TESTENGINE_CATEGORIES differs, the wrapper raises SkipTest."""
        test_engine_globals.TESTENGINE_CATEGORIES = set(["pr"])
        wrapped = self._apply_decorator("nightly")

        self.assertTrue(getattr(wrapped, "__unittest_skip__", False))
        expected_why = "Test requires category 'nightly' but the current categories are '{'pr'}'."
        self.assertEqual(getattr(wrapped, "__unittest_skip_why__", ""), expected_why)

        with self.assertRaises(unittest.SkipTest) as cm:
            wrapped()
        self.assertIn(expected_why, str(cm.exception))

    def test_wrapper_skips_when_test_category_is_none(self) -> None:
        """If TESTENGINE_CATEGORIES is not set, the test is skipped."""
        wrapped = self._apply_decorator("pr")

        self.assertTrue(getattr(wrapped, "__unittest_skip__", False))
        expected_why = "Test requires category 'pr' but the current categories are 'set()'."
        self.assertEqual(getattr(wrapped, "__unittest_skip_why__", ""), expected_why)

        with self.assertRaises(unittest.SkipTest) as cm:
            wrapped()
        self.assertIn(expected_why, str(cm.exception))

    def test_extra_allowed_category_matches_and_runs(self) -> None:
        """A category listed in TESTENGINE_EXTRA_ALLOWED_TEST_CATEGORIES works like a built‑in."""
        test_engine_globals.TESTENGINE_EXTRA_ALLOWED_TEST_CATEGORIES = set(["integration"])
        test_engine_globals.TESTENGINE_CATEGORIES = set(["integration"])
        wrapped = self._apply_decorator("integration")

        self.assertFalse(getattr(wrapped, "__unittest_skip__", False))
        expected_why = "Test requires category 'integration' but the current categories are '{'integration'}'."
        self.assertEqual(getattr(wrapped, "__unittest_skip_why__", ""), expected_why)

        self.assertEqual(wrapped(), self._MOCK_TEST_SENTINEL)

    def test_extra_allowed_category_mismatch_skips(self) -> None:
        """Mismatch for an extra allowed category still triggers a skip."""
        test_engine_globals.TESTENGINE_EXTRA_ALLOWED_TEST_CATEGORIES = set(["performance"])
        wrapped = self._apply_decorator("performance")

        self.assertTrue(getattr(wrapped, "__unittest_skip__", True))
        expected_why = "Test requires category 'performance' but the current categories are 'set()'."
        self.assertEqual(getattr(wrapped, "__unittest_skip_why__", ""), expected_why)

        with self.assertRaises(unittest.SkipTest) as cm:
            wrapped()
        self.assertIn(expected_why, str(cm.exception))

    def test_wrapper_preserves_callable_signature(self) -> None:
        """The returned wrapper should be callable with the same arguments."""
        def func_with_args(a: int, b: int, *, kw: int = 0) -> int:
            return a + b + kw

        test_engine_globals.TESTENGINE_CATEGORIES = set(["weekly"])
        wrapped = categorize("weekly")(func_with_args)

        self.assertEqual(wrapped(1, 2, kw=3), 6)

        self.assertFalse(getattr(wrapped, "__unittest_skip__", False))

    @categorize("pr")
    def test_integration_pr(self) -> None:
        """Show an example of how to use the decorator for pull request tests."""

    @categorize("nightly")
    def test_integration_nightly(self) -> None:
        """Show an example of how to use the decorator for nightly tests."""

    @categorize("weekly")
    def test_integration_weekly(self) -> None:
        """Show an example of how to use the decorator for weekly tests."""
