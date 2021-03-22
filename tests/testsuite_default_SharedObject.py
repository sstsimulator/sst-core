# -*- coding: utf-8 -*-

import os
import sys

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

class testcase_SharedObject(SSTTestCase):

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

    # SharedArray Tests
    # Full Initialization
    #   single - only ID 0 initializes array
    #     init_verify
    #     fe_verify
    #     no_verify
    #   multi - also has ID N-1 initialize array
    #     no conflict - both IDs write same data
    #     conflict - IDs write different data
    #       init_verify
    #       fe_verify
    #       no_verify
    # Partial initialize (everyone initializes their own portion)
    #   late_write = true
    #   do not publish = true
    #   nopub and late_write true
    #   double_initialize
    # Late_initialize

    def test_SharedObject_array_full_single_init(self):
        self.sharedobject_test_template("array_full_single_init", 0, "--param=object_type:array --param=num_entities:12 --param=full_initialization:true")

    def test_SharedObject_array_full_single_fe(self):
        self.sharedobject_test_template("array_full_single_fe", 0, "--param=object_type:array --param=num_entities:12 --param=full_initialization:true --param=verify_mode:FE")

    def test_SharedObject_array_full_single_none(self):
        self.sharedobject_test_template("array_full_single_none", 0, "--param=object_type:array --param=num_entities:12 --param=full_initialization:true --param=verify_mode:NONE")

    def test_SharedObject_array_full_multi(self):
        self.sharedobject_test_template("array_full_multi", 0, "--param=object_type:array --param=num_entities:12 --param=full_initialization:true --param=multiple_initializers:true")

    def test_SharedObject_array_full_multi_conflict_init(self):
        self.sharedobject_test_template("array_full_multi_conflict_init", 1, "--param=object_type:array --param=num_entities:12 --param=full_initialization:true --param=multiple_initializers:true --param=conflicting_write:true")

    def test_SharedObject_array_full_multi_conflict_fe(self):
        self.sharedobject_test_template("array_full_multi_conflict_fe", 1, "--param=object_type:array --param=num_entities:12 --param=full_initialization:true --param=multiple_initializers:true --param=conflicting_write:true --param=verify_mode:FE")

    def test_SharedObject_array_full_multi_conflict_none(self):
        self.sharedobject_test_template("array_full_multi_conflict_none", 0, "--param=object_type:array --param=num_entities:12 --param=full_initialization:true --param=multiple_initializers:true --param=conflicting_write:true --param=verify_mode:NONE")

    def test_SharedObject_array_partial(self):
        self.sharedobject_test_template("array_partial", 0, "--param=object_type:array --param=num_entities:12 --param=full_initialization:false")

    def test_SharedObject_array_partial_nopub(self):
        self.sharedobject_test_template("array_partial_nopub", 0, "--param=object_type:array --param=num_entities:12 --param=full_initialization:false --param=publish:false")

    def test_SharedObject_array_partial_late(self):
        self.sharedobject_test_template("array_partial_late", 1, "--param=object_type:array --param=num_entities:12 --param=full_initialization:false --param=late_write:true")

    def test_SharedObject_array_partial_late_nopub(self):
        self.sharedobject_test_template("array_partial_late_nopub", 1, "--param=object_type:array --param=num_entities:12 --param=full_initialization:false --param=late_write:true --param=publish:false")

    def test_SharedObject_array_partial_doubleinit(self):
        self.sharedobject_test_template("array_partial_doubleinit", 1, "--param=object_type:array --param=num_entities:12 --param=full_initialization:false --param=double_initialize:true")

    def test_SharedObject_array_late_initialize(self):
        self.sharedobject_test_template("array_late_initialize", 1, "--param=object_type:array --param=num_entities:12 --param=late_initialize:true")


    # SharedMap Tests
    # Full Initialization
    #   single - only ID 0 initializes map
    #     init_verify
    #     fe_verify
    #     no_verify
    #   multi - also has ID N-1 initialize map
    #     no conflict - both IDs write same data
    #     conflict - IDs write different data
    #       init_verify
    #       fe_verify
    #       no_verify
    # Partial initialize (everyone initializes their own portion)
    #   late_write = true
    #   do not publish = true
    #   nopub and late_write true
    #   double_initialize
    # Late_initialize

    def test_SharedObject_map_full_single_init(self):
        self.sharedobject_test_template("map_full_single_init", 0, "--param=object_type:map --param=num_entities:12 --param=full_initialization:true")

    def test_SharedObject_map_full_single_fe(self):
        self.sharedobject_test_template("map_full_single_fe", 0, "--param=object_type:map --param=num_entities:12 --param=full_initialization:true --param=verify_mode:FE")

    def test_SharedObject_map_full_single_none(self):
        self.sharedobject_test_template("map_full_single_none", 0, "--param=object_type:map --param=num_entities:12 --param=full_initialization:true --param=verify_mode:NONE")

    def test_SharedObject_map_full_multi(self):
        self.sharedobject_test_template("map_full_multi", 0, "--param=object_type:map --param=num_entities:12 --param=full_initialization:true --param=multiple_initializers:true")

    def test_SharedObject_map_full_multi_conflict_init(self):
        self.sharedobject_test_template("map_full_multi_conflict_init", 1, "--param=object_type:map --param=num_entities:12 --param=full_initialization:true --param=multiple_initializers:true --param=conflicting_write:true")

    def test_SharedObject_map_full_multi_conflict_fe(self):
        self.sharedobject_test_template("map_full_multi_conflict_fe", 1, "--param=object_type:map --param=num_entities:12 --param=full_initialization:true --param=multiple_initializers:true --param=conflicting_write:true --param=verify_mode:FE")

    def test_SharedObject_map_full_multi_conflict_none(self):
        self.sharedobject_test_template("map_full_multi_conflict_none", 0, "--param=object_type:map --param=num_entities:12 --param=full_initialization:true --param=multiple_initializers:true --param=conflicting_write:true --param=verify_mode:NONE")

    def test_SharedObject_map_partial(self):
        self.sharedobject_test_template("map_partial", 0, "--param=object_type:map --param=num_entities:12 --param=full_initialization:false")

    def test_SharedObject_map_partial_nopub(self):
        self.sharedobject_test_template("map_partial_nopub", 0, "--param=object_type:map --param=num_entities:12 --param=full_initialization:false --param=publish:false")

    def test_SharedObject_map_partial_late(self):
        self.sharedobject_test_template("map_partial_late", 1, "--param=object_type:map --param=num_entities:12 --param=full_initialization:false --param=late_write:true")

    def test_SharedObject_map_partial_late_nopub(self):
        self.sharedobject_test_template("map_partial_late_nopub", 1, "--param=object_type:map --param=num_entities:12 --param=full_initialization:false --param=late_write:true --param=publish:false")

    def test_SharedObject_map_partial_doubleinit(self):
        self.sharedobject_test_template("map_partial_doubleinit", 1, "--param=object_type:map --param=num_entities:12 --param=full_initialization:false --param=double_initialize:true")

    def test_SharedObject_map_late_initialize(self):
        self.sharedobject_test_template("map_late_initialize", 1, "--param=object_type:map --param=num_entities:12 --param=late_initialize:true")

    # SharedSet Tests
    # Full Initialization
    #   single - only ID 0 initializes set
    #     init_verify
    #     fe_verify
    #     no_verify
    #   multi - also has ID N-1 initialize set
    #     no conflict - both IDs write same data
    #     conflict - IDs write different data
    #       init_verify
    #       fe_verify
    #       no_verify
    # Partial initialize (everyone initializes their own portion)
    #   late_write = true
    #   do not publish = true
    #   nopub and late_write true
    #   double_initialize
    # Late_initialize

    def test_SharedObject_set_full_single_init(self):
        self.sharedobject_test_template("set_full_single_init", 0, "--param=object_type:set --param=num_entities:12 --param=full_initialization:true")

    def test_SharedObject_set_full_single_fe(self):
        self.sharedobject_test_template("set_full_single_fe", 0, "--param=object_type:set --param=num_entities:12 --param=full_initialization:true --param=verify_mode:FE")

    def test_SharedObject_set_full_single_none(self):
        self.sharedobject_test_template("set_full_single_none", 0, "--param=object_type:set --param=num_entities:12 --param=full_initialization:true --param=verify_mode:NONE")

    def test_SharedObject_set_full_multi(self):
        self.sharedobject_test_template("set_full_multi", 0, "--param=object_type:set --param=num_entities:12 --param=full_initialization:true --param=multiple_initializers:true")

    def test_SharedObject_set_full_multi_conflict_init(self):
        self.sharedobject_test_template("set_full_multi_conflict_init", 1, "--param=object_type:set --param=num_entities:12 --param=full_initialization:true --param=multiple_initializers:true --param=conflicting_write:true")

    def test_SharedObject_set_full_multi_conflict_fe(self):
        self.sharedobject_test_template("set_full_multi_conflict_fe", 1, "--param=object_type:set --param=num_entities:12 --param=full_initialization:true --param=multiple_initializers:true --param=conflicting_write:true --param=verify_mode:FE")

    def test_SharedObject_set_full_multi_conflict_none(self):
        self.sharedobject_test_template("set_full_multi_conflict_none", 0, "--param=object_type:set --param=num_entities:12 --param=full_initialization:true --param=multiple_initializers:true --param=conflicting_write:true --param=verify_mode:NONE")

    def test_SharedObject_set_partial(self):
        self.sharedobject_test_template("set_partial", 0, "--param=object_type:set --param=num_entities:12 --param=full_initialization:false")

    def test_SharedObject_set_partial_nopub(self):
        self.sharedobject_test_template("set_partial_nopub", 0, "--param=object_type:set --param=num_entities:12 --param=full_initialization:false --param=publish:false")

    def test_SharedObject_set_partial_late(self):
        self.sharedobject_test_template("set_partial_late", 1, "--param=object_type:set --param=num_entities:12 --param=full_initialization:false --param=late_write:true")

    def test_SharedObject_set_partial_late_nopub(self):
        self.sharedobject_test_template("set_partial_late_nopub", 1, "--param=object_type:set --param=num_entities:12 --param=full_initialization:false --param=late_write:true --param=publish:false")

    def test_SharedObject_set_partial_doubleinit(self):
        self.sharedobject_test_template("set_partial_doubleinit", 1, "--param=object_type:set --param=num_entities:12 --param=full_initialization:false --param=double_initialize:true")

    def test_SharedObject_set_late_initialize(self):
        self.sharedobject_test_template("set_late_initialize", 1, "--param=object_type:set --param=num_entities:12 --param=late_initialize:true")

#####

    def sharedobject_test_template(self, testtype, exp_rc, options):
        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        model_options = '--model-options="{0}"'.format(options)

        # Set the various file paths
        sdlfile = "{0}/test_SharedObject.py".format(testsuitedir)
        #reffile = "{0}/sharedobject_tests/refFiles/test_{1}.out".format(testsuitedir, testtype)
        outfile = "{0}/test_SharedObject_{1}.out".format(outdir, testtype)

        self.run_sst(sdlfile, outfile, other_args=model_options, expected_rc = exp_rc)

        # No need to perform test since we're just looking for it to
        # complete without an error
