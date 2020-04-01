# SST Testing Frameworks

## Overview
This frameworks is built upon Python's unittest module and modified to operate for SST.  It is designed to operate on Python 2.7 - Python 3.x.  No 3rd party modules are required, however to enable (optional) concurrent testing, the `testtools` module must be pip installed.

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
     * -c = Run tests concurrently.  (SEE Concurrently BELOW)
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

## Running Tests Concurrently
   * Tests may be run concurrently in multiple threads 
   * This requires a 3rd party module called ```testtools`
      * To install test tools, ```> pip install testtools```
 
## Scenarios
   * Tests and TestCases identified in testsuites can be skipped from running by using the '--scenarios' argument.  1 or more scenarios can be defined concurrently.
   * The decision to skip is made in the testsuite source code.

