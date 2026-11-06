## SST V16.1.0 Release Notes

## General
* This release adds a new clock API in SST-Core, including clocks that are guaranteed to run in a fixed order, and simpler templated `registerClock()` and `configureLink()` functions.
* The interactive debug console now uses an object tree. The `print` and `list` commands have more output options, and the console is more robust to malformed commands.
* This is the first release of SST-HGCC, a compiler toolchain (`hgcc`/`hg++`) that builds C/C++ applications into libraries that the Mercury element can run. It requires LLVM 22 with libTooling.
* Merlin has a new `anytopo` topology that can build any network topology from a NetworkX graph and supports source routing.
* Firefly and Ember have a new network storage (NetworkIO) model, including a simple SSD storage device model.
* Several memory backends that have not been maintained for some time were removed from memHierarchy (see SST-Elements below).
* Linux container builds were added to the SST-Core and SST-Elements GitHub Actions testing. 

## Deprecation and Removal Notices
* The gc64-hmcsim (goblin) and original Ramulator memory backends in memHierarchy are now deprecated. They will be removed in SST 17.0. Ramulator users should switch to Ramulator2, which is still supported.
* The DRAMSim, DRAMSim3, HBM DRAMSim, FDSim (FlashDIMMSim), and HybridSim memory backends, as well as the paged multi-backends that depend on them, were removed from memHierarchy in this release. The NVDIMMSim configure checks were also removed.
* Beginning in SST 17.0, statistics will no longer directly store their type in a string variable. Custom statistic types inheriting from `StatisticBase` must override `getStatTypeName()`. `setStatisticTypeName()` is now marked `[[deprecated]]` and will be removed in SST 17.0, because type names will become a property of the statistic type class rather than a per-statistic property.

## Known Issues
* There is an issue when using the Ubuntu 26 provided OpenMPI 5.0.10 as of this release. While simulations run correctly, the output is garbled so tests will not pass. We recommend using OpenMPI 4.1.x until this issue is resolved.
* The `merlin_anytopo_slimfly` test is skipped because it does not return consistent results.

## SST-Core
**New Features and Enhancements**
* New clock API in `BaseComponent`:
    * A templated `registerClock()` creates the clock handler and returns it. The returned handler can be stopped and started with the new `Clock::HandlerBase::activate()` and `deactivate()` functions, which can be used in place of `unregisterClock()` and `reregisterClock()`. This will become the main clock API. It ensures that a clock handler is only ever registered with one clock.
    * `registerOrderedClock()` registers clock handlers that are called in a fixed order relative to other handlers on the same frequency. The order is set with an explicit `order` parameter, holds across the whole component tree, and is kept when clocks are activated and deactivated.
    * Clock handlers now track their clock and whether they are active, which makes checkpointing clock handlers more efficient.
* Templated `configureLink()` and `configureSelfLink()` overloads build the event handler for you. Callers pass the object pointer, with the handler function as a template argument and optional metadata, instead of building an `Event::Handler` first. The existing overloads are still available.
* Added a templated `BasicEvent<dataT>` class for creating simple events with a `using` statement.
* Python `SubComponent` objects now have `addPortModule()`, so PortModules can be installed on a user SubComponent's ports.
* Interactive debug console updates:
    * The console now navigates an object tree instead of the object map.
    * `print` supports recursion depth (`-r`), verbosity (`-v`), number format (`-f hex|oct|dec`), and indexing into containers.
    * `list` supports `-l` and `-ll` for more detail.
    * The new `show` command prints one user-defined command or all of them.
    * Trace buffers can be printed with a dump trigger.
    * The console now checks that the logging and replay file paths are different.

**Notable Bug Fixes**
* Fixed checkpoints not being written when `--checkpoint-sim-period` was set in a simulation with no cross-rank links.
* Fixed restart on some container configurations. The previous config graph file is now copied by hand instead of with `std::filesystem::copy_file()`.
* Fixed shared parameter set names showing up as parameters. `Params::getKeys()` reported a key named after each subscribed shared set, and after a restart every subscribed `Params` had a local parameter named after the set.
* Fixed `PortModule.enableAllStatistics()` in Python ignoring its parameters. For example, a requested `sst.HistogramStatistic` silently became an `sst.AccumulatorStatistic`.
* Fixed `ConfigComponent::addPortModule()` returning the wrong index. With more than one PortModule on a component, the Python handle for one module could change another, or read past the end of the list.
* Fixed a null pointer dereference when setting statistic parameters on an unknown statistic. This now reports a fatal error.
* Parameters given when enabling all statistics are now kept when a model is written with `--output-json` and loaded again.
* Fixed segmentation faults in the debug console for malformed `print` commands and invalid arguments. Fixed setting a variable from a trace action.
* Fixed address sanitizer errors in the debug console and `PerfReporter`.

**General updates, changes, and cleanup**
* `setStatisticTypeName()` is marked `[[deprecated]]` (see Deprecation and Removal Notices).
* Removed dead code from `sst-info`. `sstinfo.h` is no longer installed by the CMake build.
* Fixed a duplicate key and a missing level change in the code segment time profiler.
* Fixed help text for `setThreadCount()` and `sst.rt.interactive`.
* `autogen.sh` now finds the shell in a platform-independent way.
* Moved from pre-commit to prek in GitHub Actions and added zizmor security auditing of workflows.

**Testing Framework updates**
* `run_sst()` accepts an environment to pass to SST. A key whose value is `None` unsets that variable. Tests should use this instead of changing `os.environ`, so they can run concurrently.
* SST-Core and SST-Elements tests run correctly when the test framework is run concurrently.
* Added a floating point tolerance when comparing floating point statistics in test output.
* Removed `check_param_type()` from the testing framework.
* Fixed a mutable default argument (`filters`) in the test support functions.
* Logging statements now appear immediately.

## SST-Elements
* General updates
    * Added `select_build.py`, a script that chooses which element libraries to build. It writes `.ignore` files, including the libraries each one depends on, and reruns `autogen.sh`.
    * Fixed ELI documentation in many elements: missing ports, subcomponent slots, and parameters, plus wrong descriptions.
    * All Python scripts now use `#!/usr/bin/env python3`.
    * Fixed tests so they can run concurrently. They now use separate run directories and pass the environment to `run_sst()`.
    * Added Linux container builds to GitHub Actions testing.
* ariel
    * Added the missing `frontend` slot to the ELI documentation. Fixed calls to `Output::fatal` in the Pin 3 frontend.
* ember
    * Added a NetworkIO library (`ember.networkIOLib`) and a test motif for network storage I/O.
    * Removed `EmberContextSwitchingMessagePassingGenerator` and the `--enable-ember-contexts` configure option.
    * Removed a call to an `Output` object in a destructor that could cause segfaults on shutdown.
* firefly
    * Added network storage (NetworkIO) modeling. The path covers compute nodes, the network, and SSD storage nodes. Addresses are striped round-robin across storage devices. New components include `firefly.hadesNetworkIO` and the `firefly.SimpleSSD` storage device model, which has configurable bandwidth, latency, and a multi-queue bus.
    * Fixed multi-segment intra-node sends being cut short. This caused an assertion failure in Allgather-type collectives when multiple ranks shared one NIC.
* iris
    * Added an ELI-based registry for SUMI collective algorithms, including ring allreduce and a configurable hierarchical allreduce (intra-node, inter-node, and broadcast stages).
    * Collective algorithm parameters now use the `collective.*` namespace, for example `collective.allreduce`. The old `allgather` and `alltoall` parameters now give an error that names their replacements. Unknown `collective.*` parameters are also reported as errors.
    * Fixed several collective algorithm implementations.
    * Fixed buffer handling in the SUMI libfabric provider's `fi_av_straddr`.
* mask-mpi
    * Added collective correctness tests and fixed validation in multi-rank collective tests. Fixed small bugs in the eager protocols.
* memHierarchy
    * Removed several old memory backends and deprecated the gc64-hmcsim and Ramulator backends (see Deprecation and Removal Notices).
    * Updated memory backends for the removal of `TimeConverter` pointers.
    * Added cache and directory snapshots. These work in conjunction with the 'vanadis' CPU model. Set the new `snapshot` (`save` or `load`) and `snapshotDir` parameters on `memHierarchy.Cache` and directory controllers to write cache line addresses, coherence states, and data at the end of a simulation, and load them at setup. Loading is not yet supported for the MESI shared non-inclusive coherence protocol.
    * Fixed checkpoint and restart with the MMAP backing store.
* mercury
    * Fixed loss of ARM64 floating point state when switching between Mercury contexts.
    * Fixed an abort under high fan-in. Message reassembly in the NIC is now keyed by source and flow ID.
    * Removed the unused `common/connection` files.
* merlin
    * Added the `anytopo` topology, which builds any network topology from a NetworkX graph. It comes with the `endpointNIC` and `sourceRoutingPlugin` subcomponents for source routing, where the endpoint NIC chooses the routed paths. Python utilities for common HPC topologies are in `topology/anytopo_utility`.
    * PolarFly and PolarStar topology files are now written to a temporary directory instead of the working directory, so simulations work when the working directory is not writable. Topology validation messages now only print with `--verbose`.
    * Removed an unused variable from the test NIC that could cause problems in setup.
* mmu
    * Fixed a configure check. Added the missing `tlb` slot to the TLB wrapper's ELI documentation.
* prospero
    * The trace tool now honors `traceEnabled` for all trace types. The text reader now detects malformed lines.
* vanadis
    * Fixed loading Vanadis application snapshots. The node OS parameters `checkpointDir` and `checkpoint` are renamed to `snapshotDir` and `snapshot`, to avoid confusion with SST checkpointing. The example and test configurations now set up the memHierarchy snapshot parameters too.
* zodiac
    * Fixed compiling with OTF.

## SST-Macro
* As of SST 15.0, macro's devel branch is no longer being kept in sync with sst-core's devel branch. The master branches and releases will continue to be kept in sync. This means there may be times when the sst-macro and sst-core devel branches do not work together.

# Fixed issues and significant code changes:

## SST-Core
1592    Random segfault running test_MessageMesh_restart_smaller_ckpt_restart

1613    macOS 26 test fail following PR 1604

1622    Don't create PR comment when clang-format passes

1624    Fix issue with set variable action in trace command

1626    Transition from object map to object tree and UI improvements

1627    address sanitizer fix in PerfReporter

1628    Remove check_param_type()

1629    Additions to the element Clock API and other small fixes

1632    Added registerOrderedClock() to BaseComponent API

1635    Update GitHub Actions versions

1636    Core: subtle bug in testing_compare_filtered_diiff()

1637    Fix default mutable argument: filters

1641    Add floating point tolerance for floating point statistics in tests

1644    Change the copy of previous config graph file on restart to use a manual data copy instead of using filesystem::copy_file because it fails on some container configurations

1645    Test framework: force logging statements to appear immediately

1646    Templated registerClock() and configureLink()

1647    Update signal window for SIGTERM test

1648    Add Linux containers to GitHub Actions testing

1649    Don't treat a shared param set's name as a parameter

1650    Preserve all-statistics parameters in JSON models

1651    Use the parameters passed to PortModule.enableAllStatistics()

1652    Silence erroneous unused warning for volatile counters

1653    Fix componentinfo stat maps

1654    Add addPortModule() to Python SubComponent objects

1655    Assorted small fixes

1656    sst_unittest.py: allow passing environment to run_sst

1657    Added an explicit order parameter to registerOrderedClock()

1658    Fix environment variable race in testsuite_default_Output.py

## SST-Elements
2598    Add Anytopo to merlin

2643    Network Storage (NetworkIO) Implementation for SST Firefly NIC

2682    zodiac: fix compilation with OTF

2683    memHierarchy backend updates

2684    ember: remove EmberContextSwitchingMessagePassingGenerator

2686    Firefly: intra-node LoopReq drops all but the first send segment, causing `copyIoVec copied == len` abort on multi-rank-per-node collectives

2687    firefly: preserve all IoVec segments in intra-node LoopReq

2688    fixing errors loading checkpoint files due to mismatched field names

2689    Fix sumi straddr buffer

2691    merlin: write polarfly/polarstar topology files to a writable temp dir

2692    Add script that can be used to easily modify which elements build

2697    mercury/nic: key message reassembly by (source, flowId)

2698    use traceEnabled on all trace types

2702    Apply pre-commit fixes

2703    Ember: heap corruption during teardown from EmberMessagePassingGenerator destructor calling `verbose()`

2705    Prefer python3 vs. unversioned python

2706    Change all #!/usr/bin/env python to python3

2712    Some cleanup of ember stack elements

2714    iris/mask-mpi collective registry

2715    Skip merlin_anytopo_slimfly test

2716    Remove dead variable

2722    Mercury: Preserve ARM64 FP state across Mercury contexts

2726    Remove common/connection*

2728    Mercury/MaSk-MPI: small bug fixes

2729    Fix BackingMMAP checkpointing

2730    select_build.py fixes

2731    Miscellaneous elements fixes

2732    Add Linux containers to GitHub Actions testing

2733    Test concurrency fixes

## SST-HGCC

4       Hgcc mv2-netmod-ofi compatibility update

5       Expanding the set of tests mv2/lit tests

6       Llvm22 main merge

8       Added detailed readme to describe how to build/run sst-hgcc, and added examples

9       fix infinite loop in addTransitiveNullInformation

## SST-Macro
751     Release 16.1.0
