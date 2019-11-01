
ACLOCAL_AMFLAGS = -I config

SUBDIRS = \
  src \
  doc \
  test

sstconfdir = ${sysconfdir}/sst
sstconf_DATA = src/sst/sstsimulator.conf

EXTRA_DIST = \
	autogen.sh \
	CONTRIBUTING.md \
	INSTALL \
	ISSUES \
	LICENSE \
	PLATFORMS \
	README.md \
	VERSION
	
coverage:
	-mkdir coverage
	$(LCOV_EXE) -d $(top_builddir)/src/sst/core -c -o $(top_builddir)/coverage/coverage.info
	-genhtml $(top_builddir)/coverage/coverage.info -o coverage-report
.PHONY: coverage

