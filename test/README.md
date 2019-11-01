![SST](http://sst-simulator.org/img/sst-logo-small.png)

# Coverage Testing

All new code and all modified code in each pull request should be covered by tests.
A useful tool in this respect, are code coverage tools that track all lines of code
executed by a test-suite. Although this is complicated somewhat for C++ code
heavily reliant on templates, the vast majority of SST is "vanilla" C++ that can be assessed 
with a coverage tool. Here we walk through how to setup, run, and analyze the coverage tests.

# Setting Up Coverage

The main coverage tools are lcov and gcov. These require special `CXXFLAGS` and `CFLAGS` to be
added to instrument the code for coverage. A configure option `--enable-coverage` is now available
that will automatically add these flags. In most cases, the flags are just `-fprofile-arcs` and `-ftest-coverage`.
Additionally `-fPIC` get added for SST. Once configured, the instrumented SST should be installed:
````
make install
````

# Running Coverage

The code coverage suite uses Ctest (via CMake) to run and evaluate tests.
After installing, running `make installcheck` will configure the test library and test component libraries.
The coverage suite builds a library `libctest.so` that contains a set of components and subcomponents that provide coverage of core features.
The exact setup of the components and subcomponents is determined by Python configuration files in the `test/config` folder.
`make installcheck` creates a folder `test/ctest` with the configured Ctest.
In the folder `test/ctest`, you can run `make` to build the test component library.
Once built, you must then run `make test` to run all the config files.
Each config file will execute some portion of the simulator core.
These contributions are added to `gcda` files which accumulate the overall coverage.


# Analyzing Coverage Results
After running the tests, you can run `make coverage` in the top-level build directory.
This will run the whole workflow and eventually generate a web page `coverage-report`.
This collects results from all the `gcda` files and determines which lines/functions have been executed by all the tests.
The individual steps executed by `make coverage` are:
* `mkdir coverage`
* `lcov -d <BUILD_DIR>/src/sst/core -c -o <BUILD_DIR>/coverage/converage.info`
* `genhtml <BUILD_DIR>coverage/coverage.info -o coverage-report`

This creates a folder to hold results, uses `lcov` to collect results, and then generates an HTML report.

#### Copyright (c) 2009-2019, National Technology and Engineering Solutions of Sandia, LLC (NTESS)

The Structural Simulation Toolkit (SST) was developed to explore innovations in highly concurrent systems where the ISA, microarchitecture, and memory interact with the programming model and communications system. The package provides two novel capabilities. The first is a fully modular design that enables extensive exploration of an individual system parameter without the need for intrusive changes to the simulator. The second is a parallel simulation environment based on MPI. This provides a high level of performance and the ability to look at large systems. The framework has been successfully used to model concepts ranging from processing in memory to conventional processors connected by conventional network interfaces and running MPI.

---

Visit [sst-simulator.org](http://sst-simulator.org) to learn more about SST.

See [Contributing](https://github.com/sstsimulator/sst-core/blob/devel/CONTRIBUTING.md) to learn how to contribute to SST.

##### [LICENSE](https://github.com/sstsimulator/sst-core/blob/devel/LICENSE)
