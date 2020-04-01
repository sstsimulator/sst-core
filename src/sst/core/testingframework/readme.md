# SST Testing Frameworks

## Overview
This frameworks is built upon Python's unittest module and modified to operate for SST.  It is designed to operate on Python 2.7 - Python 3.x.  No 3rd party modules are required, however to enable (optional) concurrent testing, the `testtools` module must be pip installed.  

When the test system is run, it will automatically discover testsuites (this is can be controlled by the user).  Each testsuite can have multiple tests.  All tests (of all discovered testsuites) will be given the opportunity to run, however some build or environment conditions or scenarios may require tests to be skipped.  The decision to skip a test is performed by the test code.

There is no guarantee on the order of Testsuites being run, however, all tests within a testsuite will be run before the next testsuite is started (see concurrent testing for exceptions).  Note: Tests within a test suite are not guaranteed to run in a specific order.  

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
     * -o dir = Output directory path. Where test run results are stored; default = .
     * -k = Keep output.  Normally, output directory is deleted before run to ensure clean results, setting this flag will prevent delete of output directory; default = false
     * -c xx = Run tests concurrently; default = 8  (SEE Concurrently BELOW)
     * -v, -q, -d - Screen Output mode as verbose, quiet or debug.  
        * If not defined, screen output is normal (dots indicate successful tests)
        * -q = quiet output, minimal information is displayed on screen
        * -v = Verbose output, tells user what test is running
        * -d = Debug output, outputs test specific debug data to the screen
     * -s = Scenarios name (SEE Scenarios BELOW); default = NONE
     * -r XX = Number of ranks to use during SST runs; default = 1
     * -t YY = Number of threads to use during SST runs; default = 1
     * -y name = Name of testsuits to discover (SEE Discovery BELOW); default = "default"
     * -p path = Path to testsuites (SEE Discovery BELOW); default = registered tests dir paths
     
## Discovery of Tests
   * Finding TestSuites:
      * During Startup, the 'list_of_paths' and 'testsuite_types' arguments are used to create a list of testsuites to be run.
      - If the 'list_of_paths' argument includes a testsuite file, that testsuite file will be directly added to the list of testsuites to be run.
      - If the 'list_of_paths' argument includes a directory (containing 1 or more testsuites), that directory will be searched for specific testsuites types as described below.
      - If the 'list_of_paths' argument is empty (default), testsuites paths found in the sstsimulator.conf file (located in the <sstcore_install>/etc directory) will be searched for specific testsuite types as described below.

   * Searching for testsuites types:
      * Each directory identified by the 'list_of_paths' argument will search for testsuites based upon the 'testsuite_type' argument as follows:
      - Files named 'testsuite_default_*.py' will be added to the list of testsuites to be run if argument --testsuite_type is NOT specified. Note: This will run only the 'default' set of testsuites in the directory.
      - Files named 'testsuite_<type_name>_*.py' will be added to the list of testsuites to be run when <typename> is specifed using the -y = <type_name> argument. Note: This will run user selected set of testsuites in the directory.
      - Files named 'testsuite_*.py' will be added to the list of testsuites to be run when argument --testsuite_type=all is specified. Note: This will run ALL of the testsuites in the directory.

## Running Tests Concurrently
   * Tests may be run concurrently in multiple threads to significantly reduce testing runtime.
   * This requires a 3rd party python module called ```testtools```
      * To install test tools, ```> pip install testtools```
      * If ```testtools``` is not installed, concurrent testing is not available.
   * Tests are run in multiple threads (default = 8) 
   * There is no guarantee of test run order, and tests from multiple testsuites may be running concurrently.
 
## Scenarios
   * Specific tests in testsuites can be skipped from running by using the '--scenarios' argument.  
      * Some tests are not desired to be run under some circumstances (ie reduced tests for the Autotester).
   * 1 or more scenarios can be defined concurrently.
   * The decision to skip is made in the testsuite source code.

