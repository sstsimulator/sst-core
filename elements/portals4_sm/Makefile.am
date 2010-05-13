# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(top_srcdir)/elements/include

compdir = $(pkglibdir)
comp_LTLIBRARIES = libportals4_sm.la

libportals4_sm_la_SOURCES = \
	portals4_sm.cc \
	trig_cpu/trig_cpu.cc \
	trig_cpu/trig_cpu.h \
	trig_cpu/portals.h \
	trig_cpu/portals.cc \
	trig_cpu/algorithm.h \
	trig_cpu/allreduce_narytree.h \
	trig_cpu/allreduce_narytree_trig.h \
	trig_cpu/allreduce_recdbl.h \
	trig_cpu/allreduce_recdbl_trig.h \
	trig_cpu/allreduce_tree.h \
	trig_cpu/allreduce_tree_trig.h \
	trig_cpu/bcast.h \
	trig_cpu/bcast_trig.h \
	trig_cpu/barrier_tree.h \
	trig_cpu/barrier_tree_trig.h \
	trig_cpu/barrier_recdbl.h \
	trig_cpu/barrier_recdbl_trig.h \
	trig_cpu/barrier_dissem.h \
	trig_cpu/barrier_dissem_trig.h \
	trig_cpu/test_portals.h \
	trig_nic/RtrIf.h \
	trig_nic/trig_nic.cc \
	trig_nic/trig_nic.h \
	trig_nic/trig_nic_event.h

libportals4_sm_la_LDFLAGS = -module -avoid-version

bin_PROGRAMS = portals4_sm_sdlgen
portals4_sm_sdlgen_SOURCES = portals4_sm_sdlgen.c
