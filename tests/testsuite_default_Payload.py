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


class testcase_Payload(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_Payload(self):
        self.payload_test_template("default")

    # Transfers between ranks with small and medium maximum MPI message sizes, so that the serialized data is split
    # into many messages, and the receive buffers have to grow. The serial and parallel (multithreaded) rank syncs are
    # both tested.
    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_Payload_r2(self):
        self.payload_test_template("r2", num_ranks=2, num_threads=1)

    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_Payload_r2_small_messages(self):
        self.payload_test_template("r2_small_messages", num_ranks=2, num_threads=1, max_message_size="64B")

    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_Payload_r2_medium_messages(self):
        self.payload_test_template("r2_medium_messages", num_ranks=2, num_threads=1, max_message_size="8KiB")

    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_Payload_r2t2_small_messages(self):
        self.payload_test_template("r2t2_small_messages", num_ranks=2, num_threads=2, max_message_size="64B")

    @unittest.skipIf(not have_mpi, "MPI is not included as part of this build")
    def test_Payload_r2t2_medium_messages(self):
        self.payload_test_template("r2t2_medium_messages", num_ranks=2, num_threads=2, max_message_size="8Ki")

#####

    def payload_test_template(self, testtype, num_ranks=None, num_threads=None, max_message_size=None):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        sdlfile = "{0}/test_Payload.py".format(testsuitedir)
        reffile = "{0}/refFiles/test_Payload.out".format(testsuitedir)
        outfile = "{0}/test_Payload_{1}.out".format(outdir, testtype)

        options = ""
        if max_message_size is not None:
            options = "--mpi-max-message-size={0}".format(max_message_size)

        self.run_sst(sdlfile, outfile, other_args=options, num_ranks=num_ranks, num_threads=num_threads)

        # The output of each component is the same regardless of the number of ranks and threads, but its order is not
        cmp_result = testing_compare_filtered_diff(testtype, outfile, reffile, sort=True)
        if not cmp_result:
            diffdata = testing_get_diff_data(testtype)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))
