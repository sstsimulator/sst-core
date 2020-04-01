SST Testing Frameworks

Overview
This frameworks is built upon Python's unittest module and modified to operate for SST.  It is designed to operate on Python 2.7 - Python 3.x.  No 3rd party modules are required, however to enable (optional) concurrent testing, the `testtools` module must be pip installed.

Building and Installing the SST Testing Frameworks

1. Build and install SST-Core normally. 
   * Installing SST-Core will copy the required Testing Frameworks components into the install directory of SST.  
   * At this point, sst-test-core can be run. 

1. Build and install SST-Elements normally
   * At this point, sst-test-elements can be run. 

Development of SST Testing FrameWorks components
   * If development of the SST Testing Frameworks components is desired, the developer must build and install SST-Core with the --enable-testframework-dev configuration flag.
   * This will create symbolic links to the SST Testing Frameworks components (instead of copying them), thereby allowing the developer to modify the scripts and test without having to re-copy the file to the SST-Core install directory.
    
Running SST Testing Frameworks
  * 
