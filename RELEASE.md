# SST 15.1 Release Notes

SST 15.1.2 Bug Fix Release - 2026-Feb
===========================================

## SST-Core
* Reverted JSON parser to 15.0.0 version. This reintroduces 32b file size limits but avoids performance and functional problems seen with the streaming parser introduced in 15.1.0.

SST 15.1.1 Bug Fix Release - 2026-Jan
===========================================

## SST-Core
* Fixed bug that occurred in rare cases in parallel loaded simulations when checking ports on nonlocal links.

SST 15.1.0 Release - 2025-Nov
===========================================

## General
* This release has several performance improvements and bug fixes.
* The experimental debug console that allows online introspection of elements is significantly more capable.
* The memory overhead for configuration and for statistics has been optimized.
* Release tarballs will now include both these release notes and a summary of the platforms SST has been tested on (see PLATFORMS.md in the release tarballs).
  The website will continue to be updated with this information as well.

## Deprecation and Removal Notices
* There are no new deprecations added in this point release. The following list is copied from the 15.0 release.
* Use of shared TimeConverters (via pointers returned from Core API functions) is deprecated. Core APIs that accepted a `TimeConverter*` have been updated to accept a `TimeConverter` instead.
  In SST 16.0, Core APIs that return TimeConverter pointers will be changed to return a TimeConverter instance instead. To prepare for this switch, change code to construct a local
  TimeConverter instance from  a returned TimeConverter pointer as shown.
```
        // Old code
        TimeConverter* tc = registerClock(...);

        // New code
        TimeConverter tc = registerClock(...);
```
* In SST input files, GlobalParams have been renamed to SharedParams to reflect the fact that they can be shared among elements but are not "global" as there can be multiple groups of SharedParams that are not shared across all elements.
* Reminder: The Event::Handler and Clock::Handler handler types were deprecated in SST 14.0. Use Event::Handler2 and Clock::Handler2 instead.
    * The Handler signatures will be switched to match Handler2 in SST 16.0.
    * The name Handler2 will be deprecated at SST 16.0 and removed in SST 17.0.
    * Update all elements to use the new signatures before or at SST 16.0. Switching before SST 16.0 will require changing (`Handler<class>` -> `Handler2<class,&class::callback_function>`) prior to SST 16.0 and then to (`Handler<class, &class::callback_function>`) after 16.0.
      Waiting until SST 16.0 to switch will cause your code to break in SST 16.0 until you change the definition of (`Handler<class>` -> `Handler<class, &class::callback_function>`).
* SST-Macro is a maintenance-only development effort. Due to increasing divergence between the SST-Core and SST-Macro, we will no longer be ensuring that the SST-Core and SST-Macro devel branches stay in sync.
  However, we will continue to ensure that the master  branches and releases of SST-Macro work with SST-Core. Where possible, we recommend using the mercury element instead of SST-Macro.
  If mercury is missing key functionality present in SST-Macro that is not found elsewhere in SST-Elements, please let us know.


## Known Issues
* The OpenMPI 4.1.1 available through Rocky Linux and other OS distributions masks SST's error return codes which causes some SST tests to fail. A workaround is to use OpenMPI 4.1.6.
* Element python modules (e.g., pymerlin, pyember) do not work under Python 3.14. This issue is being investigated.
* Due to changes in default floating-point compile options which leads to slightly different results, some SST tests currently report failures with gcc 14 on Rocky 10. This is a test issue only and will be fixed for the next release.


## SST-Core
**New Features and Enhancements**
* Added support for statistics to PortModules. Also added access to the sst\_fatal and sst\_assert functions.
* Reduced memory overhead for ConfigLink which reduces memory footprint for simulations with many links.
* Reduced memory overhead for Statistics which improves performance for simulations that use statistics and/or that declare statistics but do not enable them.
* Re-wrote JSON reader and writer to use a streaming interface which improves performance and eliminates prior file size limitations
* Significant usability improvements in InteractiveConsole.
* A new `SST_ELI_IS_CHECKPOINTABLE` can be used to indicate an element supports checkpoint. The macro takes no arguments.
* Parallel checkpoints can now be restarted in serial.
* Output from --print-timing-info reports more information and a verbosity level can be specified to increase level of granularity of code sections
* Enhanced checkpoint serialization support to include `std::variant`, `std::optional`, `std::bitset`, `std::valarray`, `std::unique_ptr` and others.
* Libraries that have the path key `_EXT_` in them are excluded from sst-info, enabling elements to install extra non-element libraries
* Restart no longer requires the `--load-checkpoint` flag. A checkpoint file (.sstcpt) will be automatically recognized.
* SST now exits with an error message instead of segfaulting if a bad configuration file is provided.
* Added ability to write profiling/timing information to JSON. This does not yet support the expanded timing information that was also added in this release.

**Bug Fixes**
* Fixed issue where JSON model reader could not load JSON models exceeding 32b of keys
* Fixed issue where event IDs could potentially be reused when restarting from a checkpoint
* Fixed error in mapping serialization which caused nullptr names
* Fixed missing lock\_guard in SharedSet::write() which could cause a race condition
* Fixed bug where sst-info would crash if run without arguments due to auxiliary non-element libraries present in library search path
* Added missing `unregisterClock` definition in BaseComponent
* Fixed issue where writing the simulation configuration to a Python file did not correctly instantiate links
* Fixed race condition in Params

**General updates, changes, and cleanup**
* Cleaned up #includes.
* While code for serializing `shared_ptr` and `weak_ptr` is present in the repository and release, it is not enabled in the release as it can cause compile errors on some older compilers.
* Test framework cleanup including removing unused code and always printing file diffs on test failures
* Checkpoint manifest now stores relative instead of absolute paths to checkpoint files for easier relocation
* Expose MPI\_Comm\_spawn\_multiple to elements, enabling elements to trace processes that use MPI internally without interfering with SST's use. This is experimental while we work out the mechanism.
* Code reorganization in main to simplify/unify regular-start and restart paths
* Some checkpoint reorganization so that elements are checkpointed as self-contained binary blobs. This will support a future feature allowing repartitioning and different parallelism for restarts.
  In this release, a restart must either be serial or have the same parallelism as it was checkpointed with.

## SST-Elements
* General updates
  * General fixes for compile warnings across several elements
* ariel
  * New PEBIL-based frontend supports static binary instrumentation for x86 and ARM binaries
  * Fixed nullptr format string that was passed to Output::fatal()
  * arielapi.h is now installed
* golem
  * Correctly allocate an array, fixing potential segfault
* memHierarchy
  * Fix for statistics that could be uninitialized during a checkpoint.
  * Optimized memory region intersection check which can be slow.
  * Added support for checkpointing caches and directory controller
* mercury
  * Moved auxiliary libraries to install in an "ext" subdirectory; this enables sst-info to skip them, fixing a fatal error in sst-info.
  * Threading fix and other stabilization improvements
  * Added support for application arguments
* merlin
  * Updated merlin system object to account for allocation\_block\_size when filling in extra nodes with EmptyJob.
  * Fixed propagation of init events in dragonfly
  * Fix for uniform target generator
* simpleSimulation
  * Updated example to include two versions of the same example simulation; one monolithic and one distributed
* vanadis
  * Added clone3, tgkill syscalls. Vanadis now exits with an error instead of segfaulting if the simulated application segfaults.
  * New support for pthreads when using clang/riscv-gnu-toolchain cross compilers
  * Eliminated extra decodes caused by double looping over the number of decodes
  * Fixed bug in mprotect that could break access to program data
  * Added ability to unmap part of a region
  * Some code reorganization
  * Added some example configurations to a new examples/ directory
  * Added an unimplemented rseq syscall to support newer llvm compilers
  * Minor performance optimization in vanadis registers
  * Added script for automatically regenerating test reference files
  * Added additional checks to replace segfaults with error messages


## SST-Macro
* As of SST 15.0, macro's devel branch is no longer being kept in sync with sst-core's devel branch. The master branches and releases will continue to be kept in sync.
  This means there may be times when the sst-macro and sst-core devel branches do not work together.
* Fix for OTF2 so that Macro no longer automatically uses OTF2 when found in the environment.
* Fixed bug in torus minimal path computation
* Fixed issue with sst-core includes
* There is a known issue compiling SST-Macro with Python 3.13+. This will be fixed in a future release.

# Fixed issues and significant code changes:

## SST-Core
285  serialization support for shared\_ptr, unique\_ptr objects

1223 Interactive Console run <TIME> fails if checkpoint enabled

1277 SimpleNetwork interface does not provide implementation of Handler2 for serialization

1297 timing info in json format

1308 Add script to check for missing #includes and automatically fix them; run them on the latest sources

1311 Add support for shutdown from interactive console

1325 Provide std::this\_thread::yield() as a default definition of sst\_pause()

1327 Default TimeVortex doesn't print its events when a signal is used to trigger a "rt.status.all"

1328 Moved primary component functions into BaseComponent

1329 Add -Wmissing-noreturn and [[noreturn]] to more functions

1330 Add support for general handling of trivially serializable types

1333 core: remove __DATE__ / __TIME__ references from ELI

1334 TimeVortex is no longer serialized during checkpoint/restart

1337 Get rid of GCC false -Wmissing-noreturn warnings; remove unnecessary warning suppression code

1339 Generate JSON timing information at end of simulation

1343 Add serialization of std::valarray

1344 serialize std::variant

1346 Serialize std::optional

1347 std::bitset does not serialize

1348 Serialize std::bitset

1353 Clean up serializer class

1355 Change method for accessing underlying container in priority queue to work around for gcc compiler bug.

1356 Rework Config class to be more concise and to allow for copying of option values based on annotations

1357 Fix ref file for hdf5 stat test

1359 Statistic parameter documentation

1360 sst-core does not compile using --disable-mpi configuration option

1361 macos build failures

1362 Checkpoint config object

1364 ComponentExtension test and checkpoint

1365 Serialize aggregate class types

1366 Relax serialize\_impl rules to allow polymorphism, make has\_serialize\_order trait global to the serializer namespace

1368 inline InstantiateBuilderInfo<Base, T>::loaded variable

1369 Rewrite expressions with volatile to avoid C++20 deprecation warnings

1370 Checkpoint global data into single globals file

1375 Various fixes discovered during element library checkpointing

1378 Print diff on failure for tests that generate them

1379 sst\_unittest\_support.py: Explicitly check for wget availability

1380 Use print macro for SimTime\_t

1381 Data race in params.cc

1382 core: add additional locking in params.cc

1383 Remove existing and prevent future trailing whitespace

1385 GitHub Actions: Initial addition of free macOS runners

1387 core: fix bug in tokenize() function

1388 remove reference in pointer type test

1391 Python output fails to correctly instantiate links

1394 Convert array sizes in one place, flagging errors; cleanup packing code

1395 Simplify serialization of std::variant by using std::array with bounds check

1396 Serialize std::unique\_ptr

1398 Move initialization of units in UnitAlgebra to main()

1402 Update `.git-blame-ignore-revs`

1403 pre-commit failing on PRs merging `devel` into `master`

1404 Add missing unregisterClock definition in baseComponent

1405 Reorganize code into regions in main()

1407 Commit b8b85a8 Induces Build Failure

1408 Skip hdf5 tests for parallel runs for now

1411 JSON reader currently limited to 32bit JSON file parsing

1412 core: increase timeout for os\_command

1414 Expose MPI\_Comm\_spawn\_multiple to elements

1417 Optimize statistics memory and config time

1418 JSON Reader & Writer Rewrite

1421 GitHub Actions: fix location of elements repository

1422 Missing include error on latest sst-core

1423 fix #includes

1424 Configlink memory optimizations

1427 Added the ability to a serial restart of a parallel checkpoint

1429 Remove warnings when MPI is disabled

1430 Cleanup container serialization code

1431 Added new model generator for checkpoint restarts and reorganized code in preparation for repartitioned restarts.

1434 Fix missing #include in core test

1436 Allow std::unique\_ptr serialization to interoperate with raw pointer serialization

1437 Made filenames in checkpoint manifest (.sstcpt) relative to location of manifest file, making it easier to relocate the checkpoints for restart.

1438 Update pre-commit versions

1442 Make test-includes.pl script run 10x faster and find more cases of missing #includes, plus print helpful instructions

1443 Remove unused commented code from test framework

1444 Test framework: read generic `/etc/os-release`

1445 Test framework: deprecate passing bytes to `os_awk_print`

1446 Compiler error (when building a component) in SST serialization header when using CCE

1447 Work around bug in some compilers which do not expand lambda expressions across parameter packs correctly

1448 Suppress spurious unused variable warnings which are conditionally used in `if constexpr`

1449 Remove dependence on Perl Memoize module

1450 Allow paths to be specified on the command line for testing missing #includes

1451 Added SST\_ELI\_IS\_CHECKPOINTABLE() macro

1452 Interactive Console Major Update

1453 GitHub Actions-based tests are failing with Python 3.14

1454 GitHub Actions: Pin Python versions for macOS runners

1455 Add statistics to port modules

1456 Remove cmake-format from GitHub actions

1457 adding missing header file to appease test-includes script

1458 Refine test-includes.pl to print more precise error messages about compiler requirements; work with more versions of GCC

1459 Fix missing #includes in interactive debugger

1460 Fix compilation errors due to compiler / libraries bugs by always using workarounds

1461 lib-and-graph-fix

1463 Fixed a couple bugs and a compile warning:

1465 Fix more missing headers

1468 Various release fixes

1470 Fix missing lock\_guard in SharedSet::write()

1471 Fixes for TimeVortex

1472 Use SST\_SER() and SST\_SER\_NAME() macros instead of calling sst\_ser\_object()

## SST-Elements

2439 Vanadis examples 2

2493 Developed a new Ariel Frontend that use EP Analytics tools to collect the address stream

2499 Remove existing and prevent future trailing whitespace

2507 Clang 18.1.8 does not compile with Intel PIN 3.31

2510 Allow compiling using Clang on Linux with Intel Pin

2516 Fix scanf() format mismatch; move side effects outside of assert()

2518 Ignore \*.mem files produced in memH tests

2519 Mercury: fix race condition

2521 Add guards to the declaration of OneShot::Handler in SimpleCarWash

2524 Incorrect dependency for ramulator2 on macOS

2526 install arielapi.h

2527 Mercury: restore multithread tests

2528 Don't skip test\_testme() if threads > 1

2531 Mercury: overhaul library/app loading and eliminate direct dlopen usage

2535 Issue-fix: Updated bytes sent metric in Ember spyplot

2536 Update SimpleSimulation example

2537 Fix so golem test does not need sst-info in PATH

2539 fix format specifier to remove warning

2540 MemH checkpoint and code clean up

2544 Remove LLVM build requirement from llyr

2546 mercury: add support for app arguments

2547 mercury: add "argv" to pymercury params

2548 small logic fix found during mi300 debugging

2550 mask-mpi: pass parameters to halo3d

2552 mercury: reduce nodes down to one

2555 Allreduce crashes when count>119

2557 alloc array, fixes segfault

2559 Minor updates

2563 Performance optimizations in MemH and Vanadis

2564 Fix debug builds

2568 Vanadis: Add dummy rseq syscall for newer llvm compilers

2569 Fix empty array and string parameter passing to MemNIC

2572 fixing uniform targetgen in sst-merlin

2573 Vanadis syscalls and cleanup

2578 GitHub Actions: Initial addition of free macOS runners

2579 Compile error on latest sst-core

2580 Miranda: ref update related to stat fix in sst-core

2582 Error in Dragonfly topology init events propagation between groups

2586 Fix null pointer format string passed to Output::fatal()

2587 GitHub Actions: Pin Python versions for macOS runners

2589 MemH: Fix possibly uninitialized stats for checkpoint

2590 Modify mercury installation to support sst-info fix

2591 Update balar CI Infrastructure

2593 rdmaNIC: Fix ref file that was missed in earlier PR

2594 Remove some compiler warnings

2595 Update merlin System object to account for allocation\_block\_size when filling in extra nodes with EmptyJob.

2596 Fix warnings due to TimeConverter change


## SST-Macro

730	Fix bug in torus minimal path computation

734	fix bug in otf2 configuration

735	Fix otf2 env configuration

736	Stop breaking core includes

737	fix otf2 bugs

