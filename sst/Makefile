
TINYXML_DIR=tinyxml
LIB_TINYXML=$(TINYXML_DIR)/libtinyxml.so
MPI_DIR=$(HOME)/local/openmpi/1.3.2
MPI_DIR=/usr
# MPI_DIR=/home/rolf/OpenMPI-1.3.2
ZOLTAN_DIR=$(HOME)/Desktop/simDocs/test2/Zoltan/
ZOLTAN_LIB_DIR=$(HOME)/Desktop/simDocs/test2/Zoltan/Obj_mac
#ZOLTAN_DIR=/usr/local/zoltan/3.1
# ZOLTAN_DIR=/home/rolf/Zoltan-3.1
PARMETIS_DIR=$(HOME)/local/parmetis/3.1
PARMETIS_LIB_DIR=$(HOME)/Desktop/simDocs/test2/ParMETIS3_1/
# PARMETIS_DIR=/usr/local
# PARMETIS_DIR=/home/rolf/Zoltan-3.1
SHARED_LIB_FLAGS = -dynamiclib -undefined error

BOOST_CC=xgcc40
# BOOST_CC=gcc43
BOOST_VER=1_38
#BOOST_DIR  = $(HOME)/local/boost/openmpi-1.3.2/1.38.0
BOOST_DIR  = $(HOME)/Desktop/boost_1_38_0/
#BOOST_DIR  = /usr/local/boost
# BOOST_DIR  = /home/rolf/Boost-1.38

CXX=mpic++ 

#BOOST_INC_DIR  = $(BOOST_DIR)/include/boost-$(BOOST_VER)
BOOST_INC_DIR  = $(BOOST_DIR)
#BOOST_LIB_DIR  = $(BOOST_DIR)/lib 
BOOST_LIB_DIR  = /usr/local/lib
BOOST_LIBS  = -lboost_program_options-$(BOOST_CC)-mt
BOOST_LIBS += -lboost_serialization-$(BOOST_CC)-mt 
BOOST_LIBS += -lboost_mpi-$(BOOST_CC)-mt 

CPPFLAGS  = -Wall -g -O0 -fno-inline
# CPPFLAGS += -pedantic
CPPFLAGS += -Iinclude -I.
CPPFLAGS += -I$(BOOST_INC_DIR)
CPPFLAGS += -I$(TINYXML_DIR)
CPPFLAGS += -I$(MPI_DIR)/include
CPPFLAGS += -I$(ZOLTAN_DIR)/include
genProcFlags = -I../components/genericProc -I../components/genericProc/FE \
	-I../components/genericProc/FE/ppcFrontEnd \
	-I../components/genericProc/FE/ppcFrontEnd/softfloat \
	-I../componenst/genericProc/FE/ppcFrontEnd/osx_headers
CPPFLAGS += $(genProcFlags)
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
        -L$(TINYXML_DIR) -ltinyxml \
        -Llib -lsst \
	-L$(ZOLTAN_LIB_DIR) -lzoltan \
	-L$(PARMETIS_LIB_DIR) -lparmetis -lmetis 

#sim: $(sst_OBJS) $(libsst_OBJS) $(LIB_TINYXML)
#	$(CXX) -o $@ $(sst_OBJS) $(libsst_OBJS) \
#	-L$(BOOST_LIB_DIR) $(BOOST_LIBS) -ldl \
#        -L$(TINYXML_DIR) -ltinyxml \
#	-L$(ZOLTAN_LIB_DIR) -lzoltan \
#	-L$(PARMETIS_LIB_DIR) -lparmetis -lmetis

genericProc_SOURCES = ../components/genericProc/proc.cc \
	../components/genericProc/FE/memory.cc \
        ../components/genericProc/FE/specMem.cc \
	../components/genericProc/FE/ptoVMapper.cc \
	../components/genericProc/FE/debug.cc \
	../components/genericProc/FE/processor.cc \
        ../components/genericProc/FE/thread.cc \
	../components/genericProc/FE/ppcFrontEnd/ofile_print.cc \
        ../components/genericProc/FE/ppcFrontEnd/ppcThread.cc \
        ../components/genericProc/FE/ppcFrontEnd/environ.cc \
        ../components/genericProc/FE/tt7.cc \
        ../components/genericProc/FE/rand.cc \
        ../components/genericProc/FE/sim_trace.cc \
        ../components/genericProc/FE/memSpace.cc \
        ../components/genericProc/FE/mallocSysCall.cc \
        ../components/genericProc/FE/ppcFrontEnd/regs.cc \
        ../components/genericProc/FE/ppcFrontEnd/ppcLoader.cc \
        ../components/genericProc/FE/ppcFrontEnd/ppcMachine.cc \
        ../components/genericProc/FE/ppcFrontEnd/ppcInstruction.cc \
        ../components/genericProc/FE/ppcFrontEnd/ppcCommitMachTrap.cc \
        ../components/genericProc/FE/ppcFrontEnd/ppcCommitPIMTrap.cc \
        ../components/genericProc/FE/ppcFrontEnd/ppcIssueSystemTrap.cc \
        ../components/genericProc/FE/ppcFrontEnd/ppcCommitSystemTrap.cc \
        ../components/genericProc/FE/ppcFrontEnd/ppcInstructionCommit.cc \
        ../components/genericProc/FE/ppcFrontEnd/ppcInstructionIssue.cc 
genericProc_OBJS = $(genericProc_SOURCES:.cc=.o) ../components/genericProc/FE/ppcFrontEnd/softfloat/softfloat.o

../components/libgenericProc.a: $(genericProc_SOURCES)
	(cd ../components/; make libgenericProc.a)

# for some reason, had to compile the genericProc objects into the sst
#library, or it doesn't register classes properly. - at least on
#MacOS. And no, even a static library doesn't seem to work.
$(LIB_SST): $(libsst_OBJS)  ../components/libgenericProc.a
	mpic++ $(SHARED_LIB_FLAGS) -o $@ $(libsst_OBJS)  $(genericProc_OBJS) \
	-L$(BOOST_LIB_DIR) $(BOOST_LIBS) \
	-ldl -L$(ZOLTAN_LIB_DIR) -lzoltan -L$(PARMETIS_LIB_DIR) \
	-lparmetis -lmetis \
	-L$(TINYXML_DIR) -ltinyxml 

$(LIB_TINYXML):
	$(MAKE) SHARED_LIB_FLAGS="$(SHARED_LIB_FLAGS)" -C $(TINYXML_DIR)

clean:
	rm -f sim $(LIB_SST) $(libsst_OBJS) $(sst_OBJS)
	$(MAKE) -C $(TINYXML_DIR) clean

depend:
	makedepend $(genProcFlags) -Iinclude -I. `find . -name "*.cc" -print`

# DO NOT DELETE

clock.o: include/sst/clock.h include/sst/sst.h include/sst/clockEvent.h
clock.o: include/sst/event.h include/sst/boost.h include/sst/debug.h
clock.o: /usr/include/stdio.h /usr/include/_types.h
clock.o: /usr/include/sys/_types.h /usr/include/sys/cdefs.h
clock.o: /usr/include/machine/_types.h /usr/include/i386/_types.h
clock.o: include/sst/eventFunctor.h include/sst/clockHandler.h
clock.o: include/sst/eventQueue.h include/sst/queue.h
clock.o: include/sst/cache.h include/sst/pool.h
clock.o: include/sst/simulation.h include/sst/component.h
clock.o: include/sst/linkMap.h include/sst/sync.h
clock.o: include/sst/syncEvent.h include/sst/factory.h
clock.o: include/sst/sdl.h
clockEvent.o: include/sst/clock.h include/sst/sst.h
clockEvent.o: include/sst/clockEvent.h include/sst/event.h
clockEvent.o: include/sst/boost.h include/sst/debug.h
clockEvent.o: /usr/include/stdio.h /usr/include/_types.h
clockEvent.o: /usr/include/sys/_types.h /usr/include/sys/cdefs.h
clockEvent.o: /usr/include/machine/_types.h /usr/include/i386/_types.h
clockEvent.o: include/sst/eventFunctor.h include/sst/clockHandler.h
clockEvent.o: include/sst/eventQueue.h include/sst/queue.h
clockEvent.o: include/sst/cache.h include/sst/pool.h
clockEvent.o: include/sst/simulation.h include/sst/component.h
clockEvent.o: include/sst/linkMap.h include/sst/sync.h
clockEvent.o: include/sst/syncEvent.h include/sst/factory.h
clockEvent.o: include/sst/sdl.h
component.o: include/sst/component.h include/sst/boost.h
component.o: include/sst/debug.h /usr/include/stdio.h
component.o: /usr/include/_types.h /usr/include/sys/_types.h
component.o: /usr/include/sys/cdefs.h /usr/include/machine/_types.h
component.o: /usr/include/i386/_types.h include/sst/sst.h
component.o: include/sst/linkMap.h include/sst/eventQueue.h
component.o: include/sst/queue.h include/sst/cache.h include/sst/pool.h
component.o: include/sst/event.h include/sst/eventFunctor.h
component.o: include/sst/clock.h include/sst/clockEvent.h
component.o: include/sst/clockHandler.h
config.o: include/sst/config.h include/sst/archive.h
config.o: include/sst/boost.h include/sst/debug.h /usr/include/stdio.h
config.o: /usr/include/_types.h /usr/include/sys/_types.h
config.o: /usr/include/sys/cdefs.h /usr/include/machine/_types.h
config.o: /usr/include/i386/_types.h include/sst/simulation.h
config.o: include/sst/event.h include/sst/eventFunctor.h
config.o: include/sst/sst.h include/sst/eventQueue.h
config.o: include/sst/queue.h include/sst/cache.h include/sst/pool.h
config.o: include/sst/component.h include/sst/linkMap.h
config.o: include/sst/clock.h include/sst/clockEvent.h
config.o: include/sst/clockHandler.h include/sst/sync.h
config.o: include/sst/syncEvent.h include/sst/factory.h
config.o: include/sst/sdl.h
configGraph.o: include/sst/configGraph.h include/sst/graph.h
configGraph.o: include/sst/sdl.h include/sst/debug.h
configGraph.o: /usr/include/stdio.h /usr/include/_types.h
configGraph.o: /usr/include/sys/_types.h /usr/include/sys/cdefs.h
configGraph.o: /usr/include/machine/_types.h /usr/include/i386/_types.h
debug.o: include/sst/debug.h /usr/include/stdio.h /usr/include/_types.h
debug.o: /usr/include/sys/_types.h /usr/include/sys/cdefs.h
debug.o: /usr/include/machine/_types.h /usr/include/i386/_types.h
event.o: include/sst/event.h include/sst/boost.h include/sst/debug.h
event.o: /usr/include/stdio.h /usr/include/_types.h
event.o: /usr/include/sys/_types.h /usr/include/sys/cdefs.h
event.o: /usr/include/machine/_types.h /usr/include/i386/_types.h
event.o: include/sst/eventFunctor.h include/sst/sst.h
export.o: include/sst/clockEvent.h include/sst/event.h
export.o: include/sst/boost.h include/sst/debug.h /usr/include/stdio.h
export.o: /usr/include/_types.h /usr/include/sys/_types.h
export.o: /usr/include/sys/cdefs.h /usr/include/machine/_types.h
export.o: /usr/include/i386/_types.h include/sst/eventFunctor.h
export.o: include/sst/sst.h include/sst/clockHandler.h
export.o: include/sst/stopEvent.h include/sst/syncEvent.h
export.o: include/sst/compEvent.h include/sst/component.h
export.o: include/sst/linkMap.h include/sst/eventQueue.h
export.o: include/sst/queue.h include/sst/cache.h include/sst/pool.h
export.o: include/sst/clock.h
factory.o: include/sst/factory.h include/sst/sst.h
factory.o: include/sst/component.h include/sst/boost.h
factory.o: include/sst/debug.h /usr/include/stdio.h
factory.o: /usr/include/_types.h /usr/include/sys/_types.h
factory.o: /usr/include/sys/cdefs.h /usr/include/machine/_types.h
factory.o: /usr/include/i386/_types.h include/sst/linkMap.h
factory.o: include/sst/eventQueue.h include/sst/queue.h
factory.o: include/sst/cache.h include/sst/pool.h include/sst/event.h
factory.o: include/sst/eventFunctor.h include/sst/clock.h
factory.o: include/sst/clockEvent.h include/sst/clockHandler.h
factory.o: /usr/include/dlfcn.h /usr/include/stdbool.h
factory.o: /usr/include/AvailabilityMacros.h
factory.o: ../components/genericProc/proc.h include/sst/link.h
factory.o: include/sst/compEvent.h include/sst/simulation.h
factory.o: include/sst/sync.h include/sst/syncEvent.h include/sst/sdl.h
factory.o: ../components/genericProc/FE/thread.h
factory.o: ../components/genericProc/FE/instruction.h
factory.o: ../components/genericProc/FE/global.h /usr/include/stdlib.h
factory.o: /usr/include/available.h /usr/include/sys/wait.h
factory.o: /usr/include/sys/signal.h /usr/include/sys/appleapiopts.h
factory.o: /usr/include/machine/signal.h /usr/include/i386/signal.h
factory.o: /usr/include/i386/_structs.h /usr/include/sys/_structs.h
factory.o: /usr/include/machine/_structs.h /usr/include/sys/resource.h
factory.o: /usr/include/machine/endian.h /usr/include/i386/endian.h
factory.o: /usr/include/sys/_endian.h
factory.o: /usr/include/libkern/_OSByteOrder.h
factory.o: /usr/include/libkern/i386/_OSByteOrder.h
factory.o: /usr/include/alloca.h /usr/include/machine/types.h
factory.o: /usr/include/i386/types.h /usr/include/stdint.h
factory.o: /usr/include/errno.h /usr/include/sys/errno.h
factory.o: /usr/include/arpa/inet.h /usr/include/netinet/in.h
factory.o: /usr/include/sys/socket.h /usr/include/machine/_param.h
factory.o: /usr/include/i386/_param.h /usr/include/netinet6/in6.h
factory.o: /usr/include/sys/param.h /usr/include/sys/types.h
factory.o: /usr/include/sys/syslimits.h /usr/include/machine/param.h
factory.o: /usr/include/i386/param.h /usr/include/limits.h
factory.o: /usr/include/machine/limits.h /usr/include/i386/limits.h
factory.o: /usr/include/i386/_limits.h
factory.o: ../components/genericProc/FE/exceptions.h
factory.o: ../components/genericProc/FE/memory.h
factory.o: ../components/genericProc/FE/specMem.h
factory.o: ../components/genericProc/FE/debug.h
factory.o: ../components/genericProc/FE/mallocSysCall.h
factory.o: ../components/genericProc/FE/ppcFrontEnd/ppcMachine.h
factory.o: ../components/genericProc/FE/fuClasses.h
factory.o: ../components/genericProc/FE/ppcFrontEnd/host.h
factory.o: ../components/genericProc/FE/ppcFrontEnd/misc.h
factory.o: /usr/include/stdarg.h /usr/include/string.h
factory.o: ../components/genericProc/FE/ppcFrontEnd/ieee-fp.h
factory.o: ../components/genericProc/FE/ppcFrontEnd/softfloat/softfloat.h
factory.o: ../components/genericProc/FE/ppcFrontEnd/softfloat/softfloat_macros.h
factory.o: ../components/genericProc/FE/ppcFrontEnd/softfloat/softfloat.h
factory.o: ../components/genericProc/FE/ppcFrontEnd/powerpc.def
factory.o: ../components/genericProc/FE/processor.h
factory.o: ../components/genericProc/FE/thread.h
factory.o: ../components/genericProc/FE/ptoVMapper.h
factory.o: ../components/genericProc/FE/pimSysCallTypes.h
linkMap.o: include/sst/link.h include/sst/sst.h include/sst/compEvent.h
linkMap.o: include/sst/event.h include/sst/boost.h include/sst/debug.h
linkMap.o: /usr/include/stdio.h /usr/include/_types.h
linkMap.o: /usr/include/sys/_types.h /usr/include/sys/cdefs.h
linkMap.o: /usr/include/machine/_types.h /usr/include/i386/_types.h
linkMap.o: include/sst/eventFunctor.h include/sst/component.h
linkMap.o: include/sst/linkMap.h include/sst/eventQueue.h
linkMap.o: include/sst/queue.h include/sst/cache.h include/sst/pool.h
linkMap.o: include/sst/clock.h include/sst/clockEvent.h
linkMap.o: include/sst/clockHandler.h include/sst/simulation.h
linkMap.o: include/sst/sync.h include/sst/syncEvent.h
linkMap.o: include/sst/factory.h include/sst/sdl.h
main.o: include/sst/archive.h include/sst/boost.h include/sst/debug.h
main.o: /usr/include/stdio.h /usr/include/_types.h
main.o: /usr/include/sys/_types.h /usr/include/sys/cdefs.h
main.o: /usr/include/machine/_types.h /usr/include/i386/_types.h
main.o: include/sst/simulation.h include/sst/event.h
main.o: include/sst/eventFunctor.h include/sst/sst.h
main.o: include/sst/eventQueue.h include/sst/queue.h
main.o: include/sst/cache.h include/sst/pool.h include/sst/component.h
main.o: include/sst/linkMap.h include/sst/clock.h
main.o: include/sst/clockEvent.h include/sst/clockHandler.h
main.o: include/sst/sync.h include/sst/syncEvent.h
main.o: include/sst/factory.h include/sst/sdl.h include/sst/config.h
main.o: include/sst/configGraph.h include/sst/graph.h
main.o: include/sst/zolt.h
memEvent.o: include/sst/memEvent.h include/sst/compEvent.h
memEvent.o: include/sst/event.h include/sst/boost.h include/sst/debug.h
memEvent.o: /usr/include/stdio.h /usr/include/_types.h
memEvent.o: /usr/include/sys/_types.h /usr/include/sys/cdefs.h
memEvent.o: /usr/include/machine/_types.h /usr/include/i386/_types.h
memEvent.o: include/sst/eventFunctor.h include/sst/sst.h
memEvent.o: include/sst/component.h include/sst/linkMap.h
memEvent.o: include/sst/eventQueue.h include/sst/queue.h
memEvent.o: include/sst/cache.h include/sst/pool.h include/sst/clock.h
memEvent.o: include/sst/clockEvent.h include/sst/clockHandler.h
nicEvent.o: include/sst/nicEvent.h include/sst/compEvent.h
nicEvent.o: include/sst/event.h include/sst/boost.h include/sst/debug.h
nicEvent.o: /usr/include/stdio.h /usr/include/_types.h
nicEvent.o: /usr/include/sys/_types.h /usr/include/sys/cdefs.h
nicEvent.o: /usr/include/machine/_types.h /usr/include/i386/_types.h
nicEvent.o: include/sst/eventFunctor.h include/sst/sst.h
nicEvent.o: include/sst/component.h include/sst/linkMap.h
nicEvent.o: include/sst/eventQueue.h include/sst/queue.h
nicEvent.o: include/sst/cache.h include/sst/pool.h include/sst/clock.h
nicEvent.o: include/sst/clockEvent.h include/sst/clockHandler.h
simulation.o: include/sst/simulation.h include/sst/boost.h
simulation.o: include/sst/debug.h /usr/include/stdio.h
simulation.o: /usr/include/_types.h /usr/include/sys/_types.h
simulation.o: /usr/include/sys/cdefs.h /usr/include/machine/_types.h
simulation.o: /usr/include/i386/_types.h include/sst/event.h
simulation.o: include/sst/eventFunctor.h include/sst/sst.h
simulation.o: include/sst/eventQueue.h include/sst/queue.h
simulation.o: include/sst/cache.h include/sst/pool.h
simulation.o: include/sst/component.h include/sst/linkMap.h
simulation.o: include/sst/clock.h include/sst/clockEvent.h
simulation.o: include/sst/clockHandler.h include/sst/sync.h
simulation.o: include/sst/syncEvent.h include/sst/factory.h
simulation.o: include/sst/sdl.h include/sst/stopEvent.h
simulation.o: include/sst/compEvent.h include/sst/config.h
simulation.o: include/sst/archive.h include/sst/graph.h
sync.o: include/sst/sync.h include/sst/sst.h include/sst/syncEvent.h
sync.o: include/sst/event.h include/sst/boost.h include/sst/debug.h
sync.o: /usr/include/stdio.h /usr/include/_types.h
sync.o: /usr/include/sys/_types.h /usr/include/sys/cdefs.h
sync.o: /usr/include/machine/_types.h /usr/include/i386/_types.h
sync.o: include/sst/eventFunctor.h include/sst/component.h
sync.o: include/sst/linkMap.h include/sst/eventQueue.h
sync.o: include/sst/queue.h include/sst/cache.h include/sst/pool.h
sync.o: include/sst/clock.h include/sst/clockEvent.h
sync.o: include/sst/clockHandler.h include/sst/link.h
sync.o: include/sst/compEvent.h include/sst/simulation.h
sync.o: include/sst/factory.h include/sst/sdl.h
xml.o: include/sst/sdl.h include/sst/debug.h /usr/include/stdio.h
xml.o: /usr/include/_types.h /usr/include/sys/_types.h
xml.o: /usr/include/sys/cdefs.h /usr/include/machine/_types.h
xml.o: /usr/include/i386/_types.h
zolt.o: include/sst/zolt.h include/sst/graph.h include/sst/sdl.h
zolt.o: include/sst/debug.h /usr/include/stdio.h /usr/include/_types.h
zolt.o: /usr/include/sys/_types.h /usr/include/sys/cdefs.h
zolt.o: /usr/include/machine/_types.h /usr/include/i386/_types.h
