# -*- Makefile -*-
#
#

bin_SCRIPTS = \
	testingframework/sst-test-core \
	testingframework/sst-test-elements \
	testingframework/ssttestengineloader.py

libexec_SCRIPTS += \
	testingframework/test_engine.py \
	testingframework/test_support.py \
	testingframework/test_globals.py

EXTRA_DIST += \
	testingframework/sst-test-core \
	testingframework/sst-test-elements \
	testingframework/ssttestengineloader.py \
	testingframework/test_engine.py \
	testingframework/test_support.py \
	testingframework/test_globals.py

