
ACLOCAL_AMFLAGS = -I config

SUBDIRS = \
	share \
	src \
	doc

sstconfdir = ${sysconfdir}/sst
sstconf_DATA = src/sst/sstsimulator.conf

EXTRA_DIST = \
	autogen.sh \
	CONTRIBUTING.md \
	scripts/clang-format-test.sh \
	INSTALL \
	ISSUES \
	LICENSE \
	PLATFORMS \
	README.md \
	VERSION

include tests/Makefile.inc
