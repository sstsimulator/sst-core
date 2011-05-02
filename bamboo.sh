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


#-------------------------------------------------------------------------
# Function: defaultaction
# Description:
#   Purpose: Default action for Bamboo script.
#   Input: none
#   Output: none
#   Return value: 0 if success, or error at step.
defaultaction() {

    # autogen to create ./configure
    ./autogen.sh
    retval=$?
    if [ $retval -ne 0 ]
    then
        return $retval
    fi

    # configure SST
    ./configure --prefix=/usr/local --with-boost=/usr/local --with-zoltan=/usr/local --with-parmetis=/usr/local
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

#-------------------------------------------------------------------------
# Function: powertherm_test
# Description:
#   Purpose: Configure and build with DRAMSim2.
#   Input: none
#   Output: none
#   Return value: 0 if success
powertherm_test() {

    # autogen to create ./configure
    ./autogen.sh
    retval=$?
    if [ $retval -ne 0 ]
    then
        return $retval
    fi

    # configure SST with power and thermal options enabled
    ./configure --prefix=/usr/local --with-boost=/usr/local --with-zoltan=/usr/local --with-parmetis=/usr/local --with-McPAT=/usr/local/lib --with-hotspot=/usr/local/lib --with-orion=/usr/local/lib
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

#-------------------------------------------------------------------------
# Function: dramsim_test
# Description:
#   Purpose: Configure and build with DRAMSim2.
#   Input: none
#   Output: none
#   Return value: 0 if success
dramsim_test() {

    # autogen to create ./configure
    ./autogen.sh
    retval=$?
    if [ $retval -ne 0 ]
    then
        return $retval
    fi

    # configure SST with DRAMSim2
    ./configure --prefix=/usr/local --with-boost=/usr/local --with-zoltan=/usr/local --with-parmetis=/usr/local --with-dramsim=/usr/local
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

#-------------------------------------------------------------------------
# Function: customaction1
# Description:
#   Purpose: Custom action example.
#   Input: none
#   Output: none
#   Return value: 0 if success
# customaction1() {
#     # Example template for custom build actions
#     # If a step fails, exit at that point, return with the error code.
# }


#=========================================================================

retval=0

if [ $# -ne 1 ]
then
    echo "Usage : $0 <action>"
    retval=0

elif [ $1 = "default" ]
then
    defaultaction
    retval=$?

elif [ $1 = "DRAMSim_test" ]
then
    # Build SST with DRAMSim2 options enabled
    dramsim_test
    retval=$?

elif [ $1 = "PowerTherm_test" ]
then
    # Build SST with Power and Thermal options enabled
    powertherm_test
    retval=$?

# elif [ $1 = "custom1" ]
# then
#     # example custom action 1
#     customaction1
#     retval=$?
#
# elif [ $1 = "custom2" ]
# then
#     # example custom action 2
#     customaction2
#     retval=$?
else
    echo "$0 : unknown action \"$1\""
    retval=1
fi

if [ $retval -ne 0 ]
then
    echo "$0 : exit failure."
else
    echo "$0 : exit success."
fi

exit $retval
