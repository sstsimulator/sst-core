# SST Testing Frameworks

## Overview
This frameworks is built upon Python's unittest module and modified to operate for SST.  It is designed to operate on Python 2.7 - Python 3.x.  No 3rd party modules are required, however to enable (optional) concurrent testing, the `testtools` module must be pip installed.  

When the test system is run, it will automatically discover testsuites (this is can be controlled by the user).  Each testsuite can have multiple tests.  All tests (of all discovered testsuites) will be given the opportunity to run, however some build settings, environment conditions or scenarios may require tests to be skipped.  The decision to skip a test is performed by the test code.

There is no guarantee on the order of Testsuites being run, however, all tests within a testsuite will be run before the next testsuite is started (see concurrent testing for exceptions).  Note: Tests within a test suite are ALSO not guaranteed to run in a specific order.  

## Building and Installing the SST Testing Frameworks

1. Build and install SST-Core normally. 
   * Installing SST-Core will copy the required Testing Frameworks components into the install directory of SST.  
   * At this point, ```sst-test-core``` can be run. 

1. Build and install SST-Elements normally
   * At this point, ```sst-test-elements``` can be run. 

## Development of SST Testing FrameWorks components
   * If development of the SST Testing Frameworks components is desired, the developer must build and install SST-Core with the --enable-testframework-dev configuration flag.
      * This will create symbolic links to the SST Testing Frameworks components (instead of copying them), thereby allowing the developer to modify the scripts and test without having to re-copy the file to the SST-Core install directory.
    
## Running SST Testing Frameworks
  * To test SST-Core, run ```sst-test-core```
  * To test SST-Elements, run ```sst-test-elements```
  * Parameter help can be seen by ```sst-test-core --help```
  * Run-time Parameters:
     * -h = help
     * -f = Fail Fast. Testing will stop on the first detected failure; default = false
     * -o dir = Output directory path. Where test run results are stored; default = ./sst_test_outputs
     * -k = Keep output.  Normally, output directory is deleted before run to ensure clean results, setting this flag will prevent delete of output directory; default = false
     * -c xx = Run tests concurrently; default = 8  (SEE Concurrent Testing BELOW)
     * -v, -q, -d - Screen Output mode as verbose, quiet or debug.  
        * If not defined, screen output is normal (dots indicate successful tests)
        * -q = quiet output, minimal information is displayed on screen
        * -v = Verbose output, tells user what test is running
        * -d = Debug output, outputs test specific debug data to the screen
     * -s = Scenarios name (SEE Scenarios BELOW); default = NONE
     * -r XX = Number of ranks to use during SST runs; default = 1
     * -t YY = Number of threads to use during SST runs; default = 1
     * -y name = Name of testsuites to discover (SEE Discovery BELOW); default = "default"
     * -w name = Wildcard name of testsuites to discover (SEE Discovery BELOW); default = ""
     * -p path = Path to testsuites (SEE Discovery BELOW); default = registered tests dir paths
     
## Discovery of Tests
   * Testsuite files live in the `tests` subdirectory under core and elements and are named `testsuite_<testtype>_<testsuitename>`
      * `testsuite_` is the required prefix for all testsuites.  The testengine will only find scripts with this prefix.
      * `<testsuitename>` is the general name of the testsuite and can be of any text.  It does not have to match the element name.
      * `<testtype>` is a catagory of test types.  A test suites with a type name of `"default"` will be run under normal conditions.  This allows the user to create specialized test suites that are not normally run, but perform other test functions.
   * Finding TestSuites:
      * During Startup, the -p and -y (or -w) arguments are used to create a list of testsuites to be run.
      - If the -p argument includes a testsuite file, that testsuite file will be directly added to the list of testsuites to be run.  Multiple files can be defined.  
         * Example: `sst-test-elements -p <PathToMerlinTests>/testsuite_default_merlin.py <PathToMerlinTests>/testsuite_extra_merlin.py`
      - If the -p argument includes a directory (containing 1 or more testsuites), that directory will be searched for specific testsuites as described below.  Muliple paths can be defined.  
         * Example: `sst-test-elements -p <PathToMerlinTests> <PathToSambaTests>`
      - If the 'list_of_paths' argument is empty (by default), test directory paths found in the sstsimulator.conf file (located in the <sstcore_install>/etc directory) will be searched for specific testsuites as described below.

   * Searching for testsuites by type or wildcard:
      * Each directory identified by the -p argument will search for testsuites based upon the -y or -w (mutually exclusive) argument as follows:
      - Files named 'testsuite_default_*.py' will be added to the list of testsuites to be run if argument **-y AND -w** are both **NOT** specified. Note: This will run only the 'default' set of testsuites in the directory.
      - Files named 'testsuite_<type_name>_*.py' will be added to the list of testsuites to be run when <type_name> is specifed using the `-y = <type_name>` argument. Note: This will run user selected set of testsuites in the directory. 
         * Example: ```-y = autotester```
      - Files named 'testsuite_*.py' will be added to the list of testsuites to be run when argument `-y = all` is specified. Note: This will run ALL of the testsuites in the directory.
      - Files named 'testsuite_<wildcard_name>.py' will be added to the list of testsuites to be run when <wildcard_name> is specifed using the -w = <wildcard_name> argument. Note: This will run user selected set of testsuites in the directory. 
         * Example: ```-w = *merlin*```

## Running Tests Concurrently
   * Tests may be run concurrently in multiple threads to significantly reduce testing runtime.
   * This requires a 3rd party python module called ```testtools```
      * To install test tools, ```> pip install testtools```
      * If ```testtools``` is not installed, concurrent testing is not available.
   * Tests are run in multiple threads (default = 8) 
   * There is no guarantee of test run order, and tests from multiple testsuites may be running concurrently.
   * Debug output mode cannot be used with concurrent testing (debug output is not thread-safe)  
 
## Scenarios
   * Specific tests in testsuites can be skipped from running by using the '--scenarios' argument.  
      * Some tests are not desired to be run under some circumstances (ie reduced tests for the Autotester).
   * 1 or more scenarios can be defined concurrently.
   * The decision to skip is made in the testsuite source code.

## Creating new testsuites
  * In general, testsuites are named `testsuite_XXX_YYY.py` where XXX = The test_type name, and YYY is the general name of the testsuite.  These testsuites should live the tests directory of an element
     - test_type name is normally `default`.  All testsuites with a test_type of `default` will be run under normal run conditions.  specialized testsuites may be created with a unique test_type name and will only be run if the -y option identifies that name (or `all`).
     - Test directories must be in a registered path for the SST Test Frameworks to automatically discover.  See Makefile.am files of various elements on how to register.
     - 3rd party elements can also register their test paths to the SST Test Frameworks to enable testing. 
     - Testsuites are python files that contain classes derived from SSTTestCase (defined in module sst_unittest)
        - Numerous support functions exist in the the modules sst_unittest and sst_unittest_support to assist with testing.
     - It is recommended that the developer look at examples of other testsuites to see implementation details.
  * **SST-Core**
     * SST-Core tests require the support of test elements.  These elements are simular to normal elements defined in SST-Elements.  The core test elements are located in `<core source dir>/src/sst/core/testElements`
     * SDL Testfiles are located in `<core source dir>/src/sst/core/tests`
     * Reference Ouput file for the SDL Tests files located in `<core source dir>/src/sst/core/tests/refFiles`
     * Testscripts that run the SDL Testfiles are located in `<core source dir>/src/sst/core/tests`     
 * **SST-Elements**
     * SDL Testfiles are located in `<elements source dir>/src/sst/elements/<element>/tests`
     * Reference Ouput file for the SDL Tests files located in `<elements source dir>/src/sst/elements/<element>/tests/refFiles`
     * Testscripts that run the SDL Testfiles are located in `<elements source dir>/src/sst/elements/<element>/tests`
