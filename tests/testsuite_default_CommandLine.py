# -*- coding: utf-8 -*-
#
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

from sst_unittest import *
from sst_unittest_support import *

have_mpi = sst_core_config_include_file_get_value(define="SST_CONFIG_HAVE_MPI", type=int, default=0, disable_warning=True) == 1


class testcase_CommandLine(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_CommandLine_unknown_option(self):
        # An unknown option prints the usage and is an error
        outfile, errfile = self.command_line_test_template("unknown_option", "--no-such-option", expected_rc=1)
        self.assertTrue(self.file_contains(errfile, "Usage:"), "Usage not printed in {0}".format(errfile))

    def test_CommandLine_usage(self):
        # Asking for the usage prints it and is not an error
        outfile, errfile = self.command_line_test_template("usage", "--usage")
        self.assertTrue(self.file_contains(errfile, "Usage:"), "Usage not printed in {0}".format(errfile))

    def test_CommandLine_version(self):
        # Asking for the version prints it and is not an error
        outfile, errfile = self.command_line_test_template("version", "--version")
        self.assertTrue(self.file_contains(outfile, "SST-Core Version"), "Version not printed in {0}".format(outfile))

    # The same tests on 2 ranks, which check that SST exits properly from MPI when it exits after parsing the command
    # line. Otherwise mpirun returns an error even if SST does not.
    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_CommandLine_unknown_option_mpi(self):
        outfile, errfile = self.command_line_test_template(
            "unknown_option_mpi", "--no-such-option", num_ranks=2, expected_rc=1)
        self.assertTrue(self.file_contains(errfile, "Usage:"), "Usage not printed in {0}".format(errfile))

    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_CommandLine_usage_mpi(self):
        outfile, errfile = self.command_line_test_template("usage_mpi", "--usage", num_ranks=2)
        self.assertTrue(self.file_contains(errfile, "Usage:"), "Usage not printed in {0}".format(errfile))

    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_CommandLine_version_mpi(self):
        outfile, errfile = self.command_line_test_template("version_mpi", "--version", num_ranks=2)
        self.assertTrue(self.file_contains(outfile, "SST-Core Version"), "Version not printed in {0}".format(outfile))

#####

    def command_line_test_template(self, testtype, options, num_ranks=None, expected_rc=0):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        sdlfile = "{0}/test_Component.py".format(testsuitedir)
        outfile = "{0}/test_CommandLine_{1}.out".format(outdir, testtype)
        errfile = "{0}/test_CommandLine_{1}.err".format(outdir, testtype)

        self.run_sst(sdlfile, outfile, errfile, other_args=options, num_ranks=num_ranks, expected_rc=expected_rc)
        return outfile, errfile

    def file_contains(self, filename, text):
        with open(filename) as fp:
            return any(text in line for line in fp)
