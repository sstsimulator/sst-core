# Copyright 2009-2023 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2023, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

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
