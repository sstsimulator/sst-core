
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
	INSTALL.md \
	ISSUES.md \
	LICENSE.md \
	PLATFORMS.md \
	README.md \
	VERSION.md

include tests/Makefile.inc
