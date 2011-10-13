#!/bin/sh
# bamboo.sh

# Description:

# A shell script to command a build from the Atlassian Bamboo
# Continuous Integration Environment.

# Because there are pre-make steps that need to occur due to the use
# of the GNU Autotools, this script simplifies the build activation by
# consolidating the build steps.

# Assume that fresh code revision has been downloaded by Bamboo from
# the SST Google Code repository prior to invocation of this
# script. Plow through the build, exiting if something goes wrong.

#=========================================================================
# Definitions
#=========================================================================
# Root of directory checked out, where this script should be found
export SST_ROOT=`pwd`
# Location of SST library dependencies
export SST_DEPS=/usr/local
# Location where SST files are installed
export SST_INSTALL=${HOME}/local
# Location where SST build files are installed
export SST_INSTALL_BIN=${SST_INSTALL}/bin
# Initialize build type to null
export SST_BUILD_TYPE=""
# Load test definitions
. test/include/testDefinitions.sh

#=========================================================================
# Functions
#=========================================================================

#-------------------------------------------------------------------------
# Function: dotests
# Description:
#   Purpose:
#       Based on build type and architecture, run tests
#   Input:
#       $1 (build type): kind of build to run tests for
#   Output: none
#   Return value: 0 if success
dotests() {
    # Build type is available as SST_BUILD_TYPE global, if
    # needed to be selective about the tests that are run.

    # NOTE: Bamboo does a fresh checkout of code each time, so there
    # are no residuals left over from the last build. The directories
    # initialized here are ephemeral, and not kept in CM/SVN.

    # Initialize directory to hold testOutputs
    rm -Rf ${SST_TEST_OUTPUTS}
    mkdir -p ${SST_TEST_OUTPUTS}

    # Initialize directory to hold Bamboo-compatible XML test results
    rm -Rf ${SST_TEST_RESULTS}
    mkdir -p ${SST_TEST_RESULTS}

    # Initialize directory to hold temporary test input files
    rm -Rf ${SST_TEST_INPUTS_TEMP}
    mkdir -p ${SST_TEST_INPUTS_TEMP}

    # Run test suites

    # DO NOT pass args to the test suite, it confuses
    # shunit. Use an environment variable instead.
    ${SST_TEST_SUITES}/testSuite_portals.sh
    # Add other test suites here, i.e.
    # ${SST_TEST_SUITES}/testSuite_moe.sh
    # ${SST_TEST_SUITES}/testSuite_larry.sh
    # ${SST_TEST_SUITES}/testSuite_curly.sh
    # ${SST_TEST_SUITES}/testSuite_shemp.sh
    # etc.

    # Purge SST installation
    rm -Rf ${SST_INSTALL}

}


#-------------------------------------------------------------------------
# Function: getconfig
# Description:
#   Purpose:
#       Based on build type and architecture, generate 'configure'
#       parameters.
#   Input:
#       $1 (build type): kind of build to configure for
#       $2 (architecture): build platform architecture from uname
#   Output: string containing 'configure' parameters
#   Return value: none
getconfig() {

    # These base options get applied to every 'configure'
    baseoptions="--disable-silent-rules --prefix=$SST_INSTALL --with-boost=$SST_DEPS --with-zoltan=$SST_DEPS --with-parmetis=$SST_DEPS"

    case $1 in
        Disksim_test)
            # TAU installs in architecture-specific directories
            case $2 in
                x86_64)
                    # 64-bit Linux 
                    disksimdir="x86_64"
                    ;;
                i686)
                    # 32-bit Linux 
                    disksimdir="i386_linux"
                    ;;
            esac

            # Environment variables used for Disksim config
            disksimenv="CFLAGS=-DDISKSIM_DBG CFLAGS=-g CXXFLAGS=-g"

            configStr="$baseoptions --with-boost-mpi --with-dramsim=no --with-disksim=$SST_DEPS/$disksimdir --no-recursion $disksimenv"
            ;;
        PowerTherm_test)
            configStr="$baseoptions --with-McPAT=$SST_DEPS/lib --with-hotspot=$SST_DEPS/lib --with-orion=$SST_DEPS/lib"
            ;;
        default|*)
            configStr="$baseoptions --with-dramsim=$SST_DEPS"
            ;;
    esac

    echo $configStr
}


#-------------------------------------------------------------------------
# Function: dobuild
# Description:
#   Purpose: Performs the actual build
#   Input: string for 'configure' operation
#   Output: none
#   Return value: 0 if success
dobuild() {

    # autogen to create ./configure
    ./autogen.sh
    retval=$?
    if [ $retval -ne 0 ]
    then
        return $retval
    fi

    # configure SST with passed-in string
    echo "bamboo.sh: config args = $*"
    ./configure $*
    retval=$?
    if [ $retval -ne 0 ]
    then
        return $retval
    fi

    # build SST
    make all
    retval=$?
    if [ $retval -ne 0 ]
    then
        return $retval
    fi

    # install SST
    make install
    retval=$?
    if [ $retval -ne 0 ]
    then
        return $retval
    fi

}

#=========================================================================
# main

retval=0

if [ $# -ne 1 ]
then
    # need 1 build type as argument
    echo "Usage : $0 <buildtype>"

else

    # Determine architecture
    arch=`uname -p`

    case $1 in
        default|PowerTherm_test|Disksim_test)
            # Build type given as argument to this script
            export SST_BUILD_TYPE=$1
            configline=`getconfig $1 $arch`
            dobuild $configline
            retval=$?
            ;;

        *)
            echo "$0 : unknown action \"$1\""
            retval=1
            ;;
    esac
fi

if [ $retval -eq 0 ]
then
    # Build was successful, so run tests, providing command line args
    # as a convenience
    dotests $1
fi

if [ $retval -eq 0 ]
then
    echo "$0 : exit success."
else
    echo "$0 : exit failure."
fi

exit $retval
