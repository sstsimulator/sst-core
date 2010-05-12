# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
    -I$(top_srcdir)/elements/include \
	-I$(srcdir)/FE \
	-I$(srcdir)/FE/ppcFrontEnd \
	-I$(srcdir)/FE/ppcFrontEnd/osx_headers \
	-I$(srcdir)/FE/ppcFrontEnd/softfloat

compdir = $(pkglibdir)
comp_LTLIBRARIES = libgenericProc.la
libgenericProc_la_SOURCES = \
	proc.h \
	proc.cc \
	FE/fe_debug.h \
	FE/fe_debug.cc \
	FE/exceptions.h \
	FE/fuClasses.h \
	FE/global.h \
	FE/instruction.h \
        FE/mallocSysCall.h \
        FE/mallocSysCall.cc \
        FE/memSpace.cc \
	FE/memory.h \
	FE/memory.cc \
	FE/pimSysCallDefs.h \
	FE/pimSysCallTypes.h \
	FE/pool.h \
	FE/processor.h \
	FE/processor.cc \
	FE/ptoVMapper.h \
	FE/ptoVMapper.cc \
        FE/rand.h \
        FE/rand.cc \
        FE/sim_trace.h \
        FE/sim_trace.cc \
        FE/sim_trace_types.h \
        FE/specMem.h \
        FE/specMem.cc \
        FE/thread.h \
        FE/thread.cc \
        FE/tt7.cc \
	FE/ppcFrontEnd/SCall.h \
        FE/ppcFrontEnd/environ.cc \
        FE/ppcFrontEnd/environ.h \
        FE/ppcFrontEnd/eval.h \
        FE/ppcFrontEnd/host.h \
	FE/ppcFrontEnd/ieee-fp.h \
        FE/ppcFrontEnd/loadInfo.h \
        FE/ppcFrontEnd/misc.h \
	FE/ppcFrontEnd/ofile_print.h \
	FE/ppcFrontEnd/ofile_print.cc \
	FE/ppcFrontEnd/powerpc.def \
        FE/ppcFrontEnd/ppcCommitMachTrap.cc \
        FE/ppcFrontEnd/ppcCommitPIMTrap.cc \
        FE/ppcFrontEnd/ppcCommitSystemTrap.cc \
        FE/ppcFrontEnd/ppcFront.h \
        FE/ppcFrontEnd/ppcInstruction.cc \
        FE/ppcFrontEnd/ppcInstructionCommit.cc \
        FE/ppcFrontEnd/ppcInstructionIssue.cc \
        FE/ppcFrontEnd/ppcIssueSystemTrap.cc \
        FE/ppcFrontEnd/ppcLoader.h \
        FE/ppcFrontEnd/ppcLoader.cc \
        FE/ppcFrontEnd/ppcMachine.h \
        FE/ppcFrontEnd/ppcMachine.cc \
	FE/ppcFrontEnd/ppcSystemTrapHandler.h \
        FE/ppcFrontEnd/ppcThread.cc \
	FE/ppcFrontEnd/ppc_syscall.h \
        FE/ppcFrontEnd/regs.cc \
	FE/ppcFrontEnd/regs.h \
	FE/ppcFrontEnd/sim_syscalls_compat.hpp \
	FE/ppcFrontEnd/osx_headers/architecture/ppc/cframe.h \
	FE/ppcFrontEnd/osx_headers/mach/mach.h \
	FE/ppcFrontEnd/osx_headers/mach-o/fat.h \
	FE/ppcFrontEnd/osx_headers/mach-o/loader.h \
	FE/ppcFrontEnd/osx_headers/mach-o/nlist.h \
	FE/ppcFrontEnd/osx_headers/mach-o/reloc.h \
	FE/ppcFrontEnd/osx_headers/sys/sysctl.h \
	ssBackEnd/prefetch.cc \
	ssBackEnd/prefetch.h \
	ssBackEnd/ssb.h \
	ssBackEnd/ssb_DMA_fakeInst.cc \
	ssBackEnd/ssb_DMA_fakeInst.h \
	ssBackEnd/ssb_bitmap.h \
	ssBackEnd/ssb_bpred.cc \
	ssBackEnd/ssb_bpred.h \
	ssBackEnd/ssb_cache.cc \
	ssBackEnd/ssb_cache.h \
	ssBackEnd/ssb_cv_link.cc \
	ssBackEnd/ssb_cv_link.h \
	ssBackEnd/ssb_eventq.cc \
	ssBackEnd/ssb_eventq.h \
	ssBackEnd/ssb_fetch_rec.h \
	ssBackEnd/ssb_fu_config-defs.h \
	ssBackEnd/ssb_fu_config.h \
	ssBackEnd/ssb_host.h \
	ssBackEnd/ssb_machine.h \
	ssBackEnd/ssb_main.cc \
	ssBackEnd/ssb_mainProc.cc \
	ssBackEnd/ssb_mainProc.h \
	ssBackEnd/ssb_memory.cc \
	ssBackEnd/ssb_memory.h \
	ssBackEnd/ssb_misc.cc \
	ssBackEnd/ssb_misc.h \
	ssBackEnd/ssb_options.cc \
	ssBackEnd/ssb_options.h \
	ssBackEnd/ssb_resource.cc \
	ssBackEnd/ssb_resource.h \
	ssBackEnd/ssb_rs_link.cc \
	ssBackEnd/ssb_rs_link.h \
	ssBackEnd/ssb_ruu.h \
	ssBackEnd/ssb_sim-outorder-constructor.cc \
	ssBackEnd/ssb_sim-outorder-dep.cc \
	ssBackEnd/ssb_sim-outorder-enkidu.cc \
	ssBackEnd/ssb_sim-outorder-eventq.cc \
	ssBackEnd/ssb_sim-outorder-init.cc \
	ssBackEnd/ssb_sim-outorder-memory.cc \
	ssBackEnd/ssb_sim-outorder-options.cc \
	ssBackEnd/ssb_sim-outorder-options.h \
	ssBackEnd/ssb_sim-outorder-processor.cc \
	ssBackEnd/ssb_sim-outorder-readyq.cc \
	ssBackEnd/ssb_sim-outorder-tracer.cc \
	ssBackEnd/ssb_sim-outorder.cc \
	ssBackEnd/ssb_sim-outorder.h \
	ssBackEnd/ssb_spec_mem_ent.h \
	ssBackEnd/ssb_stats.cc \
	ssBackEnd/ssb_stats.h \
	ssBackEnd/ssb_version.h \
	FE/ppcFrontEnd/softfloat/softfloat.c \
	FE/ppcFrontEnd/softfloat/softfloat.h \
	FE/ppcFrontEnd/softfloat/softfloat_macros.h \
	FE/ppcFrontEnd/softfloat/milieu.h \
	FE/ppcFrontEnd/softfloat/softfloat-macros \
	FE/ppcFrontEnd/softfloat/softfloat-specialize

EXTRA_DIST = \
	FE/ppcFrontEnd/ieee-fp.c \
	FE/ppcFrontEnd/softfloat/README \
	FE/ppcFrontEnd/softfloat/test.cpp \
	FE/ppcFrontEnd/softfloat/processors/386-GCC.h \
	FE/ppcFrontEnd/softfloat/processors/SPARC-GCC.h \
	FE/ppcFrontEnd/softfloat/softfloat/README.txt \
	FE/ppcFrontEnd/softfloat/softfloat/SoftFloat-history.txt \
	FE/ppcFrontEnd/softfloat/softfloat/SoftFloat-source.txt \
	FE/ppcFrontEnd/softfloat/softfloat/SoftFloat.txt \
	FE/ppcFrontEnd/softfloat/softfloat/timesoftfloat.txt

libgenericProc_la_LDFLAGS = -module -avoid-version
