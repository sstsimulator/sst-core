CMakeCache.txt: CMakeLists.txt
	-mkdir ctest
	cd ctest && cmake ${abs_srcdir} -DCMAKE_CXX_COMPILER=${CXX} -DSST_ROOT=${prefix}

EXTRA_DIST=Makefile.am component.cc config

installcheck-local: CMakeCache.txt
