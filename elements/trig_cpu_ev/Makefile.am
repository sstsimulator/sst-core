# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(top_srcdir)/components/include

compdir = $(pkglibdir)
comp_LTLIBRARIES = libtrig_cpu_ev.la

libtrig_cpu_ev_la_SOURCES = \
	trig_cpu.cc \
	trig_cpu.h \
	portals.h \
	portals.cc \
	algorithm.h \
	allreduce_narytree.h \
	allreduce_narytree_trig.h \
	allreduce_recdbl.h \
	allreduce_recdbl_trig.h \
	allreduce_tree.h \
	allreduce_tree_trig.h \
	bcast.h \
	bcast_trig.h \
	barrier_tree.h \
	barrier_tree_trig.h \
	barrier_recdbl.h \
	barrier_recdbl_trig.h \
	barrier_dissem.h \
	barrier_dissem_trig.h \
	test_portals.h

libtrig_cpu_ev_la_LDFLAGS = -module -avoid-version

bin_PROGRAMS = trig_cpu_ev_sdlgen
trig_cpu_ev_sdlgen_SOURCES = trig_cpu_ev_sdlgen.c
