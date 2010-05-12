# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(top_srcdir)/elements/include

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcpuV2.la
libcpuV2_la_SOURCES = cpu.h cpu.cc libAlloc.cc
libcpuV2_la_LDFLAGS = -module -avoid-version
