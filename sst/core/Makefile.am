# -*- Makefile -*-
#
#

CC = $(MPICC)
CXX = $(MPICXX)

AM_CPPFLAGS =  \
	-DTIXML_USE_STL \
	$(BOOST_CPPFLAGS) \
	$(ZOLTAN_CPPFLAGS) \
	$(LTDLINCL)

DIST_SUBDIRS = libltdl .
SUBDIRS = $(LIBLTDL_DIR) .

EXTRA_DIST = mainpage.dox 

sstdir = $(includedir)/sst
dist_sst_HEADERS =
nodist_sst_HEADERS =
nodist_include_HEADERS =

bin_PROGRAMS = sst.x
sst_x_SOURCES =
sst_x_LDADD = \
	$(LIBLTDL) \
	$(BOOST_SERIALIZATION_LIB) \
	$(BOOST_MPI_LIB) \
	$(BOOST_PROGRAM_OPTIONS_LIB) \
	$(ZOLTAN_LIB) \
	$(PARMETIS_LIB) \
	$(MPILIBS)
sst_x_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	$(PARMETIS_LDFLAGS) \
	$(ZOLTAN_LDFLAGS) \
	-export-dynamic \
	$(SST_LTLIBS_COMPONENTS) \
	$(SST_LTLIBS_INTROSPECTORS)

include include/Makefile.inc
include include/sst/Makefile.inc
include tinyxml/Makefile.inc
include src/Makefile.inc
