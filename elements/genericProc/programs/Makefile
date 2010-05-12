
INC = -I../FE
#LIB = -L/Users/afrodri/Desktop/staticTests/ -L/Users/afrodri/Desktop/staticTests/c++static/lib/
#LIB = -L$(HOME)/Desktop/allStatLibs
LIB = -L./allStatLibs
LIBS = -lc_static -lm_ppc -lgcc_static -lstdc++
CFLAGS=-D__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=1030 -U__LDBL_MANT_DIG__
CC = gcc-3.3  $(CFLAGS) $(INC) $(LIB) -static -arch ppc
CXX = g++-3.3 $(CFLAGS) -fno-exceptions $(INC) $(LIB) -static -arch ppc

DCXX = g++ -fno-exceptions $(INC) $(LIB) 
#-U__LDBL_MANT_DIG__ removes the name munging for printf, etc...
CXX4 = g++ -arch ppc -fno-exceptions $(INC) $(LIB) -static -U__LDBL_MANT_DIG__
# a crt1 which points to dyld.sim
DCRT = /Users/afrodri/Desktop/Source_tars/Csu-46/crt1.o
#CRT0 = $(HOME)/Desktop/allStatLibs/crt0.o

default: fooPrint

constructorTest: constructorTest.cc
	$(CXX4) -arch ppc constructorTest.cc $(LIBS) -o constructorTest 

tHello: tHello.cc
	$(CXX) tHello.cc -o tHello $(CRT0) $(LIBS)

dytest.o: dytest.cc
	$(CXX) -c dytest.cc

dytest: dytest.o
	$(DCXX) -bind_at_load -nostdlib dytest.cc -o dytest $(LIB) -lc_static -lgcc_static -lstdc++ -L/usr/lib/system -lmathCommon -L/tmp -lc_new $(DCRT)

scTest: scTest.c ppcPimCalls.h
	gcc $(INC) -x c $(LIB) -g -static scTest.c -o scTest -lc_static -lm_ppc -lgcc_static 

vecTest: vecTest.cc ppcPimCalls.h
	g++ -O3 -Wall -force_cpusubtype_ALL -fno-exceptions $(INC) $(LIB) -static vecTest.cc -o vecTest $(LIBS)

netTest: netTest.c ppcPimCalls.h
	$(CC) netTest.c -o netTest $(LIBS)

smpTest: smpTest.cc ppcPimCalls.h
	$(CXX) smpTest.cc -o smpTest $(LIBS)

netDMATest: netDMATest.cc ppcPimCalls.h
	$(CXX) netDMATest.cc -o netDMATest $(LIBS)

speedTest: speedTest.cc
	$(CXX) -O1 speedTest.cc -o speedTest $(LIBS)

IBMLIB = -L/opt/ibmcmp/xlf/8.1/staticLib -lxlf90 -lxl -lxlfmath
ABLIB = -L/Applications/Absoft/lib/ -lfio -lf77math

hello-ibm: hello.f
#	gcc -static fhelp.c -c
#	/sw/bin/f77 -static  $(INC) $(LIB) hello.f -c
	/opt/ibmcmp/xlf/8.1/bin//f77 -c hello.f
	gcc -L. -nostdlib -L/Users/afrodri/Desktop/staticTests/ -L/Users/afrodri/Desktop/staticTests/c++static/lib/ hello.o  -o hello-ibm  $(DCRT) $(IBMLIB) -lm_ppc -L/usr/lib/gcc/darwin/3.3 -lgcc  -L/usr/lib -lSystem.B

hello-ab: hello.f
	/Applications/Absoft/bin/f77 +B22 $(INC) $(LIB) hello.f -c
	gcc -L. -nostdlib -L/Users/afrodri/Desktop/staticTests/ -L/Users/afrodri/Desktop/staticTests/c++static/lib/ hello.o  -o hello-ab $(ABLIB) -lg2c -lm_ppc -lgcc_static  $(DCRT)  -L/Users/afrodri/Desktop/simSystem -lSystem.B

#note the hard ref to simSystem/libc.a is to get pthread_init
xlfThreadTest: xlfThreadTest.f
	/opt/ibmcmp/xlf/8.1/bin/xlf_r -qsmp=omp -qsmp=schedule=affinity=1562 -qreport -c xlfThreadTest.f
	gcc xlfThreadTest.o -o xlfThreadTest -L. -nostdlib $(DCRT) -L/opt/ibmcmp/lib -lxlsmp.A $(IBMLIB) -L/usr/lib -lSystem.B -L/usr/lib/gcc/darwin/3.3 -L/Users/afrodri/Desktop/simSystem/ /Users/afrodri/Desktop/simSystem/libSimSystem.B.dylib -lgcc
	@echo 
	@echo "don't worry about all the warnings"
	@echo 

#pthreadTest: pthreadTest.cc
#	g++ $(CFLAGS) $(INC) -fno-exceptions -mno-multiple -nostartfiles -nodefaultlibs -o pthreadTest pthreadTest.cc /Users/afrodri/Desktop/Source_tars/Csu-46/crt1.o -Wl,-e,start /Users/afrodri/Desktop/simSystem/libSimSystem.B.dylib

env: env.cc
	g++ $(CFLAGS) $(INC) -mno-multiple -nostartfiles -o env env.cc /Users/afrodri/Desktop/Source_tars/Csu-46/crt1.o -Wl,-e,start 

libcrt0.o: crt0.o
	ar -r libcrt0.o crt0.o
	ranlib libcrt0.o


fooPrint: fooPrint.cc 
	gcc $(CFLAGS) $(INC) -x c $(LIB) -g -static fooPrint.cc -lc_static -lm_ppc -lgcc_static -o fooPrint -arch ppc

fooPrint2: fooPrint2.cc 
	gcc $(INC) -arch ppc -x c $(LIB) -g -static fooPrint2.cc -lc_static -lm_ppc -lgcc_static -o fooPrint2 

pthreadTest: pthreadTest.cc
	$(CC) -arch ppc -x c $(LIB) -g -static pthreadTest.cc -lc_static -lm_ppc -lgcc_static -o pthreadTest 
