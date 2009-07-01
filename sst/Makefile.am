# -*- Makefile -*-
#
#

AM_CPPFLAGS =  \
	-DTIXML_USE_STL \
	$(BOOST_CPPFLAGS) \
	$(ZOLTAN_CPPFLAGS)
AM_LDFLAGS = $(BOOST_LDFLAGS) $(PARMETIS_LDFLAGS) $(ZOLTAN_LDFLAGS)

SUBDIRS = libltdl .

EXTRA_DIST =

lib_LTLIBRARIES = libsst.la
libsst_la_SOURCES =
libsst_la_LIBADD = \
	$(LIBLTDL) \
	$(BOOST_SERIALIZATION_LIB) \
	$(BOOST_MPI_LIB) \
	$(BOOST_PROGRAM_OPTIONS_LIB) \
	$(ZOLTAN_LIB) \
	$(PARMETIS_LIB) \
	$(MPILIBS)

sstdir = $(includedir)/sst
dist_sst_HEADERS =
nodist_include_HEADERS =

bin_PROGRAMS = sst.x
sst_x_SOURCES =
sst_x_LDADD = libsst.la

include include/Makefile.inc
include include/sst/Makefile.inc
include tinyxml/Makefile.inc
include src/Makefile.inc
