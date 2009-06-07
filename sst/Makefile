
TINYXML_DIR=./tinyxml
LIB_TINYXML=$(TINYXML_DIR)/libtinyxml.so
MPI_DIR=$(HOME)/local/openmpi/1.3.2
MPI_DIR=/usr
# MPI_DIR=/home/rolf/OpenMPI-1.3.2
ZOLTAN_DIR=$(HOME)/local/zoltan/3.1
ZOLTAN_DIR=/usr/local/zoltan/3.1
# ZOLTAN_DIR=/home/rolf/Zoltan-3.1
PARMETIS_DIR=$(HOME)/local/parmetis/3.1
PARMETIS_DIR=/usr/local
# PARMETIS_DIR=/home/rolf/Zoltan-3.1

BOOST_CC=gcc42
# BOOST_CC=gcc43
BOOST_VER=1_38
BOOST_DIR  = $(HOME)/local/boost/openmpi-1.3.2/1.38.0
BOOST_DIR  = /usr/local/boost
# BOOST_DIR  = /home/rolf/Boost-1.38

CXX=mpic++ 

BOOST_INC_DIR  = $(BOOST_DIR)/include/boost-$(BOOST_VER)
BOOST_LIB_DIR  = $(BOOST_DIR)/lib 
BOOST_LIBS  = -lboost_program_options-$(BOOST_CC)-mt
BOOST_LIBS += -lboost_serialization-$(BOOST_CC)-mt 
BOOST_LIBS += -lboost_mpi-$(BOOST_CC)-mt 

CPPFLAGS  = -Wall
# CPPFLAGS += -pedantic
CPPFLAGS += -Iinclude -I.
CPPFLAGS += -I$(BOOST_INC_DIR)
CPPFLAGS += -I$(TINYXML_DIR)
CPPFLAGS += -I$(MPI_DIR)/include
CPPFLAGS += -I$(ZOLTAN_DIR)/include
CPPFLAGS += -DTIXML_USE_STL

CXXFLAGS = -fPIC

VPATH = src

libsst_SOURCES = linkMap.cc sync.cc export.cc clock.cc simulation.cc config.cc\
	factory.cc debug.cc event.cc component.cc clockEvent.cc xml.cc \
	configGraph.cc zolt.cc memEvent.cc nicEvent.cc

LIB_SST=lib/libsst.so
    
libsst_OBJS = $(libsst_SOURCES:.cc=.o)

sst_SOURCES = main.cc

sst_OBJS = $(sst_SOURCES:.cc=.o)

sim: $(sst_OBJS) $(LIB_SST) $(LIB_TINYXML)
	$(CXX) -o $@ $(sst_OBJS) -L$(BOOST_LIB_DIR) $(BOOST_LIBS) -ldl \
        -Llib -lsst \
        -L$(TINYXML_DIR) -ltinyxml \
		-L$(ZOLTAN_DIR)/lib -lzoltan \
		-L$(PARMETIS_DIR)/lib -lparmetis -lmetis

$(LIB_SST): $(libsst_OBJS)
	mpic++ -shared -o $@ $^ -L$(BOOST_LIB_DIR) $(BOOST_LIBS) -ldl

$(LIB_TINYXML):
	$(MAKE) -C $(TINYXML_DIR)

clean:
	rm -f sim $(LIB_SST) $(libsst_OBJS) $(sst_OBJS)
	$(MAKE) -C $(TINYXML_DIR) clean
