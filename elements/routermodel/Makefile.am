# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libroutermodel.la
libroutermodel_la_SOURCES = routermodel.h routermodel.cc
libroutermodel_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = 2Dmesh4x4.xml 2Dmesh4x4.README

include src_genTopo/Makefile.inc
