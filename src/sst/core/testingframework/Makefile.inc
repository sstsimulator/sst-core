# -*- Makefile -*-
#
#

bin_SCRIPTS = \
	testingframework/sst-test-core \
	testingframework/sst-test-elements \
	testingframework/sst_test_engine_loader.py

libexec_SCRIPTS += \
	testingframework/sst_unittest_support.py \
	testingframework/test_engine.py \
	testingframework/test_engine_globals.py \
	testingframework/test_engine_support.py \
	testingframework/test_engine_junit.py

EXTRA_DIST += \
	testingframework/sst-test-core \
	testingframework/sst-test-elements \
	testingframework/sst_test_engine_loader.py \
	testingframework/sst_unittest_support.py \
	testingframework/test_engine.py \
	testingframework/test_engine_globals.py \
	testingframework/test_engine_support.py \
	testingframework/test_engine_junit.py

