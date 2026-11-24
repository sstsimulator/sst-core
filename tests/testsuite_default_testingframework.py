# Copyright 2009-2026 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2026, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import os
from typing import List, Tuple

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
        test_engine_globals.init_test_engine_categories()
        test_engine_globals.TESTENGINE_EXTRA_ALLOWED_TEST_CATEGORIES = set()

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

        self.assertEqual(wrapped(), self._MOCK_TEST_SENTINEL)

    def test_wrapper_skips_when_category_mismatches(self) -> None:
        """When TESTENGINE_CATEGORIES differs, the wrapper raises SkipTest."""
        test_engine_globals.TESTENGINE_CATEGORIES = set(["pr"])
        wrapped = self._apply_decorator("nightly")

        expected_why = "Test requires category 'nightly' but the current categories are '['pr']'."

        with self.assertRaises(unittest.SkipTest) as cm:
            wrapped()
        self.assertIn(expected_why, str(cm.exception))

    def test_default_test_category_is_pr(self) -> None:
        """When TESTENGINE_CATEGORIES is empty, still run pull request (PR) tests."""
        wrapped = self._apply_decorator("pr")

        self.assertEqual(wrapped(), self._MOCK_TEST_SENTINEL)

    def test_extra_allowed_category_matches_and_runs(self) -> None:
        """A category listed in TESTENGINE_EXTRA_ALLOWED_TEST_CATEGORIES works like a built‑in."""
        test_engine_globals.TESTENGINE_EXTRA_ALLOWED_TEST_CATEGORIES = set(["integration"])
        test_engine_globals.TESTENGINE_CATEGORIES = set(["integration"])
        wrapped = self._apply_decorator("integration")

        self.assertEqual(wrapped(), self._MOCK_TEST_SENTINEL)

    def test_extra_allowed_category_mismatch_skips(self) -> None:
        """Mismatch for an extra allowed category still triggers a skip."""
        test_engine_globals.TESTENGINE_EXTRA_ALLOWED_TEST_CATEGORIES = set(["performance"])
        wrapped = self._apply_decorator("performance")

        expected_why = "Test requires category 'performance' but the current categories are '['pr']'."

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

    @categorize("pr")
    def test_integration_pr(self) -> None:
        """Show an example of how to use the decorator for pull request tests."""

    @categorize("nightly")
    def test_integration_nightly(self) -> None:
        """Show an example of how to use the decorator for nightly tests."""

    @categorize("weekly")
    def test_integration_weekly(self) -> None:
        """Show an example of how to use the decorator for weekly tests."""


class TestLineFilter(SSTTestCase):
    """Tests for the LineFilter classes used when comparing output and reference files."""

    def _write_file(self, name: str, lines: List[str]) -> str:
        path = os.path.join(test_output_get_tmp_dir(), name)
        with open(path, "w") as fp:
            fp.writelines(lines)
        return path

    def test_base_line_filter_keeps_lines(self) -> None:
        """The base LineFilter does not change lines, so it can be used where no filtering is needed."""
        self.assertEqual(LineFilter().filter("some output\n"), "some output\n")

    def test_compare_with_base_line_filter(self) -> None:
        """A comparison using the base LineFilter still compares the lines of both files."""
        outfile = self._write_file("linefilter_out.txt", ["first\n", "second\n"])
        difffile = self._write_file("linefilter_diff.txt", ["first\n", "different\n"])
        samefile = self._write_file("linefilter_same.txt", ["second\n", "first\n"])

        self.assertFalse(testing_compare_filtered_diff("linefilter_diff", outfile, difffile, sort=True,
                                                       filters=[LineFilter()]))
        self.assertTrue(testing_compare_filtered_diff("linefilter_same", outfile, samefile, sort=True,
                                                      filters=[LineFilter()]))

    def test_compare_functions_accept_no_filters(self) -> None:
        """The filtered compare functions work with the filters left out or set to None."""
        outfile = self._write_file("nofilter_out.txt", ["first\n", "second\n"])
        samefile = self._write_file("nofilter_same.txt", ["second\n", "first\n"])
        difffile = self._write_file("nofilter_diff.txt", ["first\n", "different\n"])

        for filters in ({}, {"filters": None}):
            self.assertTrue(testing_compare_filtered_diff("nofilter_same", outfile, samefile, sort=True, **filters))
            self.assertFalse(testing_compare_filtered_diff("nofilter_diff", outfile, difffile, sort=True, **filters))
            self.assertTrue(testing_compare_filtered_diff_with_fp_tolerance("nofilter_fp_same", outfile, samefile,
                                                                            sort=True, **filters))
            self.assertFalse(testing_compare_filtered_diff_with_fp_tolerance("nofilter_fp_diff", outfile, difffile,
                                                                             sort=True, **filters))
            self.assertTrue(testing_compare_filtered_subset(outfile, samefile, **filters))
            self.assertFalse(testing_compare_filtered_subset(outfile, difffile, **filters))


class TestRunSst(SSTTestCase):
    """Tests for the checks run_sst() makes on the result of an SST run."""

    def _paths(self, name: str) -> Tuple[str, str]:
        sdlfile = os.path.join(self.get_testsuite_dir(), "test_Component.py")
        outfile = os.path.join(test_output_get_run_dir(), "testingframework_{0}.out".format(name))
        return sdlfile, outfile

    def test_run_sst_fails_on_unexpected_error(self) -> None:
        """run_sst() fails if SST returns an error when no error is expected."""
        sdlfile, outfile = self._paths("unexpected_error")
        with self.assertRaises(AssertionError):
            self.run_sst(sdlfile, outfile, other_args="--num-threads=invalid")

    def test_run_sst_accepts_expected_error(self) -> None:
        """run_sst() succeeds if SST returns an error when an error is expected."""
        sdlfile, outfile = self._paths("expected_error")
        self.run_sst(sdlfile, outfile, other_args="--num-threads=invalid", expected_rc=1)

