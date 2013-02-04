#!/bin/bash
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

#	This assumes a directory strucure
#                     SST_BASE   (was $HOME)
#           devel                sstDeps
#           trunk (SST_ROOT)       src

if [[ ${SST_DEPS_USER_MODE:+isSet} = isSet ]]
then
    export SST_BASE=$SST_DEPS_USER_DIR
else
    export SST_BASE=$HOME
fi

# Location of SST library dependencies (deprecated)
export SST_DEPS=${SST_BASE}/local
# Location where SST files are installed
export SST_INSTALL=${SST_BASE}/local
# Location where SST build files are installed
export SST_INSTALL_BIN=${SST_INSTALL}/bin

# Location where SST dependencies are installed. This only specifies
# the root; dependencies may be installed in various locations under
# this directory. The user can override this value by setting the
# exporting the SST_INSTALL_DEPS_USER variable in their environment.
export SST_INSTALL_DEPS=${SST_BASE}/local
# Initialize build type to null
export SST_BUILD_TYPE=""
# Load test definitions
echo "bamboo.sh: This directory is:"
pwd
echo "bamboo.sh: ls test/include"
ls test/include
echo "bamboo.sh: ls deps/include"
ls deps/include
echo "bamboo.sh: Sourcing test/include/testDefinitions.sh"
. test/include/testDefinitions.sh
echo "bamboo.sh: Done sourcing test/include/testDefinitions.sh"
# Load dependency definitions
echo "bamboo.sh: deps/include/depsDefinitions.sh"
. deps/include/depsDefinitions.sh
echo "bamboo.sh: Done sourcing deps/include/depsDefinitions.sh"

# Uncomment the following line or export from your environment to
# retain binaries after build
#export SST_RETAIN_BIN=1
#=========================================================================
#Functions
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

    if [ $kernel != "Darwin" ]
    then
        # Only run if the OS *isn't* Darwin (MacOS)
        ${SST_TEST_SUITES}/testSuite_macsim.sh
        ${SST_TEST_SUITES}/testSuite_zesto.sh
        ${SST_TEST_SUITES}/testSuite_zesto_qsimlib.sh
    fi



    if [ $1 != "iris_test" ]
    then
        ${SST_TEST_SUITES}/testSuite_portals.sh
        # jwilso: running simpleComponent test here temporarily
        ${SST_TEST_SUITES}/testSuite_simpleComponent.sh
    fi

    if [ $1 == "PowerTherm_test" ]
    then
        ${SST_TEST_SUITES}/testSuite_PowerTherm.sh
    fi

    if [ $1 == "portals4_test" ]
    then
        ${SST_TEST_SUITES}/testSuite_PowerTherm.sh
    fi

    ${SST_TEST_SUITES}/testSuite_iris.sh

    ${SST_TEST_SUITES}/testSuite_M5.sh
    if [ $1 == "M5_test" ]
    then
        ${SST_TEST_SUITES}/testSuite_M5.sh
        ${SST_TEST_SUITES}/testSuite_M5.sh
        ${SST_TEST_SUITES}/testSuite_M5.sh
    fi

    if [ `find . -name libPhoenixSim.so | wc -w` != 0 ]
    then
        ${SST_TEST_SUITES}/testSuite_phoenixsim.sh
    else
        echo -e  "No PhoenixSim test:  libPhoenixSim.so is not available\n"
    fi

    if [[ $BOOST_HOME == *boost-1.50* ]]
    then
        ${SST_TEST_SUITES}/testSuite_macro.sh
    else
        echo -e "No SST Macro test:    Only test with Boost 1.50"
    fi
    
    #  The following restrictions are not about required dependencies,
    #  but are to only run the lengthy test on one case per OS environment.
echo "BOOST_HOME is ${BOOST_HOME}, MPIHOME is ${MPIHOME}"
    if [[ $BOOST_HOME == *boost-1.50* ]] && [[ "$MPIHOME" == *openmpi-1.4.4* ]]
    then 
echo  "Dotests is invoking the portals4 test"
        ${SST_TEST_SUITES}/testSuite_portals4.sh
    else
echo " Arg in is $1,  kernel is ${kernel} "
## Disable test in nightly on MacOS per Issue #38
#        if [ $1 == "portals4_test" ] || [ $kernel == "Darwin" ]
        if [ $kernel == "Darwin" ]
        then
            echo "Portals4 test is disabled -- see Issue #38"
        fi
        if [ $1 == "portals4_test" ] 
        then
            ${SST_TEST_SUITES}/testSuite_portals4.sh
        fi
    fi
    # Add other test suites here, i.e.
    ${SST_TEST_SUITES}/testSuite_scheduler.sh
    ${SST_TEST_SUITES}/testSuite_patterns.sh
    ${SST_TEST_SUITES}/testSuite_zesto.sh

    # ${SST_TEST_SUITES}/testSuite_moe.sh
    # ${SST_TEST_SUITES}/testSuite_larry.sh
    # ${SST_TEST_SUITES}/testSuite_curly.sh
    # ${SST_TEST_SUITES}/testSuite_shemp.sh
    # etc.
   if [ $1 == "phoenixsim_test" ]
   then 
   ${SST_TEST_SUITES}/testSuite_phoenixsim.sh
   fi
   
   if [ $1 == "macro_test" ]
   then 
   ${SST_TEST_SUITES}/testSuite_macro.sh
   fi

    # Purge SST installation 
    if [[ ${SST_RETAIN_BIN:+isSet} != isSet ]]
    then
        rm -Rf ${SST_INSTALL}
    fi

}


#-------------------------------------------------------------------------
# Function: setConvenienceVars
# Description:
#   Purpose:
#       set convenience vars
#   Input:
#       $1 (depsStr): selected dependencies
#   Output: string containing 'configure' parameters
#   Return value: none
setConvenienceVars() {
    # generate & load convenience variables
    echo "setConvenienceVars() : input = ($1), capturing to SST_deps_env.sh..."
    $SST_DEPS_BIN/sstDependencies.sh $1 queryEnv > $SST_BASE/SST_deps_env.sh
    . $SST_BASE/SST_deps_env.sh
    echo "setConvenienceVars() : SST_deps_env.sh file contents"
    echo "startfile-----"
    cat $SST_BASE/SST_deps_env.sh
    echo "endfile-------"
    echo "setConvenienceVars() : exported variables"
    export | egrep SST_DEPS_
    baseoptions="--disable-silent-rules --prefix=$SST_INSTALL --with-boost=$SST_DEPS_INSTALL_BOOST --without-zoltan"
    echo "setConvenienceVars() : baseoptions = $baseoptions"
}

#-------------------------------------------------------------------------
# Function: getconfig
# Description:
#   Purpose:
#       Based on build config and architecture, generate 'configure'
#       parameters.
#   Input:
#       $1 (build configuration): name of build configuration
#       $2 (architecture): build platform architecture from uname
#       $3 (os): operating system name
#   Output: string containing 'configure' parameters
#   Return value: none
getconfig() {

    # Configure default dependencies to use if nothing is explicitly specified
    local defaultDeps="-k default -d default -p default -z default -b default -g default -m default -i default -o default -h default -s none -q none"

    local depsStr=""

    # Determine MPI wrappers
    local mpicc_compiler=`which mpicc`
    local mpicxx_compiler=`which mpicxx`
    local cc_compiler=`which gcc`
    local cxx_compiler=`which g++`
    local mpi_environment="CC=${cc_compiler} CXX=${cxx_compiler} MPICC=${mpicc_compiler} MPICXX=${mpicxx_compiler}"

    # Interrogate Python install to obtain location of Python includes
    local tmp_python_inc=`python-config --includes`
    local python_inc_dir=`expr "$tmp_python_inc" : '\([[:graph:]]*/python2\..\)'`

    # make sure that sstmacro is suppressed
    if [ -e ./sst/elements/macro_component/.unignore ] && [ -f ./sst/elements/macro_component/.unignore ]
    then
        rm ./sst/elements/macro_component/.unignore
    fi

    # On MacOSX Lion, suppress genericProc
    # On MacOSX Lion, suppress PhoenixSim
    if [ $3 == "Darwin" ]
    then
        echo "$USER" > ./sst/elements/genericProc/.ignore
        echo "$USER" > ./sst/elements/PhoenixSim/.ignore
    fi

    case $1 in
        sst2.3_config)
            #-----------------------------------------------------------------
            # sst2.3_config
            #     This option used for configuring SST with supported 2.3 deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment} CFLAGS=$python_inc_dir CXXFLAGS=$python_inc_dir"
            depsStr="-k none -d 2.2.1 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s 2.4.0 -q stabledevel -M 1.1"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-sstmacro=$SST_DEPS_INSTALL_SSTMACRO  --enable-phoenixsim --with-omnetpp=$SST_DEPS_INSTALL_OMNET --enable-zesto --with-qsim=$SST_DEPS_INSTALL_QSIM $miscEnv"
            ;;
        sst2.2_config)
            #-----------------------------------------------------------------
            # sst2.2_config
            #     This option used for configuring SST with supported 2.2 deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment} CFLAGS=$python_inc_dir CXXFLAGS=$python_inc_dir
"
            depsStr="-k none -d r4b00b22 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s 2.3.0 -q none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-sstmacro=$SST_DEPS_INSTALL_SSTMACRO  --with-omnetpp=$SST_DEPS_INSTALL_OMNET $miscEnv"
            ;;
        non_std_sst2.2_config)
            #-----------------------------------------------------------------
            # non_std_sst2.2_config
            #     This option used for configuring SST with supported 2.2 deps
            #           Using not standard configuration
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment} CFLAGS=$python_inc_dir CXXFLAGS=$python_inc_dir"
            depsStr="-k none -d r4b00b22 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s 2.3.0 -q none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-sstmacro=$SST_DEPS_INSTALL_SSTMACRO  --with-omnetpp=$SST_DEPS_INSTALL_OMNET"
            ;;
        sst2.3_config_macosx)
            #-----------------------------------------------------------------
            # sst2.3_config_macosx
            #     This option used for configuring SST with supported 2.3 deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment} CFLAGS=$python_inc_dir CXXFLAGS=$python_inc_dir"
            depsStr="-k none -d 2.2.1 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s 2.4.0 -q none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-sstmacro=$SST_DEPS_INSTALL_SSTMACRO $miscEnv"
            ;;
        sst2.2_config_macosx)
            #-----------------------------------------------------------------
            # sst2.2_config_macosx
            #     This option used for configuring SST with supported 2.2 deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment} CFLAGS=$python_inc_dir CXXFLAGS=$python_inc_dir"
            depsStr="-k none -d r4b00b22 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s 2.3.0 -q none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-sstmacro=$SST_DEPS_INSTALL_SSTMACRO $miscEnv"
            ;;
        Disksim_test)
            #-----------------------------------------------------------------
            # Disksim_test
            #     This option used for configuring SST for use with DiskSim
            #-----------------------------------------------------------------
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
            depsStr="$defaultDeps"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-boost-mpi --with-dramsim=no --with-disksim=$SST_INSTALL_DEPS/$disksimdir --no-recursion $disksimenv"
            ;;
        PowerTherm_test)
            #-----------------------------------------------------------------
            # PowerTherm_test
            #     This option used for configuring SST with Power and
            #     Therm enabled
            #-----------------------------------------------------------------
            depsStr="-k none -d r4b00b22 -p default -z none -b default -g stabledevel -m default -i default -o default -h default -s none -q none"

            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-McPAT=$SST_DEPS_INSTALL_MCPAT --with-hotspot=$SST_DEPS_INSTALL_HOTSPOT --with-orion=$SST_DEPS_INSTALL_ORION --with-IntSim=$SST_DEPS_INSTALL_INTSIM"
            ;;
        dramsim_test)
            #-----------------------------------------------------------------
            # dramsim_test
            #     This option used for configuring SST with DRAMSim enabled
            #-----------------------------------------------------------------
            depsStr="$defaultDeps"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt"
            ;;
        gem5_test)
            #-----------------------------------------------------------------
            # gem5_test
            #     This option used for configuring SST with gem5 enabled
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d r4b00b22 -p none -z none -b default -g stabledevel -m default -i default -o default -h default -s none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt $miscEnv"
            ;;
        sstmacro_latest_test)
            #-----------------------------------------------------------------
            # sstmacro_latest_test
            #     This option used for configuring SST with latest devel sstmacro
            #-----------------------------------------------------------------
            echo "$USER" > ./sst/elements/macro_component/.unignore
            miscEnv="${mpi_environment}"
            depsStr="-k default -d r4b00b22 -p none -z none -b default -g stabledevel -m default -i default -o default -h default -s stabledevel"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt $miscEnv"
            ;;
        sstmacro_2.2.0_test)
            #-----------------------------------------------------------------
            # sstmacro_2.2.0_test
            #     This option used for configuring SST with sstmacro 2.2.0
            #-----------------------------------------------------------------
            echo "$USER" > ./sst/elements/macro_component/.unignore
            miscEnv="${mpi_environment} CFLAGS=$python_inc_dir CXXFLAGS=$python_inc_dir"
            depsStr="-k none -d r4b00b22 -p none -z none -b default -g stabledevel -m none -i none -o none -h default -s 2.2.0"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt $miscEnv"
            ;;
        sstmacro_2.3.0_test)
            #-----------------------------------------------------------------
            # sstmacro_2.3.0_test
            #     This option used for configuring SST with sstmacro 2.3.0
            #-----------------------------------------------------------------
            echo "$USER" > ./sst/elements/macro_component/.unignore
            miscEnv="${mpi_environment} CFLAGS=$python_inc_dir CXXFLAGS=$python_inc_dir"
            depsStr="-k none -d r4b00b22 -p none -z none -b default -g stabledevel -m none -i none -o none -h default -s 2.3.0"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt $miscEnv"
            ;;
        dramsim_latest_test)
            #-----------------------------------------------------------------
            # dramsim_test
            #     This option used for configuring SST with latest devel DRAMSim 
            #-----------------------------------------------------------------
            depsStr="-k none -d stabledevel -p none -z none  -g stabledevel -m none -i none -o none -h none -s none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt"
            ;;
        boost_1.49_test)
            #-----------------------------------------------------------------
            # dramsim_test
            #     This option used for configuring SST with latest devel DRAMSim 
           #-----------------------------------------------------------------
            miscEnv="${mpi_environment} CFLAGS=$python_inc_dir CXXFLAGS=$python_inc_dir"
            depsStr="-k default -d stabledevel -p none -z none -b 1.49 -g stabledevel -m default -i default -o default -h default -s none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt $miscEnv"
            ;;
        phoenixsim_test)
            #-----------------------------------------------------------------
            # phoenixsim_test
            #     This option used for configuring SST with PhoenixSim enabled
            #-----------------------------------------------------------------
            depsStr="-b default -e default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-omnetpp=${SST_DEPS_SRC_STAGING}/omnetpp-4.1/"
            ;;
        portals4_test|M5_test)
            depsStr="-k none -d none -p none -z none -g stabledevel -m default -i default -o default -h default -s 2.3.0 -4 stabledevel"
            setConvenienceVars "$depsStr"
            configStr="--prefix=$SST_INSTALL --with-boost=$SST_DEPS_INSTALL_BOOST --with-gem5=$SST_BASE/sstDeps/src/staged/sst-gem5-devel.devel/build/X86_SE --with-omnetpp=$SST_DEPS_INSTALL_OMNET --with-sstmacro=$SST_DEPS_INSTALL_SSTMACRO --with-McPAT=$SST_DEPS_INSTALL_MCPAT --with-hotspot=$SST_DEPS_INSTALL_HOTSPOT --with-orion=$SST_DEPS_INSTALL_ORION --with-IntSim=$SST_DEPS_INSTALL_INTSIM"
            ;;
        iris_test)
            depsStr="-k none -d none -p none -z none -g none -m none -i none -o none -h none -s none -4 none -I stabledevel"
            setConvenienceVars "$depsStr"
            configStr="--prefix=$SST_INSTALL --enable-iris --with-boost=$SST_DEPS_INSTALL_BOOST"
            ;;
        simpleComponent_test)
            depsStr="-k none -d none -p none -z none -b 1.43 -g none -m none -i none -o none -h none -s none -4 none -I stabledevel"
            setConvenienceVars "$depsStr"
            configStr="--prefix=$SST_INSTALL --enable-simpleComponent --with-boost=$SST_DEPS_INSTALL_BOOST"
            ;;
        default|*)
            depsStr="$defaultDeps"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM"
            ;;
    esac

    export SST_SELECTED_DEPS="$depsStr"
    export SST_SELECTED_CONFIG="$configStr"
#    echo $configStr
}


#-------------------------------------------------------------------------
# Function: dobuild
# Description:
#   Purpose: Performs the actual build
#   Input:
#     -t <build type>
#     -a <architecture>
#   Output: none
#   Return value: 0 if success
dobuild() {

    # process cmdline options
    OPTIND=1
    while getopts :t:a:k: opt
    do
        case "$opt" in
            t) # build type
                local buildtype=$OPTARG
                ;;
            a) # architecture
                local architecture=$OPTARG
                ;;
            k) #kernel
                local kernel=$OPTARG
                ;;
            *) # unknown option 
                echo "dobuild () : Unknown option $opt"
                return 126 # command can't execute
                ;;
        esac
    done

    export PATH=$SST_INSTALL_BIN:$PATH

    # obtain dependency and configure args
    getconfig $buildtype $architecture $kernel

    # after getconfig is run,
    # $SST_SELECTED_DEPS now contains selected dependencies 
    # $SST_SELECTED_CONFIG now contains config line
    # based on buildtype, configure and build dependencies
    # build, patch, and install dependencies
    $SST_DEPS_BIN/sstDependencies.sh $SST_SELECTED_DEPS cleanBuild
    retval=$?
    if [ $retval -ne 0 ]
    then
        return $retval
    fi

    echo "==================== Building SST ===================="
    export LD_LIBRARY_PATH=${SST_INSTALL_DEPS}/lib:${SST_INSTALL_DEPS}/lib/sst:${SST_DEPS_INSTALL_GEM5SST}:${SST_INSTALL_DEPS}/packages/DRAMSim:${LD_LIBRARY_PATH}
    # Mac OS X needs some help finding dylibs
    if [ $kernel == "Darwin" ]
    then
	    export DYLD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${DYLD_LIBRARY_PATH}
    fi
    # Dump pre-build environment and modules status
    echo "--------------------env--------------------"
    env
    echo "--------------------env--------------------"
    echo "--------------------modules status--------------------"
    module avail
    module list
    echo "--------------------modules status--------------------"

    # autogen to create ./configure
    echo "bamboo.sh: running \"autogen.sh\"..."
    ./autogen.sh
    retval=$?
    if [ $retval -ne 0 ]
    then
        return $retval
    fi
    echo "bamboo.sh: running \"configure\"..."
    echo "bamboo.sh: config args = $SST_SELECTED_CONFIG"

    ./configure $SST_SELECTED_CONFIG
    retval=$?
    if [ $retval -ne 0 ]
    then
        # Something went wrong in configure, so dump config.log
        echo "bamboo.sh: dumping config.log"
        echo "--------------------dump of config.log--------------------"
        sed -e 's/^/#dump /' ./config.log
        echo "--------------------dump of config.log--------------------"
        return $retval
    fi

    echo "bamboo.sh: making SST"
    # build SST
    make all
    retval=$?
    if [ $retval -ne 0 ]
    then
        return $retval
    fi

    # print build and linkage information for warm fuzzy
    echo "SSTBUILD============================================================"
    echo "Built SST with configure string"
    echo "    ./configure ${SST_SELECTED_CONFIG}"
    echo "----------------"
    echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
    if [ $kernel == "Darwin" ]
    then
        # Mac OS X
        echo "DYLD_LIBRARY_PATH=${DYLD_LIBRARY_PATH}"
    fi
    echo "----------------"
    echo "sst exectuable linkage information"

    if [ $kernel == "Darwin" ]
    then
        # Mac OS X 
        echo "$ otool -L ./sst/core/sst.x"
        otool -L ./sst/core/sst.x
    else
        echo "$ ldd ./sst/core/sst.x"
        ldd ./sst/core/sst.x
    fi
    echo "SSTBUILD============================================================"

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

# $1 = build type
# $2 = MPI type
# $3 = boost type
#=========================================================================

echo "==============================ENVIRONMENT DUMP=============================="
env
echo "==============================ENVIRONMENT DUMP=============================="

retval=0
echo  $0  $1 $2 $3
echo `pwd`

if [ $# -ne 3 ]
then
    # need build type and MPI type as argument

    echo "Usage : $0 <buildtype> <mpitype> <boost type>"
    exit 0

else

    # Determine architecture
    arch=`uname -p`
    # Determine kernel name (Linux or MacOS i.e. Darwin)
    kernel=`uname -s`

    echo "bamboo.sh: KERNEL = $kernel"

    case $1 in
        default|PowerTherm_test|sst2.2_config|sst2.3_config|sst2.2_config_macosx|sst2.3_config_macosx|Disksim_test|sstmacro_latest_test|sstmacro_2.2.0_test|dramsim_latest_test|dramsim_test|boost_1.49_test|gem5_test|portals4_test|M5_test|iris_test|simpleComponent_test|phoenixsim_test|macro_test|sstmacro_2.3.0_test|non_std_sst2.2_config)
            # Configure MPI and Boost (Linux only)
            if [ $kernel != "Darwin" ]
            then
                # For some reason, .bashrc is not being run prior to
                # this script. Kludge initialization of modules.
                if [ -f /etc/profile.modules ]
                then
                    . /etc/profile.modules
                fi

                # load MPI
                case $2 in
                    mpich2_stable|mpich2-1.4.1p1)
                        echo "MPICH2 stable (mpich2-1.4.1p1) selected"
                        module unload mpi # unload any default to avoid conflict error
                        module load mpi/mpich2-1.4.1p1
                        mpisuffix="mpich2-1.4.1p1"
                        ;;
                    ompi_1.6_stable|openmpi-1.6)
                        echo "OpenMPI stable (openmpi-1.6) selected"
                        module unload mpi # unload any default to avoid conflict error
                        module load mpi/openmpi-1.6
                        mpisuffix="ompi-1.6"
                        ;;
                    openmpi-1.4.4)
                        echo "OpenMPI (openmpi-1.4.4) selected"
                        module unload mpi # unload any default to avoid conflict error
                        module load mpi/openmpi-1.4.4
                        mpisuffix="ompi-1.4.4"
                        ;;
                    *)
                        echo "Default OpenMPI stable (openmpi-1.6) selected"
                        module unload mpi # unload any default to avoid conflict error
                        module load mpi/openmpi-1.6
                        mpisuffix="ompi-1.6"
                        ;;
                esac

                # load corresponding Boost
                case $3 in
                    boost-1.43)
                        echo "bamboo.sh: Boost 1.43 selected"
                        module unload boost
                        module load boost/boost-1.43.0_${mpisuffix}
                        ;;
                    boost-1.48)
                        echo "bamboo.sh: Boost 1.48 selected"
                        module unload boost
                        module load boost/boost-1.48.0_${mpisuffix}
                        ;;
                    boost-1.50)
                        echo "bamboo.sh: Boost 1.50 selected"
                        module unload boost
                        module load boost/boost-1.50.0_${mpisuffix}
                        ;;
                    *)
                        echo "bamboo.sh: No Valid Boost selected"
                        echo "Third argument was $3"
                        echo "Using Boost-1.50 by default"
                        module unload boost
                        module load boost/boost-1.50.0_${mpisuffix}
                        ;;
                esac
                echo "bamboo.sh: BOOST_HOME=${BOOST_HOME}"
                export SST_DEPS_INSTALL_BOOST=${BOOST_HOME}
                echo "bamboo.sh: SST_DEPS_INSTALL_BOOST=${SST_DEPS_INSTALL_BOOST}"

                # load OMNet++
                module unload omnet++
                module load omnet++/omnet++-4.1_no-mpi 2>__std.err__

                cat __std.err__
                if [[ "`cat __std.err__`" == *ERROR* ]]
                then
                     echo Load of omnet module failed
                     exit
                fi
                echo "bamboo.sh: OMNET_HOME=${OMNET_HOME}"
                export SST_DEPS_INSTALL_OMNET=${OMNET_HOME}
                echo "bamboo.sh: SST_DEPS_INSTALL_OMNET=${SST_DEPS_INSTALL_OMNET}"

            else  # MacOS
                # Initialize modules for Jenkins (taken from $HOME/.bashrc on Mac)
                if [ -f /etc/profile.modules ]
                then
                    . /etc/profile.modules
                # put any module loads here
                    echo "bamboo.sh: Loadin Modules for MacOSX"
                    module add boost/boost-1.50.0
                    module list
                fi

                # Make sure that Mac uses the "new" autotools and can find other utils
                PATH=$HOME/tools/autotools/bin:/opt/openmpi/bin:/opt/local/bin:/usr/bin:$HOME/bin:/usr/local/bin:$PATH; export PATH

                echo "bamboo.sh: MacOS build."
                echo "bamboo.sh:   MPI = $2, Boost = $3"
                echo "bamboo.sh:   MPI and Boost options ignored; using default MPI and Boost per $1 buildtype"
            fi

            # Build type given as argument to this script
            export SST_BUILD_TYPE=$1

            dobuild -t $SST_BUILD_TYPE -a $arch -k $kernel
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
    # as a convenience. SST binaries must be generated before testing.
    dotests $1
fi

if [ $retval -eq 0 ]
then
    echo "$0 : exit success."
else
    echo "$0 : exit failure."
fi

exit $retval
