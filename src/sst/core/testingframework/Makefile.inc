# -*- Makefile -*-
#
#


if !SST_TESTFRAMEWORK_DEV
# Install test frameworks by copy of files
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

else
# Install test frameworks using symbolic links to the source (we can do development)
install-exec-hook:
	ln -s $(abs_srcdir)/testingframework/sst-test-core              $(bindir)/sst-test-core
	ln -s $(abs_srcdir)/testingframework/sst-test-elements          $(bindir)/sst-test-elements
	ln -s $(abs_srcdir)/testingframework/sst_test_engine_loader.py  $(bindir)/sst_test_engine_loader.py

	ln -s $(abs_srcdir)/testingframework/sst_unittest_support.py    $(libexecdir)/sst_unittest_support.py
	ln -s $(abs_srcdir)/testingframework/test_engine.py             $(libexecdir)/test_engine.py
	ln -s $(abs_srcdir)/testingframework/test_engine_globals.py     $(libexecdir)/test_engine_globals.py
	ln -s $(abs_srcdir)/testingframework/test_engine_support.py     $(libexecdir)/test_engine_support.py
	ln -s $(abs_srcdir)/testingframework/test_engine_junit.py       $(libexecdir)/test_engine_junit.py
endif

EXTRA_DIST += \
	testingframework/sst-test-core \
	testingframework/sst-test-elements \
	testingframework/sst_test_engine_loader.py \
	testingframework/sst_unittest_support.py \
	testingframework/test_engine.py \
	testingframework/test_engine_globals.py \
	testingframework/test_engine_support.py \
	testingframework/test_engine_junit.py
