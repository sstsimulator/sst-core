# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(top_srcdir)/elements/include

compdir = $(pkglibdir)
comp_LTLIBRARIES = libportals4_sm.la

libportals4_sm_la_SOURCES = \
	portals4_sm.cc \
	trig_cpu_ev/trig_cpu.cc \
	trig_cpu_ev/trig_cpu.h \
	trig_cpu_ev/portals.h \
	trig_cpu_ev/portals.cc \
	trig_cpu_ev/algorithm.h \
	trig_cpu_ev/allreduce_narytree.h \
	trig_cpu_ev/allreduce_narytree_trig.h \
	trig_cpu_ev/allreduce_recdbl.h \
	trig_cpu_ev/allreduce_recdbl_trig.h \
	trig_cpu_ev/allreduce_tree.h \
	trig_cpu_ev/allreduce_tree_trig.h \
	trig_cpu_ev/bcast.h \
	trig_cpu_ev/bcast_trig.h \
	trig_cpu_ev/barrier_tree.h \
	trig_cpu_ev/barrier_tree_trig.h \
	trig_cpu_ev/barrier_recdbl.h \
	trig_cpu_ev/barrier_recdbl_trig.h \
	trig_cpu_ev/barrier_dissem.h \
	trig_cpu_ev/barrier_dissem_trig.h \
	trig_cpu_ev/test_portals.h \
	trig_nic/RtrIf.h \
	trig_nic/trig_nic.cc \
	trig_nic/trig_nic.h \
	trig_nic/trig_nic_event.h

libportals4_sm_la_LDFLAGS = -module -avoid-version

bin_PROGRAMS = portals4_sm_sdlgen
portals4_sm_sdlgen_SOURCES = portals4_sm_sdlgen.c
