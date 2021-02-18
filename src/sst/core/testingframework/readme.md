# **SST Testing Frameworks**

---

---

## **Overview**
The SST Testing Frameworks is built upon Python's unittest module and modified for SST.  It is designed to operate on Python 2.7 - Python 3.x  No 3rd party modules are required.

When the Testing Frameworks system is run, it will automatically discover testsuites (this is can be controlled by the user).  Each testsuite can have multiple tests.  All tests will be given the opportunity to run, however some build settings, environment conditions or scenarios may require tests to be skipped.  The decision to skip a test is performed by the specific test code.

There is no guarantee on the order of testsuites being run, however, all tests within a testsuite will be run before the next testsuite is started (see concurrent testing for exceptions).  <br/>Tests within a testsuite are also not guaranteed to run in a specific order.

Optional 3rd party python modules can be added to enable additional features.

   * To enable concurrent testing, the `testtools` module must be pip installed <br/>`(> pip install testtools)`.
   * To enable colorized outputs, the  `blessings` and `pygments` modules must be installed <br/>`(> pip install blessings pygments)`.


---

---

## **Building the Frameworks Documentation**
The SST Testing frameworks documentation is build using `pdoc` and requires Python 3.5+ to be created

   1. Install pdoc on your machine <br/> `> pip3 install pdoc3`

   2. Change directory to the core's testing frameworks source directory and run build_docs.sh <br/> `> cd <sstcore_source>/src/sst/core/testingframework` <br/> `> ./build_docs.sh`

   3. The top-level file of the html documentation will be located at <br/> `<sstcore_source>/src/sst/core/testingframework/html/testingframework/index.html` <br/> Use your favorite browser to view it.

---

## **Building, Installing, and Running the SST Testing Frameworks**
   1. Build and install SST-Core normally.

      * Installing SST-Core will copy the required Testing Frameworks components into the install directory of SST.
      * At this point, ```sst-test-core``` can be run.

   2. Build and install SST-Elements normally
      * At this point, ```sst-test-elements``` can be run.

---

## **Configuration settings when developing SST Testing Frameworks components**
   * If development of the SST Testing Frameworks components is desired, the developer must configure the SST-Core with the `--enable-testframework-dev` configuration flag and then (re)build and install SST-Core.
      * This will create symbolic links to the SST Testing Frameworks components (instead of copying them), thereby allowing the developer to modify the python scripts and test changes without having to re-copy the files to the SST-Core install directory.

---

## **Running SST Testing Frameworks**
  * To test the SST-Core, run `> sst-test-core`
  * To test SST-Elements and other registered elements, run `> sst-test-elements`
  * Help can be shown by `sst-test-core --help` or `sst-test-elements --help`
  * Command line arguments:
     * `-h` = help
     * `-f` = Fail Fast. Testing will stop on the first detected failure; `[False]`
     * `-o` dir = Output directory path. Where test run results are stored; `[./sst_test_outputs]`
     * `-k` = Keep output directory.  Normally, output directory is deleted at startup to ensure clean results, setting this flag will prevent delete of output directory; `[False]`
     * `-c TT` = Run tests concurrently in TT threads; `[8]`  (SEE Concurrent Testing BELOW)
     * `-l` List discovered testsuites instead of running tests; `[False]`
     * `-v`, `-n`, `-q`, `-d` - Screen Output mode as verbose, normal, quiet or debug.
        * If not defined, screen output is verbose
        * `-v` = Verbose output, tells user what test is running `[default]`
        * `-n` = Normal output, indicates minimal test results (dots indicate successful tests)
        * `-q` = Quiet output, minimal information is displayed on screen
        * `-d` = Debug output, outputs test specific debug data to the screen
     * `-z` = Display failure data during test runs (test dependent)
     * `-s = Scenarios name (SEE Scenarios BELOW); `[NONE]`
     * `-r XX` = Number of ranks to use during SST runs; `[1]`
     * `-t YY` = Number of threads to use during SST runs; `[1]`
     * `-y name` = Name of testsuites to discover (SEE Discovery BELOW); `["default"]`
     * `-w name` = Wildcard name of testsuites to discover (SEE Discovery BELOW); `[""]`
        * __Note: Quotes are important around the wildcard name to avoid the shell's automatic wildcard expansion Example: use -w "\*merlin\*" instead of -w \*merlin\*__
     * `-p path` = Path to testsuites (SEE Discovery BELOW); `[<registered tests dir paths>]`
     * `-e name` = Names of specific tests from discovered testsuites to run (SEE Discovery BELOW); `[<registered tests dir paths>]`

---
## **Testsuite File Naming**
   * Testsuite files typically live in the `tests` subdirectory under core and elements and are named `testsuite_<testtype>_<testsuitename>`
      * `testsuite_` is the required prefix for all testsuites.  The testengine will only find testsuites with this prefix.
      * `<testtype>` is a category of test types.  All testsuites with a type name of `"default"` will be run under normal conditions, all other type names will be run only if discovered (see below).  This allows the user to create specialized testsuites that are not normally run but perform other specialized test functions.
      * `<testsuitename>` is the general name of the testsuite and can be of any text.  It does not have to match the element name.
      * Example Testsuite name = `testsuite_default_merlin.py`

---

## **Discovery of Testsuites**
Under normal operation, testsuites are automatically discovered (from registered elements). The user can also control testsuite discovery using various command line arguments as described below. <br/>**NOTE: All discovered testsuites will be run after discovery.**.

  * At startup of the Testing Frameworks, the `-p` and (`-y` or `-w`) arguments are used to create a list of testsuites to be run.
     * `-p` (--list_of_paths)` argument:
        * If the `-p` argument is empty (**default**), test directory paths found in the sstsimulator.conf file (located in the <sstcore_install>/etc directory) will be searched for specific testsuites as described below in "searching for testsuites by type or wildcard".  **NOTE: This is the default operation of the Test Frameworks.**
        * If the `-p` argument includes a specific testsuite file, that testsuite file will be directly added to the list of testsuites to be run.  Multiple files can be defined.
            * Example: `sst-test-elements -p <PathToMerlinTests>/testsuite_default_merlin.py <PathToMerlinTests>/testsuite_extra_merlin.py`
        * If the `-p` argument includes a directory (containing 1 or more testsuites), that directory will be searched for specific testsuites as described below.  Multiple directories can be defined.
            * Example: `sst-test-elements -p <PathToMerlinTests> <PathToSambaTests>`

     * Searching for testsuites by type or wildcard:
        * For each test directory identified by the -p argument (or automatically), the test frameworks will search for testsuites based upon the -y or -w (mutually exclusive) argument as follows:
            * `-y` **AND** `-w` arguments are NOT Specified:
                * All files named `testsuite_default_*.py` will be added to the list of testsuites to be run.  This will run only the 'default' set of testsuites in the directory.
                * **NOTE: This is the default operation of the Test Frameworks.**
            * `-y = <type_name>` :
                * Files named `testsuite_<type_name>_*.py` will be added to the list of testsuites to be run when <type_name> is specified.  This will run user selected set of testsuites in the directory.
                * Example: `-y = autotester`
            * `-y = all` :
                * Files named `testsuite_*.py` will be added to the list of testsuites to be run.  This will run **ALL** of the testsuites in the directory.
            * `-w = <wildcard_name>` :
                * Files named `testsuite_<wildcard_name>.py` will be added to the list of testsuites to be run.  This will run user selected set of testsuites in the directory.
                * Example: `-w = "*merlin*"` - **NOTE: Quotes are important to avoid the shell's automatic wildcard expansion**

---

## **Running Specific Tests**
   * Normally all tests in the discovered testsuites are run.  However, the user may identify tests from the discovered testsuites using the -e ('--test_names') parameter.
      * The names must match the desired testnames exactly.
   * Only the specific tests will be run.

---

## **Test Scenarios**
   * Specific tests in testsuites can be skipped from running by using the '--scenarios' argument.
      * Some tests are not desired to be run under some circumstances (ie reduced tests for the Autotester).
   * 1 or more scenarios can be defined concurrently.
      * Example: `sst-test-elements -s autotester skip_cuda_tests`
   * The decision to skip is made in the testsuite source code.

---

## **Running Tests Concurrently**
   * Tests may be run concurrently in multiple threads to significantly reduce testing runtime.
   * This requires a 3rd party python module called `testtools`
      * To install test tools, `> pip install testtools`
      * If `testtools` is not installed, concurrent testing will not be available.
   * Tests are run in multiple threads (default = 8)
   * There is no guarantee of test run order, and tests from multiple testsuites may be running concurrently.
   * Debug output mode cannot be used with concurrent testing (debug output is not thread-safe)
   * Care must be taken to ensure that number of testing threads * number requested ranks does not exceed number of cores.  Failure to adhere to this can cause unexpected results.
   * Example (3 concurrent testing threads): `sst-test-elements -c 3`


---

## **Creating New Testsuites**
  * **General Info**
     * Testsuites are named `testsuite_XXX_YYY.py` where XXX = The test_type name, and YYY is the general name of the testsuite.  These testsuites should live the tests directory of an element
     * `test_type` name is normally = `default`.  All testsuites with a `test_type` of `default` will be run under normal run conditions.  Specialized testsuites may be created with a unique test_type name and will only be run if the `-y` argument identifies that name (or `all`).
     * Test directories must be in a registered path for the SST Test Frameworks to automatically discover.  See Makefile.am files of various elements on how to register the test directories.
     * 3rd party elements can also register their test paths to the SST Test Frameworks to enable testing.
     * Testsuites are python files that contain classes derived from SSTTestCase (defined in module sst_unittest)
        * Numerous support functions exist in the modules sst_unittest and sst_unittest_support to assist with testing.
     * It is recommended that the developer look at examples of other testsuites to see implementation details.
     * **IT IS IMPARATIVE THAT ANY TESTS DO NOT CHANGE THE WORKING DIRECTORY DURING RUNS**
        * In concurrent testing, multiple tests can potentially be running at the same time.  Changing directories may confuse tests that are generating data files.

  * **SST-Core Tests**
     * SST-Core tests require the support of specialized test elements.  These elements are simular to the normal elements defined in SST-Elements, but are specially crafted for testing the SST-Core.  The core test elements are located in `<core source dir>/src/sst/core/testElements`
     * SDL Testfiles are located in `<core source dir>/src/sst/core/tests`
     * Reference Ouput file for the SDL Tests files located in `<sstcore_source>/src/sst/core/tests/refFiles`
     * Testsuites that run the SDL Testfiles are located in `<sstcore_source>/src/sst/core/tests`

 * **SST-Elements and 3rd Party Tests**
     * SDL Testfiles are typically located in `<elements source dir>/src/sst/elements/<element>/tests`
     * Reference Ouput file for the SDL Tests files are typically located in `<element source dir>/src/sst/elements/<element>/tests/refFiles`
     * Testsuites that run the SDL Testfiles are typically located in `<element source dir>/src/sst/elements/<element>/tests`

---

## **Test Frameworks Design**
 * **Basic Design Concepts**
     * The test frameworks is built upon Pythons unittest infrastructure.
        * No 3rd party Python modules are to be required.  Optional modules may be used to enhance operations (example: concurrent testing via testtools module)
        * Since it is designed to run on Python 2.7 - 3.x; No Python 3.x specific features will be used (unless also implemented for 2.7)
     * Transparent to the general user and easy to use
        * The user does not need to launch the test frameworks via python.
        * All tests can be run from a single command `sst-test-core` or `sst-test-elements`
     * Support for any registered components
        * Any component (i.e. sst-core, sst-elements or 3rd party elements) can register themselves and will be available for testing.  The directory path does not matter.
 * **Core Source Directory**
    * The test frameworks are part of the sst-core and are located at `<sstcore_source>/src/sst/core/testingframework`
       * Test Frameworks Launching scripts
          * `sst-test-core` - Executable python script than loads and runs the `sst-test-engine-loader.py` configured for sst-core testing
          * `sst-test-elements` - Executable python script than loads and runs the `sst-test-engine-loader.py` configured for sst-elements testing
          * `sst-test-engine-loader.py` Python module which loads the main test frameworks infrastructure files and starts the `test_engine.py` module
       * Frameworks Infrastructure
          * `readme.md` - This file
          * `Makefile.inc` - Provides rules for the configure/make system on how to install the test frameworks
          * `sst_unittest.py` - The main python class (derived from python's unittest) for the testsuites to be created from.
          * `sst_unittest_support.py` - Support classes/functions for testsuites to help testsuites operate
          * `sst_unittest_parameterized.py` - Support for parameterized testcases.  See (https://github.com/wolever/parameterized)
          * `test_engine.py` - The main testing frameworks engine.  This provides the entry point for discovery and running testsuites.
          * `test_engine_globals.py` - Globals used by the testing frameworks
          * `test_engine_support.py` - Support classes/functions used by the testing frameworks
          * `test_engine_unittest.py` - Modified flavors of the Python unittest engine (improved useage and reporting) and support for the optional concurrent module from `testtools`
          * `test_engine_junit.py` - Generates JUnit xml data that can be consumed by Jenkins
          * `__init.py__` - Python file to create the SST Test Frameworks as a Python package.  Also supports some pdoc build rules.
          * `build_docs.sh` - Script to build documentation using pdoc
          * `pdoc_template` - Directory containing configuration information used for building the documentation using pdoc.
    * The sst-core also contains `tests` and `testelements` directories which implement the tests (using the test frameworks) for the the sst-core

 * **Install Directories**
    * When the sst-core is built and installed the frameworks are copied into in 2 subdirectories identifed by the `--prefix` configuration setting:
       * `<CoreInstallDir>/bin>` contains the test frameworks launching scripts
       * `<CoreInstallDir>/libexe>` contains the main frameworks infrastructure files

 * **General Operation**
    * COMMING SOON!

----

----

----

