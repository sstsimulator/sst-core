# -*- Makefile -*-
#
#


if !SST_TESTFRAMEWORK_DEV
# Install test frameworks by copy of files
bin_SCRIPTS = \
	sst-test-core \
	sst-test-elements \
	testingframework/sst_test_engine_loader.py

libexec_SCRIPTS += \
	testingframework/sst_unittest.py \
	testingframework/sst_unittest_support.py \
	testingframework/sst_unittest_parameterized.py \
	testingframework/test_engine.py \
	testingframework/test_engine_globals.py \
	testingframework/test_engine_support.py \
	testingframework/test_engine_junit.py \
	testingframework/test_engine_unittest.py

else
# Install test frameworks using symbolic links to the source (we can do development)
install-exec-hook:
	chmod +x $(abs_builddir)/sst-test-core
	chmod +x $(abs_builddir)/sst-test-elements
	ln -sf $(abs_builddir)/sst-test-core                            $(bindir)/sst-test-core
	ln -sf $(abs_builddir)/sst-test-elements                        $(bindir)/sst-test-elements
	ln -sf $(abs_srcdir)/testingframework/sst_test_engine_loader.py $(abs_builddir)/sst_test_engine_loader.py

	ln -sf $(abs_srcdir)/testingframework/sst_unittest.py           $(libexecdir)/sst_unittest.py
	ln -sf $(abs_srcdir)/testingframework/sst_unittest_support.py   $(libexecdir)/sst_unittest_support.py
	ln -sf $(abs_srcdir)/testingframework/sst_unittest_support.py   $(libexecdir)/sst_unittest_parameterized.py
	ln -sf $(abs_srcdir)/testingframework/test_engine.py            $(libexecdir)/test_engine.py
	ln -sf $(abs_srcdir)/testingframework/test_engine_globals.py    $(libexecdir)/test_engine_globals.py
	ln -sf $(abs_srcdir)/testingframework/test_engine_support.py    $(libexecdir)/test_engine_support.py
	ln -sf $(abs_srcdir)/testingframework/test_engine_junit.py      $(libexecdir)/test_engine_junit.py
	ln -sf $(abs_srcdir)/testingframework/test_engine_unittest.py   $(libexecdir)/test_engine_unittest.py
endif

EXTRA_DIST += \
	testingframework/__init__.py \
	testingframework/readme.md \
	testingframework/sst-test-core.in \
	testingframework/sst-test-elements.in \
	testingframework/sst_test_engine_loader.py \
	testingframework/sst_unittest.py \
	testingframework/sst_unittest_support.py \
	testingframework/sst_unittest_parameterized.py \
	testingframework/test_engine.py \
	testingframework/test_engine_globals.py \
	testingframework/test_engine_support.py \
	testingframework/test_engine_junit.py \
	testingframework/test_engine_unittest.py \
	testingframework/build_docs.sh \
	testingframework/pdoc_template/text.mako \
	testingframework/pdoc_template/search.mako \
	testingframework/pdoc_template/pdf.mako \
	testingframework/pdoc_template/logo.mako \
	testingframework/pdoc_template/html.mako \
	testingframework/pdoc_template/head.mako \
	testingframework/pdoc_template/css.mako \
	testingframework/pdoc_template/credits.mako \
	testingframework/pdoc_template/config.mako \
	testingframework/pdoc_template/_lunr_search.inc.mako \
	testingframework/pdoc_template/SSTLogo.png

sst-test-core:
	echo "#!$(PYTHON_EXE)" > $@; \
	cat $(abs_srcdir)/testingframework/sst-test-core.in >> $@

sst-test-elements:
	echo "#!$(PYTHON_EXE)" > $@; \
	cat $(abs_srcdir)/testingframework/sst-test-elements.in >> $@

BUILT_SOURCES = \
	sst-test-core \
	sst-test-elements

CLEANFILES = \
	sst-test-core \
	sst-test-elements \
	sst_test_engine_loader.py
