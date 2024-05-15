![SST](http://sst-simulator.org/img/sst-logo-small.png)

# Structural Simulation Toolkit (SST)

#### Copyright (c) 2009-2024, National Technology and Engineering Solutions of Sandia, LLC (NTESS)

---

## Cmake Build Instructions

These instructions provide a list of the permissible CMake build directives
that can be utilized to build the SST-Core infrastructure.

1. Download the respective SST release or check out the code from Github
2. Create a build directory:
```
$> cd sstcore-VERSION
$> mkdir build
$> cd build
```
3. Execute cmake with the appropriate options (listed below).
```
$> cmake ../experimental
```
4. Execute make to build the source code.  Optionally, use parallel builds and/or install the source
```
$> make -j
$> make install
```

### CMake Build Directives (-DOPTION=ON/OFF)

* -DCMAKE\_INSTALL\_PREFIX=/path : Sets the CMake installation prefix
* -DCMAKE\_BUILD\_TYPE=TYPE : (Debug, Release, etc) Sets the optimization/debug flags
* -DBUILD\_DOCUMENTATION=ON : Enables Doxygen documentation: use `make doc`
* -DBUILD\_ALL\_TESTING=ON : Enables the SST test harness: use `make test`; requires installation
* -DSST\_DISABLE\_MEM\_POOLS=ON : Disables the SST memory pools
* -DSST\_ENABLE\_EVENT\_TRACKING=ON : Enables the SST even tracking (mem pools must be enabled)
* -DSST\_ENABLE\_DEBUG\_OUTPUT=ON : Enables the SST debug flags
* -DSST\_ENABLE\_PROFILE\_BUILD: Enables the internal SST profiling options
* -DSST\_DISABLE\_ZLIB=ON : Disables LibZ support
* -DSST\_ENABLE\_HDF5=ON : Enables HDF5 support
* -DSST\_DISABLE\_MPI=ON : Disables MPI
* -DSST\_BUILD\_RPM=ON : Enables RPM packaging. Must have RPMTools installed: use `make package`
* -DSST\_BUILD\_DEB=ON : Enables DEB packaging. Must have Debian tools installed: use `make package`
* -DSST\_BUILD\_TGZ=ON : Enables TGZ packaging. Must have TGZ installed: use `make package`

Note: when using the integrated CMake packaging options (RPM/DEB/TGZ), you must also build
the SST documentation (see `-DBUILD\_DOCUMENTATION=ON`).


---
For more information on SST, see the top-level [README.md](README.md) file.
---

Visit [sst-simulator.org](http://sst-simulator.org) to learn more about SST.

See [Contributing](https://github.com/sstsimulator/sst-core/blob/devel/CONTRIBUTING.md) to learn how to contribute to SST.

##### [LICENSE](https://github.com/sstsimulator/sst-core/blob/devel/LICENSE)
