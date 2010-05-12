# -*- Makefile -*-
#
#
AM_CPPFLAGS = $(BOOST_CPPFLAGS)
compdir = $(pkglibdir)
comp_LTLIBRARIES = libPsst.la
libPsst_la_SOURCES = psst.h psst.cpp
libPsst_la_LDFLAGS = -module -avoid-version

# From old models/Makefile.am
MODEL_DEPS = models/model.h models/queue.h
comp_LTLIBRARIES += model.la \
                    fault_model.la \
                    sample_model.la \
                    cache_model.la

model_la_SOURCES = models/model.h \
                   models/queue.h \
                   models/model.cpp \
                   vm.h vm-iface.h vm.cpp \
                   eventqueue.h eventqueue.cpp

fault_model_la_SOURCES = $(MODEL_DEPS) models/fault_model.cpp
sample_model_la_SOURCES = $(MODEL_DEPS) models/sample_model.cpp
cache_model_la_SOURCES = $(MODEL_DEPS) models/cache_model.cpp
model_la_LDFLAGS = -module -avoid-version
fault_model_la_LDFLAGS = -module -avoid-version
sample_model_la_LDFLAGS = -module -avoid-version
cache_model_la_LDFLAGS = -module -avoid-version

