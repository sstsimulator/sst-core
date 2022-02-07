
There are three ways to get the SST version number:
1. From an unbuilt source tree:
grep AC_INIT $<your_sst_topdir>/configure.ac
2. From a built source tree:
grep PACKAGE_VERSION $<your_sst_topdir>/sst/sst_config.h
3. From the sst binary:
sst --version