# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS)

CC = $(MPICC)
CXX = $(MPICXX)

introdir = $(pkglibdir)
intro_LTLIBRARIES = libintrospector_cpu.la
libintrospector_cpu_la_SOURCES = introspector_cpu.h introspector_cpu.cc
libintrospector_cpu_la_LDFLAGS = -module -avoid-version
