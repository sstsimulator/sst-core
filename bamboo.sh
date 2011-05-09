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
# functions

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
    baseoptions="--prefix=/usr/local --with-boost=/usr/local --with-zoltan=/usr/local --with-parmetis=/usr/local"

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

            configStr="$baseoptions --with-boost-mpi --with-dramsim=no --with-disksim=/usr/local/$disksimdir --no-recursion $disksimenv"
            ;;
        PowerTherm_test)
            configStr="$baseoptions --with-McPAT=/usr/local/lib --with-hotspot=/usr/local/lib --with-orion=/usr/local/lib"
            ;;
        default|*)
            configStr="$baseoptions --with-dramsim=/usr/local"
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
            configline=`getconfig $1 $arch`
#            echo "generated config line: $configline"
            dobuild $configline
            retval=$?
            ;;

        *)
            echo "$0 : unknown action \"$1\""
            retval=1
            ;;
    esac
fi

if [ $retval -ne 0 ]
then
    echo "$0 : exit failure."
else
    echo "$0 : exit success."
fi

exit $retval
