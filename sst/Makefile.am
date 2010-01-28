# -*- Makefile -*-
#
#

AM_CPPFLAGS =  \
	-DTIXML_USE_STL \
	$(BOOST_CPPFLAGS) \
	$(ZOLTAN_CPPFLAGS) \
	$(LTDLINCL)

DIST_SUBDIRS = libltdl .
SUBDIRS = $(LIBLTDL_DIR) .

EXTRA_DIST = mainpage.dox 

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
libsst_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	$(PARMETIS_LDFLAGS) \
	$(ZOLTAN_LDFLAGS)
	-version-info 0:0:0

sstdir = $(includedir)/sst
dist_sst_HEADERS =
nodist_sst_HEADERS =
nodist_include_HEADERS =

bin_PROGRAMS = sst.x
sst_x_SOURCES =
sst_x_LDADD = libsst.la
sst_x_LDFLAGS = -export-dynamic $(SST_LTLIBS_COMPONENTS)

include include/Makefile.inc
include include/sst/Makefile.inc
include tinyxml/Makefile.inc
include src/Makefile.inc
