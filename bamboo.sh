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

echo SST_DEPS_USER_MODE = ${SST_DEPS_USER_MODE}
if [[ ${SST_DEPS_USER_MODE:+isSet} = isSet ]]
then
    echo  SST_BASE=\$SST_DEPS_USER_DIR
    export SST_BASE=$SST_DEPS_USER_DIR
else
    echo SST_BASE=\$HOME
    export SST_BASE=$HOME
fi
echo ' ' ; echo "        SST_BASE = $SST_BASE" ; echo ' '

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
###-BEGIN-DOTESTS
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

    if [[ $1 == *sstmainline_config_xml2python* ]]
    then
        . test/utilities/GeneratePythonFromXML
    fi

    # Run test suites

    # DO NOT pass args to the test suite, it confuses
    # shunit. Use an environment variable instead.

    # New CHDL test
    if [[ ${SST_DEPS_INSTALL_CHDL:+isSet} == isSet ]] ; then
        ${SST_TEST_SUITES}/testSuite_chdlComponent.sh
    fi
       

    # if running ALL add VaultSim and Ariel
    if [ $1 == "sstmainline_config_all" ] ; then 
##         ${SST_TEST_SUITES}/testSuite_VaultSim.sh
         ${SST_TEST_SUITES}/testSuite_Ariel.sh
    fi

    #
    #  Run only Streams test only
    #
    if [ $1 == "sstmainline_config_stream" ]
    then
        ${SST_TEST_SUITES}/testSuite_stream.sh
        return
    fi

    #
    #  Run only openMP 
    #
    if [ $1 == "sstmainline_config_openmp" ]
    then
        ${SST_TEST_SUITES}/testSuite_Sweep_openMP.sh
        return
    fi

    #
    #  Run only diropenMP 
    #
    if [ $1 == "sstmainline_config_diropenmp" ]
    then
        ${SST_TEST_SUITES}/testSuite_dirSweep.sh
        return
    fi

    #
    #  Run only dirSweepB
    #
    if [ $1 == "sstmainline_config_diropenmpB" ]
    then
        ${SST_TEST_SUITES}/testSuite_dirSweepB.sh
        return
    fi

    #
    #  Run only dirSweepI
    #
    if [ $1 == "sstmainline_config_diropenmpI" ]
    then
        ${SST_TEST_SUITES}/testSuite_dirSweepI.sh
        return
    fi

    #
    #  Run only dir Non Cacheable
    #
    if [ $1 == "sstmainline_config_dirnoncacheable" ]
    then
        ${SST_TEST_SUITES}/testSuite_dirnoncacheable_openMP.sh
        return
    fi

    #
    #  Run only openMP and memHierarchy 
    #
    if [ $1 == "sstmainline_config_memH_only" ]
    then
        ${SST_TEST_SUITES}/testSuite_openMP.sh
        ${SST_TEST_SUITES}/testSuite_memHierarchy_bin.sh
        return
    fi

    #
    #   Run short list for FAST
    #
    if [ $1 == "sstmainline_config_fast" -o $1 == "sstmainline_config_fast_static" ]
    then
        ${SST_TEST_SUITES}/testSuite_cassini_prefetch.sh
        ${SST_TEST_SUITES}/testSuite_check_maxrss.sh
        ${SST_TEST_SUITES}/testSuite_embernightly.sh
        ${SST_TEST_SUITES}/testSuite_hybridsim.sh
        ${SST_TEST_SUITES}/testSuite_iris.sh
        ${SST_TEST_SUITES}/testSuite_memHierarchy_sdl.sh
        ${SST_TEST_SUITES}/testSuite_merlin.sh
        ${SST_TEST_SUITES}/testSuite_messageGenerator.sh
        ${SST_TEST_SUITES}/testSuite_miranda.sh
        ${SST_TEST_SUITES}/testSuite_portals.sh
        ${SST_TEST_SUITES}/testSuite_prospero.sh
        ${SST_TEST_SUITES}/testSuite_qsimComponent.sh
        ${SST_TEST_SUITES}/testSuite_scheduler.sh
        ${SST_TEST_SUITES}/testSuite_simpleComponent.sh
        ${SST_TEST_SUITES}/testSuite_simpleDistrib.sh
        ${SST_TEST_SUITES}/testSuite_simpleRNG.sh
        ${SST_TEST_SUITES}/testSuite_simpleTracer.sh
        ${SST_TEST_SUITES}/testSuite_SiriusZodiacTrace.sh
        ${SST_TEST_SUITES}/testSuite_sst_mcniagara.sh
        ${SST_TEST_SUITES}/testSuite_sst_mcopteron.sh
        ${SST_TEST_SUITES}/testSuite_VaultSim.sh
        return
     fi

    if [ $kernel != "Darwin" ]
    then
        # Only run if the OS *isn't* Darwin (MacOS)
        ${SST_TEST_SUITES}/testSuite_qsimComponent.sh
#       ${SST_TEST_SUITES}/testSuite_hybridsim.sh
    elif [ $1 == "sstmainline_config_macosx_static" -a $macosVersion == "10.9" ]
    then
    #   Run an extra pass of the mcOpteron test
        ln -s ${SST_TEST_SUITES}/testSuite_sst_mcopteron.sh ${SST_TEST_SUITES}/testSuite_sst_mcopteron2.sh
        ${SST_TEST_SUITES}/testSuite_sst_mcopteron2.sh
    fi
    #
    #   Only run if configured for ariel
    #
    if [ $1 == "sstmainline_config_linux_with_ariel" ]
    then
         ${SST_TEST_SUITES}/testSuite_Ariel.sh
    else
         ${SST_TEST_SUITES}/testSuite_scheduler.sh
    fi

    #
    #    Only if configured for Zoltan
    #
    if [ $1 == "sstmainline_configZ" ] ; then
         ${SST_TEST_SUITES}/testSuite_zoltan.sh
    fi

    #  
    #    Only if macsim was requested
    #

    if [ -d ${SST_ROOT}/sst/elements/macsimComponent ] ; then
         ${SST_TEST_SUITES}/testSuite_macsim.sh
    fi

    ${SST_TEST_SUITES}/testSuite_hybridsim.sh
    ${SST_TEST_SUITES}/testSuite_SiriusZodiacTrace.sh
    ${SST_TEST_SUITES}/testSuite_memHierarchy_sdl.sh
    ${SST_TEST_SUITES}/testSuite_sst_mcopteron.sh
    ${SST_TEST_SUITES}/testSuite_sst_mcniagara.sh


    ${SST_TEST_SUITES}/testSuite_portals.sh
    ${SST_TEST_SUITES}/testSuite_simpleComponent.sh
    ${SST_TEST_SUITES}/testSuite_simpleTracer.sh
    ${SST_TEST_SUITES}/testSuite_miranda.sh

    ${SST_TEST_SUITES}/testSuite_iris.sh

    # if [[ $BOOST_HOME == *boost*1.50* ]]
    # then
    #     ${SST_TEST_SUITES}/testSuite_macro.sh
    # else
    #     echo -e "No SST Macro test:    Only test with Boost 1.50"
    # fi

    echo SST MACRO: $SST_DEPS_INSTALL_SSTMACRO
    if [[ ${SST_DEPS_INSTALL_SSTMACRO:+isSet} = isSet ]]
    then
        ${SST_TEST_SUITES}/testSuite_macro.sh
    fi

    # Add other test suites here, i.e.
    # ${SST_TEST_SUITES}/testSuite_moe.sh
    # ${SST_TEST_SUITES}/testSuite_larry.sh
    # ${SST_TEST_SUITES}/testSuite_curly.sh
    # ${SST_TEST_SUITES}/testSuite_shemp.sh
    # etc.
    ${SST_TEST_SUITES}/testSuite_merlin.sh
    ${SST_TEST_SUITES}/testSuite_embernightly.sh
    ${SST_TEST_SUITES}/testSuite_simpleDistrib.sh
    ${SST_TEST_SUITES}/testSuite_SweepEmber.sh

    if [ $1 != "sstmainline_config_gcc_4_8_1" -a $1 != "sstmainline_config_no_gem5" -a $1 != "sstmainline_config_no_mpi" -a $1 != "sstmainline_config_macosx_no_gem5" ]
    then
        # Don't run gem5 dependent test suites in these configurations
        # because gem5 is not enabled in them.
        ${SST_TEST_SUITES}/testSuite_M5.sh
        ${SST_TEST_SUITES}/testSuite_memHierarchy_bin.sh

        # These also fail in gem5 with CentOS 6.6 (libgomp-4.4.7-11 vs libgomp-4.4.7-4)  
        #         Also fail on current (Jan 21, 2015) TOSS VM
        CentOS_version=`cat /etc/centos-release`
        echo " CentOS Version is ${CentOS_version}"
        if [ "${CentOS_version}" == "CentOS release 6.6 (Final)" ] ; then
           echo " This is CentOS 6.6,  omit running OpenMP tests.  Gem-5 is \"defunct\""
        elif [ "${SST_TEST_HOST_OS_DISTRIB}" == "toss" ] ; then
           echo " This is TOSS,  omit running OpenMP tests.  Gem-5 is \"defunct\""
        else
           ${SST_TEST_SUITES}/testSuite_openMP.sh
           ${SST_TEST_SUITES}/testSuite_diropenMP.sh
           ${SST_TEST_SUITES}/testSuite_stream.sh
           ${SST_TEST_SUITES}/testSuite_noncacheable_openMP.sh
        fi
    fi

    if [ $1 != "sstmainline_config_no_mpi" ] ; then
        #  patterns requires MPI in order to build
        ${SST_TEST_SUITES}/testSuite_patterns.sh
        #  Zoltan test requires MPI to execute.
        #  sstmainline_config_no_gem5 deliberately omits Zoltan, so must skip test.
        if [ $1 != "sstmainline_config_linux_with_ariel" -a $1 != "sstmainline_config_no_gem5" ] ; then
            ${SST_TEST_SUITES}/testSuite_zoltan.sh
        fi
    fi
    ${SST_TEST_SUITES}/testSuite_simpleRNG.sh
#
#      Temporarily disabling the Prospero test  -- see issue #328
#    echo ' ' ; echo " Prospero test disabled -- see Issue #328" ; echo ' '
    echo ' ' ; echo " Prospero test Re-enabled -- November 5th" ; echo ' '
      
    HOST=`uname -n | awk -F. '{print $1}'`

    if [ $HOST == "sst-test" ] ; then
        export SST_BUILD_PROSPERO_TRACE_FILE=1
        pushd ${SST_TEST_SUITES}
          ln -s ${SST_TEST_SUITES}/testSuite_prospero.sh testSuite_prospero_pin.sh
          ${SST_TEST_SUITES}/testSuite_prospero_pin.sh
          unset SST_BUILD_PROSPERO_TRACE_FILE
        popd
    fi
    ${SST_TEST_SUITES}/testSuite_prospero.sh
#
    ${SST_TEST_SUITES}/testSuite_check_maxrss.sh
    ${SST_TEST_SUITES}/testSuite_cassini_prefetch.sh
    ${SST_TEST_SUITES}/testSuite_messageGenerator.sh
    ${SST_TEST_SUITES}/testSuite_VaultSim.sh

    # for now, only run VaultSim test on these special configurations
    if [ $1 == "sstmainline_configA" ] || [ $1 == "sstmainline_config_VaultSim" ] ; then
        if [ $HOST == "sst-test" ] ; then
            ${SST_TEST_SUITES}/testSuite_VaultSim.sh
        fi
    fi

###     ## run only on HardWare
###     
###     if [ $HOST == "sst-test" ] || [ $HOST == "johnslion" ] ; then
###         echo " $HOST: Running the Wall Clock timing test"
###         ${SST_TEST_SUITES}/testSuite_simpleClocker.sh
###     else
###         echo " $HOST: Not Running the Wall Clock timing test"
###     fi
### 
###     ## run above only on HardWare
    
    if [ $1 = "gem5_no_dramsim_config" ]
    then
        # placeholder for tests requiring gem5 with no dramsim
        :  #noop
    fi


    # Purge SST installation
    if [[ ${SST_RETAIN_BIN:+isSet} != isSet ]]
    then
        rm -Rf ${SST_INSTALL}
    fi

}
###-END-DOTESTS

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
    baseoptions="--disable-silent-rules --prefix=$SST_INSTALL --with-boost=$SST_DEPS_INSTALL_BOOST"
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

    # Determine compilers
    local mpicc_compiler=`which mpicc`
    local mpicxx_compiler=`which mpicxx`

    if [[ ${CC:+isSet} = isSet ]]
    then
        local cc_compiler=$CC
    else
        local cc_compiler=`which gcc`
    fi

    if [[ ${CXX:+isSet} = isSet ]]
    then
        local cxx_compiler=$CXX
    else
        local cxx_compiler=`which g++`
    fi

    local mpi_environment="CC=${cc_compiler} CXX=${cxx_compiler} MPICC=${mpicc_compiler} MPICXX=${mpicxx_compiler}"

    # make sure that sstmacro is suppressed
    if [ -e ./sst/elements/macro_component/.unignore ] && [ -f ./sst/elements/macro_component/.unignore ]
    then
        rm ./sst/elements/macro_component/.unignore
    fi

    # On MacOSX Lion, suppress the following:
#    #      PhoenixSim
#    if [ $3 == "Darwin" ]
#    then
#        echo "$USER" > ./sst/elements/PhoenixSim/.ignore
#    fi

    case $1 in
        sstmainline_config) 
            #-----------------------------------------------------------------
            # sstmainline_config
            #     This option used for configuring SST with supported stabledevel deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s none -q 0.2.1 -M 2.0.4 -N default -z 3.8 -c default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-qsim=$SST_DEPS_INSTALL_QSIM --with-glpk=${GLPK_HOME} --with-zoltan=$SST_DEPS_INSTALL_ZOLTAN --with-metis=${METIS_HOME}  --with-chdl=$SST_DEPS_INSTALL_CHDL $miscEnv"
            ;;
        sstmainline_configA) 
            #-----------------------------------------------------------------
            # sstmainline_configA -- temporary for testing with VaultSim
            #    This one should go away  
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s none -q 0.2.1 -M none -N default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-qsim=$SST_DEPS_INSTALL_QSIM $miscEnv --with-libphx=/home/jpvandy/Apr14/libphx/src"
            ;;
        sstmainline_config_VaultSim) 
            #-----------------------------------------------------------------
            # sstmainline_config    -- temporary for testing with VaultSim
            #    This one should be refined or incorporated into something else with dir changed.
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s none -q 0.2.1 -M none -N default -z 3.8"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-qsim=$SST_DEPS_INSTALL_QSIM $miscEnv --with-libphx=$LIBPHX_HOME/src --with-zoltan=$SST_DEPS_INSTALL_ZOLTAN"
            ;;
        sstmainline_configZ) 
            #-----------------------------------------------------------------
            # sstmainline_config    -- temporary for testing Zoltan
            #    This one should be refined or incorporated into something else with dir changed.
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s none -M none -N default -z 3.8"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM  $miscEnv --with-zoltan=$SST_DEPS_INSTALL_ZOLTAN"
            ;;
        sstmainline_config_all) 
            #-----------------------------------------------------------------
            # sstmainline_config
            #     This option used for configuring SST with supported stabledevel deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s none -q 0.2.1 -M 2.0.4 -N default -z 3.8 -c default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-qsim=$SST_DEPS_INSTALL_QSIM --with-glpk=${GLPK_HOME} --with-zoltan=$SST_DEPS_INSTALL_ZOLTAN --with-libphx=$LIBPHX_HOME/src --with-pin=$SST_DEPS_INSTALL_INTEL_PIN --with-metis=${METIS_HOME}  --with-chdl=$SST_DEPS_INSTALL_CHDL$miscEnv"
            ;;
        sstmainline_config_stream|sstmainline_config_openmp|sstmainline_config_diropenmp|sstmainline_config_diropenmpB|sstmainline_config_diropenmpI|sstmainline_config_dirnoncacheable) 
            #-----------------------------------------------------------------
            # sstmainline_config  One only of stream, openmp diropemMP
            #     This option used for configuring SST with supported stabledevel deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s none -q 0.2.1 -M none -N default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-qsim=$SST_DEPS_INSTALL_QSIM $miscEnv"
            ;;
        sstmainline_config_linux_with_ariel) 
            #-----------------------------------------------------------------
            # sstmainline_config_linux_with_ariel
            #     This option used for configuring SST with supported stabledevel deps,
            #     Intel PIN, and Ariel 
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s none -q 0.2.1 -M none -N default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-qsim=$SST_DEPS_INSTALL_QSIM --with-pin=$SST_DEPS_INSTALL_INTEL_PIN --with-metis=${METIS_HOME} $miscEnv"
            ;;
        sstmainline_config_no_gem5) 
            #-----------------------------------------------------------------
            # sstmainline_config_no_gem5
            #     This option used for configuring SST with supported stabledevel deps
            #     Some compilers (gcc 4.7, 4.8, intel 13.4) have problems building gem5,
            #     so this option removes gem5 in order to evaluate the rest of the build
            #     under those compilers.
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g none -m none -i none -o none -h none -s none -q 0.2.1 -M none -N default -z 3.8 -c default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-qsim=$SST_DEPS_INSTALL_QSIM --with-glpk=${GLPK_HOME} --with-zoltan=$SST_DEPS_INSTALL_ZOLTAN --with-metis=${METIS_HOME} --with-chdl=$SST_DEPS_INSTALL_CHDL $miscEnv"
            ;;

        sstmainline_config_no_mpi|sstmainline_config_fast) 
            #-----------------------------------------------------------------
            # sstmainline_config
            #     This option used for configuring SST with supported stabledevel deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="CC=${cc_compiler} CXX=${cxx_compiler}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g none -m none -i none -o none -h none -s none -q 0.2.1 -M none -N default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-qsim=$SST_DEPS_INSTALL_QSIM $miscEnv --disable-mpi"
                      #The following is wrong way to do it, but patterns shouldn't need MPI to build!
            touch $SST_ROOT/sst/elements/patterns/.ignore
            ;;

        sstmainline_config_gem5_gcc_4_6_4) 
            #-----------------------------------------------------------------
            # sstmainline_config
            #     This option used for configuring SST, forcing gem5 to be built with gcc-4.6.4
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g gcc-4.6.4 -m none -i none -o none -h none -s none -q 0.2.1 -M none -N default -z 3.8 -c default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-qsim=$SST_DEPS_INSTALL_QSIM --with-glpk=${GLPK_HOME} --with-zoltan=$SST_DEPS_INSTALL_ZOLTAN --with-metis=${METIS_HOME} --with-chdl=$SST_DEPS_INSTALL_CHDL $miscEnv"
            ;;

        sstmainline_config_gcc_4_8_1) 
            #-----------------------------------------------------------------
            # sstmainline_config_gcc_4_8_1
            #     This option used for configuring SST with supported stabledevel deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g none -m none -i none -o none -h none -s none -q 0.2.1 -M none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions  --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-qsim=$SST_DEPS_INSTALL_QSIM $miscEnv"
            ;;
        sstmainline_config_static) 
            #-----------------------------------------------------------------
            # sstmainline_config_static
            #     This option used for configuring SST with supported stabledevel deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s none -q 0.2.1 -M 2.0.4 -N default -z 3.8"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-qsim=$SST_DEPS_INSTALL_QSIM --with-glpk=${GLPK_HOME} --enable-static --disable-shared --with-zoltan=$SST_DEPS_INSTALL_ZOLTAN --with-metis=${METIS_HOME} $miscEnv"
            ;;

        sstmainline_config_fast_static) 
            #-----------------------------------------------------------------
            # sstmainline_config_fast_static
            #     This option used for quick static run omiting many options
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="CC=${cc_compiler} CXX=${cxx_compiler}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g none -m none -i none -o none -h none -s none -q 0.2.1 -M none -N default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-qsim=$SST_DEPS_INSTALL_QSIM --enable-static --disable-shared $miscEnv --disable-mpi"
                      #The following is wrong way to do it, but patterns shouldn't need MPI to build!
            touch $SST_ROOT/sst/elements/patterns/.ignore
            ;;

        sstmainline_config_clang_core_only) 
            #-----------------------------------------------------------------
            # sstmainline_config_clang_core_only
            #     This option used for configuring SST with no deps to build the core with clang
            #-----------------------------------------------------------------
            depsStr="-k none -d 2.2.2 -p none -z none -b none -g none -m none -i none -o none -h none -s none -q none -M none -N default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM"
            ;;
        sstmainline_config_macosx) 
            #-----------------------------------------------------------------
            # sstmainline_config_macosx
            #     This option used for configuring SST with supported stabledevel deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s none -q none -z 3.8 -N default -M 2.0.4"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-glpk=${GLPK_HOME} --with-zoltan=$SST_DEPS_INSTALL_ZOLTAN --with-metis=${METIS_HOME} $miscEnv"
            ;;
        sstmainline_config_macosx_no_gem5) 
            #-----------------------------------------------------------------
            # sstmainline_config_macosx_no_gem5
            #     This option used for configuring SST with supported stabledevel deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z 3.8  -b 1.50 -g none -m none -i none -o none -h none -s none -q none -N default -c default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions ${MTNLION_FLAG} --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-glpk=${GLPK_HOME} --with-zoltan=$SST_DEPS_INSTALL_ZOLTAN --with-metis=${METIS_HOME} --with-chdl=$SST_DEPS_INSTALL_CHDL $miscEnv"
            ;;
        sstmainline_config_macosx_static) 
            #-----------------------------------------------------------------
            # sstmainline_config_macosx_static
            #     This option used for configuring SST with supported stabledevel deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s none -q none -z 3.8 -N default -M 2.0.4"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-glpk=${GLPK_HOME} --enable-static --disable-shared --with-zoltan=$SST_DEPS_INSTALL_ZOLTAN --with-metis=${METIS_HOME} $miscEnv"
            ;;
        sstmainline_config_static_macro_devel) 
            #-----------------------------------------------------------------
            # sstmainline_config_static_macro_devel
            #     This option used for configuring SST with supported stabledevel deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s stabledevel -q 0.2.1 -M none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-sstmacro=$SST_DEPS_INSTALL_SSTMACRO  --with-qsim=$SST_DEPS_INSTALL_QSIM --enable-static --disable-shared $miscEnv"
            ;;
        sstmainline_sstmacro_xconfig) 
            #-----------------------------------------------------------------
            # sstmainline_sstmacro_xconfig
            #     This option used for configuring SST with sstmacro latest mainline UNSTABLE
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s stabledevel -q 0.2.1 -M none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-sstmacro=$SST_DEPS_INSTALL_SSTMACRO  --with-qsim=$SST_DEPS_INSTALL_QSIM $miscEnv"
            ;;
        sstmainline_config_xml2python) 
            #-----------------------------------------------------------------
            # sstmainline_config_xml2python
            #     This option used for verifying the auto generation of Python input files
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s none -q 0.2.1 -M none -N default -z 3.8"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-glpk=${GLPK_HOME} --with-qsim=$SST_DEPS_INSTALL_QSIM $miscEnv --with-zoltan=$SST_DEPS_INSTALL_ZOLTAN"
            ;;
        sstmainline_config_xml2python_static) 
            #-----------------------------------------------------------------
            # sstmainline_config_xml2python
            #     This option used for verifying the auto generation of Python input files
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s none -q 0.2.1 -M none -N default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM --with-glpk=${GLPK_HOME} --with-qsim=$SST_DEPS_INSTALL_QSIM --enable-static --disable-shared $miscEnv"
            ;;

        # ====================================================================
        # ====                                                            ====
        # ====  3.1.x build configurations start here                     ====
        # ====                                                            ====
        # ====================================================================
        sst3.1_config) 
            #-----------------------------------------------------------------
            # sst3.1_config
            #     This option used for configuring SST with supported deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g SST-3.1.0 -m none -i none -o none -h none -s none -q 0.2.1 -M none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-qsim=$SST_DEPS_INSTALL_QSIM $miscEnv"
            ;;

        sst3.1_config_with_sstdevice) 
            #-----------------------------------------------------------------
            # sst3.1_config_with_sstdevice
            #     This option used for configuring SST with supported deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g SST-3.1.0-with-sstdevice -m none -i none -o none -h none -s none -q 0.2.1 -M none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-qsim=$SST_DEPS_INSTALL_QSIM $miscEnv"
            ;;

        sst3.1_config_static) 
            #-----------------------------------------------------------------
            # sst3.1_config_static
            #     This option used for configuring SST with supported deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g SST-3.1.0 -m none -i none -o none -h none -s none -q 0.2.1 -M none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-qsim=$SST_DEPS_INSTALL_QSIM --enable-static --disable-shared $miscEnv"
            ;;

        sst3.1_config_macosx) 
            #-----------------------------------------------------------------
            # sst3.1_config_macosx
            #     This option used for configuring SST with supported deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g SST-3.1.0 -m none -i none -o none -h none -s none -q none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM $miscEnv"
            ;;

        sst3.1_config_macosx_static) 
            #-----------------------------------------------------------------
            # sst3.1_config_macosx_static
            #     This option used for configuring SST with supported deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g SST-3.1.0 -m none -i none -o none -h none -s none -q none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --enable-static --disable-shared $miscEnv"
            ;;
        sstmainline_config_memH_only) 
            #-----------------------------------------------------------------
            # sstmainline_config
            #     This option used for Quick test of opemMP and memHierarchy
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -g stabledevel -m none -i none -o none -h none -s none -q none -M none -N default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-hybridsim=$SST_DEPS_INSTALL_HYBRIDSIM $miscEnv"
            ;;
 
        # ====================================================================
        # ====                                                            ====
        # ====  Older 3.0.x build configurations start here  ====
        # ====                                                            ====
        # ====================================================================
        sst3.0_config) 
            #-----------------------------------------------------------------
            # sst3.0_config
            #     This option used for configuring SST with supported 3.0 deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g SST-3.0.0 -m none -i none -o none -h none -s 2.4.0 -q SST-3.0 -M none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-sstmacro=$SST_DEPS_INSTALL_SSTMACRO  --enable-phoenixsim --with-omnetpp=$SST_DEPS_INSTALL_OMNET --with-qsim=$SST_DEPS_INSTALL_QSIM $miscEnv"
            ;;
        sst3.0_config_macosx) 
            #-----------------------------------------------------------------
            # sst3.0_config_macosx
            #     This option used for configuring SST with supported 3.0 deps
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d 2.2.2 -p none -z none -b 1.50 -g SST-3.0.0 -m none -i none -o none -h none -s 2.4.0 -q none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-sstmacro=$SST_DEPS_INSTALL_SSTMACRO $miscEnv"
            ;;
        # ====================================================================
        # ====                                                            ====
        # ====  Experimental/exploratory build configurations start here  ====
        # ====                                                            ====
        # ====================================================================
        non_std_sst2.2_config) 
            #-----------------------------------------------------------------
            # non_std_sst2.2_config
            #     This option used for configuring SST with supported 2.2 deps
            #           Using not standard configuration
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment} CFLAGS=$python_inc_dir CXXFLAGS=$python_inc_dir"
            depsStr="-k none -d r4b00b22 -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s 2.3.0 -q none -M default"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST --with-gem5-build=opt --with-sstmacro=$SST_DEPS_INSTALL_SSTMACRO  --with-omnetpp=$SST_DEPS_INSTALL_OMNET"
            ;;
        gem5_no_dramsim_config) 
            #-----------------------------------------------------------------
            # gem5_no_dramsim_config
            #     This option used for configuring SST with gem5, but without
            #     dramsim enabled
            #-----------------------------------------------------------------
            export | egrep SST_DEPS_
            miscEnv="${mpi_environment}"
            depsStr="-k none -d none -p none -z none -b 1.50 -g stabledevel -m none -i none -o none -h none -s none -q none -M none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions --with-gem5=$SST_DEPS_INSTALL_GEM5SST $miscEnv"
            ;;
        sst_config_dist_test)
            #-----------------------------------------------------------------
            # sst_config_dist_test
            #      Do a "make dist"  (creating a tar file.)
            #      Then,  untar the created tar-file.
            #      Invoke bamboo.sh, (this file), to build sst from the tar.  
            #            Yes, bamboo invoked from bamboo.
            #      Finally, run tests to validate the created sst.
            #-----------------------------------------------------------------
            depsStr="-d none -g none"
            setConvenienceVars "$depsStr"
            configStr="$baseoptions  --with-glpk=${GLPK_HOME} --with-dramsim=$SST_DEPS_INSTALL_DRAMSIM --with-metis=${METIS_HOME}"
   
  ## perhaps do no more here
            ;;
        default|*)
            #-----------------------------------------------------------------
            # default
            #     If you've made a mistake in specifying your build config,
            #     do the default build. But this is probably not what you want!
            #-----------------------------------------------------------------
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
                buildtype=$OPTARG
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
    export LD_LIBRARY_PATH=${SST_INSTALL_DEPS}/lib:${SST_INSTALL_DEPS}/lib/sst:${SST_DEPS_INSTALL_GEM5SST}:${SST_INSTALL_DEPS}/packages/DRAMSim:${SST_DEPS_INSTALL_NVDIMMSIM}:${SST_DEPS_INSTALL_HYBRIDSIM}:${SST_INSTALL_DEPS}/packages/Qsim/lib:${SST_DEPS_INSTALL_CHDL}/lib:${LD_LIBRARY_PATH}
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BOOST_LIBS
    # Mac OS X needs some help finding dylibs
    if [ $kernel == "Darwin" ]
    then
	    export DYLD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${DYLD_LIBRARY_PATH}
    fi
    # Dump pre-build environment and modules status
    echo "--------------------PRE-BUILD ENVIRONMENT VARIABLE DUMP--------------------"
    env | sort
    echo "--------------------PRE-BUILD ENVIRONMENT VARIABLE DUMP--------------------"
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
        echo "bamboo.sh: Uh oh. Something went wrong during configure of sst.  Dumping config.log"
        echo "--------------------dump of config.log--------------------"
        sed -e 's/^/#dump /' ./config.log
        echo "--------------------dump of config.log--------------------"
        return $retval
    fi
    echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    echo ' '    
    echo       Configure complete without error
    echo ' '    
    echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"


    echo "at this time \$buildtype is $buildtype"

    if [ $buildtype == "sst_config_dist_test" ] ; then
        make dist
        retval=$?
        echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
        echo ' '    
        echo       make dist is done
        echo ' '    
        echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
        ls -ltr | tail -5
        return $retval        ##   This is in dobuild
    fi
    echo   " This is the non dist test path     +++++++++++++++++++++++++++++++++++++++++++++++"
    echo "bamboo.sh: making SST"
    # build SST
    make -j4 all
    retval=$?
    if [ $retval -ne 0 ]
    then
        return $retval
    fi

    # print build and linkage information for warm fuzzy
    echo "SSTBUILD INFO============================================================"
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
        echo "$ otool -L ./sst/core/sstsim.x"
        otool -L ./sst/core/sstsim.x
    else
        echo "$ ldd ./sst/core/sstsim.x"
        ldd ./sst/core/sstsim.x
    fi
    echo "SSTBUILD INFO============================================================"

    # install SST
    make -j4 install
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
# $4 = compiler type
#=========================================================================

echo "==============================INITIAL ENVIRONMENT DUMP=============================="
env|sort
echo "==============================INITIAL ENVIRONMENT DUMP=============================="

retval=0
echo  $0  $1 $2 $3 $4
echo `pwd`

if [ $# -lt 3 ] || [ $# -gt 4 ]
then
    # need build type and MPI type as argument

    echo "Usage : $0 <buildtype> <mpitype> <boost type> <[compiler type (optional)]>"
    exit 0

else
    # get desired compiler, if option provided
    compiler=""
    if [ "x$4" = x ]
    then
        echo "bamboo.sh: \$4 is empty or null, setting compiler to default"
        compiler="default"
    else
        echo "bamboo.sh: setting compiler to $4"
        compiler="$4"
    fi

    echo "bamboo.sh: compiler is set to $compiler"


    # Determine architecture
    arch=`uname -p`
    # Determine kernel name (Linux or MacOS i.e. Darwin)
    kernel=`uname -s`

    echo "bamboo.sh: KERNEL = $kernel"

    case $1 in
        default|sstmainline_config|sstmainline_config_linux_with_ariel|sstmainline_config_no_gem5|sstmainline_config_no_mpi|sstmainline_config_gcc_4_8_1|sstmainline_config_static|sstmainline_config_clang_core_only|sstmainline_config_macosx|sstmainline_config_macosx_no_gem5|sstmainline_config_macosx_static|sstmainline_config_static_macro_devel|sst3.0_config|sst3.0_config_macosx|sst3.1_config|sst3.1_config_with_sstdevice|sst3.1_config_static|sst3.1_config_macosx|sst3.1_config_macosx_static|non_std_sst2.2_config|gem5_no_dramsim_config|sstmainline_sstmacro_xconfig|sstmainline_config_xml2python|sstmainline_config_xml2python_static|sstmainline_config_memH_only|sst_config_dist_test|documentation|sstmainline_configA|sstmainline_config_VaultSim|sstmainline_configZ|sstmainline_config_stream|sstmainline_config_openmp|sstmainline_config_diropenmp|sstmainline_config_diropenmpB|sstmainline_config_dirnoncacheable|sstmainline_config_diropenmpI|sstmainline_config_all|sstmainline_config_gem5_gcc_4_6_4|sstmainline_config_fast|sstmainline_config_fast_static)
            #   Save Parameters $2, $3 and $4 in case they are need later
            SST_DIST_MPI=$2
            SST_DIST_BOOST=$3
            SST_DIST_PARAM4=$4

            # Configure MPI, Boost, and Compiler (Linux only)
            if [ $kernel != "Darwin" ]
            then

                # For some reason, .bashrc is not being run prior to
                # this script. Kludge initialization of modules.
                if [ -f /etc/profile.modules ]
                then
                    . /etc/profile.modules
                fi


                # build MPI and Boost selectors
                if [[ "$2" =~ openmpi.* ]]
                then
                    # since Boost flavor labeled with "ompi" not "openmpi"
                    mpiStr="ompi-"$(expr "$2" : '.*openmpi-\([0-9]\(\.[0-9][0-9]*\)*\)')
                else
                    mpiStr=${2}
                fi

                if [ $compiler = "default" ]
                then
                    desiredMPI="${2}"
                    desiredBoost="${3}.0_${mpiStr}"
                    module unload swig/swig-2.0.9
                else
                    desiredMPI="${2}_${4}"
                    desiredBoost="${3}.0_${mpiStr}_${4}"
                    # load non-default compiler
                    if   [[ "$4" =~ gcc.* ]]
                    then
                        module load gcc/${4}
                        module load swig/swig-2.0.9
                        echo "LOADED gcc/${4} compiler"
                    elif [[ "$4" =~ intel.* ]]
                    then
                        module load intel/${4}
                    fi
                fi
                echo "CHECK:  \$2: ${2}"
                echo "CHECK:  \$3: ${3}"
                echo "CHECK:  \$4: ${4}"
                echo "CHECK:  \$desiredMPI: ${desiredMPI}"
                echo "CHECK:  \$desiredBoost: ${desiredBoost}"
                gcc --version 2>&1 | grep ^g
                python --version

                # load MPI
                case $2 in
                    mpich2_stable|mpich2-1.4.1p1)
                        echo "MPICH2 stable (mpich2-1.4.1p1) selected"
                        module unload mpi # unload any default to avoid conflict error
                        module load mpi/${desiredMPI}
                        ;;
                    openmpi-1.7.2)
                        echo "OpenMPI 1.7.2 (openmpi-1.7.2) selected"
                        module unload mpi # unload any default to avoid conflict error
                        module load mpi/${desiredMPI}
                        ;;
                    ompi_1.6_stable|openmpi-1.6)
                        echo "OpenMPI stable (openmpi-1.6) selected"
                        module unload mpi # unload any default to avoid conflict error
                        module load mpi/${desiredMPI}
                        ;;
                    openmpi-1.4.4)
                        echo "OpenMPI (openmpi-1.4.4) selected"
                        module unload mpi # unload any default to avoid conflict error
                        module load mpi/${desiredMPI}
                        ;;
                    openmpi-1.8)
                        echo "OpenMPI (openmpi-1.8) selected"
                        module unload mpi # unload any default to avoid conflict error
                        module load mpi/${desiredMPI}
                        ;;
                    none)
                        echo "MPI requested as \"none\".    No MPI loaded"
                        module unload mpi # unload any default 
                        ;;
                    *)
                        echo "Default MPI option, loading mpi/${desiredMPI}"
                        module unload mpi # unload any default to avoid conflict error
                        module load mpi/${desiredMPI} 2>catch.err
                        if [ -s catch.err ] 
                        then
                            cat catch.err
                            exit 1
                        fi
                        ;;
                esac

                # load corresponding Boost
                case $3 in
                    boost-1.43)
                        echo "bamboo.sh: Boost 1.43 selected"
                        module unload boost
                        module load boost/${desiredBoost}
                        ;;
                    boost-1.48)
                        echo "bamboo.sh: Boost 1.48 selected"
                        module unload boost
                        module load boost/${desiredBoost}
                        ;;
                    boost-1.50)
                        echo "bamboo.sh: Boost 1.50 selected"
                        module unload boost
                        module load boost/${desiredBoost}
                        ;;
                    boost-1.54)
                        echo "bamboo.sh: Boost 1.54 selected"
                        module unload boost
                        module load boost/${desiredBoost}
                        ;;
                    boost-1.56)
                        echo "bamboo.sh: Boost 1.56 selected"
                        module unload boost
                        module load boost/${desiredBoost}
                        ;;
                    myBoost)
                        if [ $2 == "openmpi-1.8" ] ; then
                            export BOOST_HOME=/home/jpvandy/local/packages/boost/boost-1.54.0_ompi-1.8_mine
                            export BOOST_LIBS=/home/jpvandy/local/packages/boost/boost-1.54.0_ompi-1.8_mine/lib
                            export BOOST_INCLUDE=/home/jpvandy/local/packages/boost/boost-1.54.0_ompi-1.8_mine/include
                        else
                            export BOOST_LIBS=/home/jpvandy/User-Build-Oct14/local/module-pkgs/boost/boost-1.54.0/lib
                            export BOOST_HOME=/home/jpvandy/User-Build-Oct14/local/module-pkgs/boost/boost-1.54.0
                            export BOOST_INCLUDE=/home/jpvandy/User-Build-Oct14/local/module-pkgs/boost/boost-1.54.0/include
                        fi
                        ;; 
                    noMpiBoost)
                        export BOOST_LIBS=/home/jpvandy/local/packages/boost-1.54_no-mpi/lib
                        export BOOST_HOME=/home/jpvandy/local/packages/boost-1.54_no-mpi
                        export BOOST_INCLUDE=/home/jpvandy/local/packages/boost-1.54_no-mpi/include
                        export LD_LIBRARY_PATH=$BOOST_LIBS:$LD_LIBRARY_PATH
                        ;;
                    *)
                        echo "bamboo.sh: \"Default\" Boost selected"
                        echo "Third argument was $3"
                        echo "Loading boost/${desiredBoost}"
                        module unload boost
                        module load boost/${desiredBoost} 2>catch.err
                        if [ -s catch.err ] 
                        then
                            cat catch.err
                            exit 1
                        fi
                        ;;
                esac
                echo "bamboo.sh: BOOST_HOME=${BOOST_HOME}"
                export SST_DEPS_INSTALL_BOOST=${BOOST_HOME}
                echo "bamboo.sh: SST_DEPS_INSTALL_BOOST=${SST_DEPS_INSTALL_BOOST}"

                # Load other modules that were built with the default compiler
                if [ $compiler = "default" ]
                then
                    # GNU Linear Programming Kit (GLPK)
                    echo "bamboo.sh: Load GLPK"
                    module load glpk/glpk-4.54
                    # System C
                    echo "bamboo.sh: Load System C"
                    module load systemc/systemc-2.3.0
                    # METIS 5.1.0
                    echo "bamboo.sh: Load METIS 5.1.0"
                    module load metis/metis-5.1.0
                    # Other misc
                    echo "bamboo.sh: Load libphx"
                    module load libphx/libphx-2014-MAY-08
                fi

#                # load OMNet++
#                module unload omnet++
#                module load omnet++/omnet++-4.1_no-mpi 2>__std.err__
#
#                cat __std.err__
#                if [[ "`cat __std.err__`" == *ERROR* ]]
#                then
#                     echo Load of omnet module failed
#                     exit
#                fi
#                echo "bamboo.sh: OMNET_HOME=${OMNET_HOME}"
#                export SST_DEPS_INSTALL_OMNET=${OMNET_HOME}
#                echo "bamboo.sh: SST_DEPS_INSTALL_OMNET=${SST_DEPS_INSTALL_OMNET}"

            else  # kernel is "Darwin", so this is MacOS
                # Obtain Mac OS version (works only on MacOS!!!)
                macosVersionFull=`sw_vers -productVersion`
                macosVersion=${macosVersionFull%.*}

                if [[ $macosVersion = "10.8" && $compiler = "clang-503.0.40" ]]
                then
                    # Mountain Lion + clang PATH                    
                    PATH="$HOME/Documents/GNUTools/Automake/1.14.1/bin:$HOME/Documents/GNUTools/Autoconf/2.69.0/bin:$HOME/Documents/GNUTools/Libtool/2.4.2/bin:$HOME/Documents/wget-1.15/bin:$PATH"
                    export PATH

                    # Mountain Lion requires this extra flag passed to SST ./configure
                    # Mavericks does not require this.
                    MTNLION_FLAG="--disable-cxx11"
                    export MTNLION_FLAG
                else
                    # macports or hybrid clang/macports
                    PATH="/opt/local/bin:/usr/local/bin:$PATH"
                    export PATH
                fi


                # Point to aclocal per instructions from sourceforge on MacOSX installation
                export ACLOCAL_FLAGS="-I/opt/local/share/aclocal $ACLOCAL_FLAGS"
                echo $ACLOCAL_FLAGS

                # Initialize modules for Jenkins (taken from $HOME/.bashrc on Mac)
                if [ -f /etc/profile.modules ]
                then
                    . /etc/profile.modules
                    echo "bamboo.sh: loaded /etc/profile.modules. Available modules"
                    module avail
                    # put any module loads here
                    echo "bamboo.sh: Loading Modules for MacOSX"
                    # Do things specific to the MacOS version
                    case $macosVersion in
                        10.6) # Snow Leopard
                            # use modules Boost, built-in MPI, default compiler
                            module unload boost
                            module add boost/boost-1.50.0
                            module list
                            ;;
                        10.7) # Lion
                            # use modules Boost and MPI, default compiler (gcc)
                            module unload mpi
                            module unload boost

#                           // Lion used to be hardcoded to this configuration, now changed.                            
#                           module add mpi/openmpi-1.4.4_gcc-4.2.1
#                           module add boost/boost-1.50.0_ompi-1.4.4_gcc-4.2.1

                            #Check for Illegal configurations of Boost and MPI
                            if [[ ( $2 = "openmpi-1.7.2" &&  $3 = "boost_default" ) || \
                                  ( $2 = "openmpi-1.7.2" &&  $3 = "boost-1.50" )    || \
                                  ( $2 = "openmpi-1.4.4" &&  $3 = "boost-1.54" )    || \
                                  ( $2 = "ompi_default"  &&  $3 = "boost-1.54" ) ]]
                            then
                                echo "ERROR: Invalid configuration of $2 and $3 These two modules cannot be combined"
                                exit 0
                            fi
                           
                            # load MPI
                            case $2 in
                                openmpi-1.7.2)
                                    echo "OpenMPI 1.7.2 (openmpi-1.7.2) selected"
                                    module add mpi/openmpi-1.7.2_gcc-4.2.1
                                    ;;
                                ompi_default|openmpi-1.4.4)
                                    echo "OpenMPI 1.4.4 (Default) (openmpi-1.4.4) selected"
                                    module add mpi/openmpi-1.4.4_gcc-4.2.1
                                    ;;
                                *)
                                    echo "Default MPI option, loading mpi/openmpi-1.4.4"
                                    module load mpi/openmpi-1.4.4_gcc-4.2.1 2>catch.err
                                    if [ -s catch.err ] 
                                    then
                                        cat catch.err
                                        exit 0
                                    fi
                                    ;;
                            esac
                                                
                            # load corresponding Boost
                            case $3 in
                                boost-1.54)
                                    echo "Boost 1.54 selected"
                                    module add boost/boost-1.54.0_ompi-1.7.2_gcc-4.2.1
                                    ;;
                                boost_default|boost-1.50)
                                    echo "Boost 1.50 (Default) selected"
                                    module add boost/boost-1.50.0_ompi-1.4.4_gcc-4.2.1
                                    ;;
                                *)
                                    echo "bamboo.sh: \"Default\" Boost selected"
                                    echo "Third argument was $3"
                                    echo "Loading boost/Boost 1.50"
                                    module load boost/boost-1.50.0_ompi-1.4.4_gcc-4.2.1 2>catch.err
                                    if [ -s catch.err ] 
                                    then
                                        cat catch.err
                                        exit 0
                                    fi
                                    ;;
                            esac
                            export CC=`which gcc`
                            export CXX=`which g++`
                            module list
                            ;;
                        10.8) # Mountain Lion
                            # Depending on specified compiler, load Boost and MPI
                            case $compiler in
                                gcc-4.2.1)
                                    # Use Selected Boost and MPI built with GCC
                                    module unload mpi
                                    module unload boost

                                   #Check for Illegal configurations of Boost and MPI
                                    if [[ ( $2 = "openmpi-1.7.2" &&  $3 = "boost_default" ) || \
                                          ( $2 = "openmpi-1.7.2" &&  $3 = "boost-1.50" )    || \
                                          ( $2 = "openmpi-1.6.3" &&  $3 = "boost-1.54" )    || \
                                          ( $2 = "ompi_default"  &&  $3 = "boost-1.54" ) ]]
                                    then
                                        echo "ERROR: Invalid configuration of $2 and $3 These two modules cannot be combined"
                                        exit 0
                                    fi
                                   
                                    # load MPI
                                    case $2 in
                                        openmpi-1.7.2)
                                            echo "OpenMPI 1.7.2 (openmpi-1.7.2) selected"
                                            module add mpi/openmpi-1.7.2_gcc-4.2.1
                                            ;;
                                        ompi_default|openmpi-1.6.3)
                                            echo "OpenMPI 1.6.3 (Default) (openmpi-1.6.3) selected"
                                            module add mpi/openmpi-1.6.3_gcc-4.2.1
                                            ;;
                                        *)
                                            echo "Default MPI option, loading mpi/openmpi-1.6.3"
                                            module load mpi/openmpi-1.6.3_gcc-4.2.1 2>catch.err
                                            if [ -s catch.err ] 
                                            then
                                                cat catch.err
                                                exit 0
                                            fi
                                            ;;
                                    esac
                                                        
                                    # load corresponding Boost
                                    case $3 in
                                        boost-1.54)
                                            echo "Boost 1.54 selected"
                                            module add boost/boost-1.54.0_ompi-1.7.2_gcc-4.2.1
                                            ;;
                                        boost_default|boost-1.50)
                                            echo "Boost 1.50 (Default) selected"
                                            module add boost/boost-1.50.0_ompi-1.6.3_gcc-4.2.1
                                            ;;
                                        *)
                                            echo "bamboo.sh: \"Default\" Boost selected"
                                            echo "Third argument was $3"
                                            echo "Loading boost/Boost 1.50"
                                            module load boost/boost-1.50.0_ompi-1.6.3_gcc-4.2.1 2>catch.err
                                            if [ -s catch.err ] 
                                            then
                                                cat catch.err
                                                exit 0
                                            fi
                                            ;;
                                    esac
                                    export CC=`which gcc`
                                    export CXX=`which g++`
                                    module list
                                    ;;
                                    
                                    
                                clang-425.0.27)
                                    # Use Boost and MPI built with CLANG
                                    module unload mpi
                                    module unload boost

                                   #Check for Illegal configurations of Boost and MPI
                                    if [[ ( $2 = "openmpi-1.7.2" &&  $3 = "boost_default" ) || \
                                          ( $2 = "openmpi-1.7.2" &&  $3 = "boost-1.50" )    || \
                                          ( $2 = "openmpi-1.6.3" &&  $3 = "boost-1.54" )    || \
                                          ( $2 = "ompi_default"  &&  $3 = "boost-1.54" ) ]]
                                    then
                                        echo "ERROR: Invalid configuration of $2 and $3 These two modules cannot be combined"
                                        exit 0
                                    fi

                                    # load MPI
                                    case $2 in
                                        openmpi-1.7.2)
                                            echo "OpenMPI 1.7.2 (openmpi-1.7.2) selected"
                                            module add mpi/openmpi-1.7.2_clang-425.0.27
                                            ;;
                                        ompi_default|openmpi-1.6.3)
                                            echo "OpenMPI 1.6.3 (Default) (openmpi-1.6.3) selected"
                                            module add mpi/openmpi-1.6.3_clang-425.0.27
                                            ;;
                                        *)
                                            echo "Default MPI option, loading mpi/openmpi-1.6.3"
                                            module load mpi/openmpi-1.6.3_clang-425.0.27 2>catch.err
                                            if [ -s catch.err ] 
                                            then
                                                cat catch.err
                                                exit 0
                                            fi
                                            ;;
                                    esac
                                                        
                                    # load corresponding Boost
                                    case $3 in
                                        boost-1.54)
                                            echo "Boost 1.54 selected"
                                            module add boost/boost-1.54.0_ompi-1.7.2_clang-425.0.27
                                            ;;
                                        boost_default|boost-1.50)
                                            echo "Boost 1.50 (Default) selected"
                                            module add boost/boost-1.50.0_ompi-1.6.3_clang-425.0.27
                                            ;;
                                        *)
                                            echo "bamboo.sh: \"Default\" Boost selected"
                                            echo "Third argument was $3"
                                            echo "Loading boost/Boost 1.50"
                                            module load boost/boost-1.50.0_ompi-1.6.3_clang-425.0.27 2>catch.err
                                            if [ -s catch.err ] 
                                            then
                                                cat catch.err
                                                exit 0
                                            fi
                                            ;;
                                    esac
                                    export CC=`which clang`
                                    export CXX=`which clang++`
                                    module list
                                    ;;
###
                                gcc-4.6.4)
                                    # Use Selected Boost and MPI built with MacPorts GCC 4.6.4
                                    module unload mpi
                                    module unload boost

                                    # Load gcc-4.6.4 specific modules
                                    # GNU Linear Programming Kit (GLPK)
                                    echo "bamboo.sh: Load GLPK"
                                    module load glpk/glpk-4.54_gcc-4.6.4
                                    # System C
                                    echo "bamboo.sh: Load System C"
                                    module load systemc/systemc-2.3.0_gcc-4.6.4
                                    # METIS 5.1.0
                                    echo "bamboo.sh: Load METIS 5.1.0"
                                    module load metis/metis-5.1.0_gcc-4.6.4
                                    # Other misc
                                    echo "bamboo.sh: Load libphx"
                                    module load libphx/libphx-2014-MAY-08_gcc-4.6.4

                                    # load MPI
                                    case $2 in
                                        ompi_default|openmpi-1.8)
                                            echo "OpenMPI 1.8 (openmpi-1.8) selected"
                                            module add mpi/openmpi-1.8_gcc-4.6.4
                                            ;;
                                        *)
                                            echo "Default MPI option, loading mpi/openmpi-1.8"
                                            module load mpi/openmpi-1.8_gcc-4.6.4 2>catch.err
                                            if [ -s catch.err ] 
                                            then
                                                cat catch.err
                                                exit 0
                                            fi
                                            ;;
                                    esac

                                    # load corresponding Boost
                                    case $3 in
                                        boost_default|boost-1.54)
                                            echo "Boost 1.54 selected"
                                            module add boost/boost-1.54.0_ompi-1.8_gcc-4.6.4
                                            ;;
                                        boost_default|boost-1.56)
                                            echo "Boost 1.56 selected"
                                            module add boost/boost-1.56.0_ompi-1.8_gcc-4.6.4
                                            ;;
                                        *)
                                            echo "bamboo.sh: \"Default\" Boost selected"
                                            echo "Third argument was $3"
                                            echo "Loading boost/Boost 1.56"
                                            module add boost/boost-1.56.0_ompi-1.8_gcc-4.6.4 2>catch.err
                                            if [ -s catch.err ] 
                                            then
                                                cat catch.err
                                                exit 0
                                            fi
                                            ;;
                                    esac
                                    export CC=`which gcc`
                                    export CXX=`which g++`
                                    module list
                                    ;;
                                    
                                clang-503.0.38)
                                    # Use Boost and MPI built with CLANG from Xcode 5.1
                                    module unload mpi
                                    module unload boost

                                    # Load other modules for clang-503.0.38
                                    # GNU Linear Programming Kit (GLPK)
                                    echo "bamboo.sh: Load GLPK"
                                    module load glpk/glpk-4.54_clang-503.0.38
                                    # System C
                                    echo "bamboo.sh: Load System C"
                                    module load systemc/systemc-2.3.0_clang-503.0.38
                                    # METIS 5.1.0
                                    echo "bamboo.sh: Load METIS 5.1.0"
                                    module load metis/metis-5.1.0_clang-503.0.38
                                    # Other misc
                                    echo "bamboo.sh: Load libphx"
                                    module load libphx/libphx-2014-MAY-08_clang-503.0.38

                                    # load MPI
                                    case $2 in
                                        ompi_default|openmpi-1.8)
                                            echo "OpenMPI 1.8 (openmpi-1.8) selected"
                                            module add mpi/openmpi-1.8_clang-503.0.38
                                            ;;
                                        *)
                                            echo "Default MPI option, loading mpi/openmpi-1.8"
                                            module load mpi/openmpi-1.8_clang-503.0.38 2>catch.err
                                            if [ -s catch.err ] 
                                            then
                                                cat catch.err
                                                exit 0
                                            fi
                                            ;;
                                    esac
                                                        
                                    # load corresponding Boost
                                    case $3 in
                                        boost_default|boost-1.54)
                                            echo "Boost 1.54 selected"
                                            module add boost/boost-1.54.0_ompi-1.8_clang-503.0.38
                                            ;;
                                        boost_default|boost-1.56)
                                            echo "Boost 1.56 selected"
                                            module add boost/boost-1.56.0_ompi-1.8_clang-503.0.38
                                            ;;
                                        *)
                                            echo "bamboo.sh: \"Default\" Boost selected"
                                            echo "Third argument was $3"
                                            echo "Loading boost/Boost 1.56"
                                            module load boost/boost-1.56.0_ompi-1.8_clang-503.0.38 2>catch.err
                                            if [ -s catch.err ] 
                                            then
                                                cat catch.err
                                                exit 0
                                            fi
                                            ;;
                                    esac
                                    export CC=`which clang`
                                    export CXX=`which clang++`
                                    module list
                                    ;;

                                clang-503.0.40)
                                    # Use Boost and MPI built with CLANG from Xcode 5.1
                                    module unload mpi
                                    module unload boost

                                    # load MPI
                                    case $2 in
                                        ompi_default|openmpi-1.8)
                                            echo "OpenMPI 1.8 (openmpi-1.8) selected"
                                            module add mpi/openmpi-1.8_clang-503.0.40
                                            ;;
                                        *)
                                            echo "Default MPI option, loading mpi/openmpi-1.8"
                                            module load mpi/openmpi-1.8_clang-503.0.40 2>catch.err
                                            if [ -s catch.err ] 
                                            then
                                                cat catch.err
                                                exit 0
                                            fi
                                            ;;
                                    esac
                                                        
                                    # load corresponding Boost
                                    case $3 in
                                        boost_default|boost-1.54)
                                            echo "Boost 1.54 selected"
                                            module add boost/boost-1.54.0_ompi-1.8_clang-503.0.40
                                            ;;
                                        boost_default|boost-1.56)
                                            echo "Boost 1.56 selected"
                                            module add boost/boost-1.56.0_ompi-1.8_clang-503.0.40
                                            ;;
                                        *)
                                            echo "bamboo.sh: \"Default\" Boost selected"
                                            echo "Third argument was $3"
                                            echo "Loading boost/Boost 1.56"
                                            module load boost/boost-1.56.0_ompi-1.8_clang-503.0.40 2>catch.err
                                            if [ -s catch.err ] 
                                            then
                                                cat catch.err
                                                exit 0
                                            fi
                                            ;;
                                    esac
                                    export CC=`which clang`
                                    export CXX=`which clang++`
                                    module list
                                    ;;

###
                                *)
                                    # unknown compiler, use default
                                    echo "bamboo.sh: Unknown compiler selection. Assuming gcc."
                                    module unload boost
                                    module unload mpi
                                    module add boost/boost-1.50.0_ompi-1.6.3_gcc-4.2.1
                                    module add mpi/openmpi-1.6.3_gcc-4.2.1
                                    module list
                                    ;;  
                            esac
                            ;;

################################################################################
                        10.9) # Mavericks
                            # Depending on specified compiler, load Boost and MPI
                            case $compiler in
                                gcc-4.6.4)
                                    # Use Selected Boost and MPI built with MacPorts GCC 4.6.4
                                    module unload mpi
                                    module unload boost

                                    # Load gcc-4.6.4 specific modules
                                    # GNU Linear Programming Kit (GLPK)
                                    echo "bamboo.sh: Load GLPK"
                                    module load glpk/glpk-4.54_gcc-4.6.4
                                    # System C
                                    echo "bamboo.sh: Load System C"
                                    module load systemc/systemc-2.3.0_gcc-4.6.4
                                    # METIS 5.1.0
                                    echo "bamboo.sh: Load METIS 5.1.0"
                                    module load metis/metis-5.1.0_gcc-4.6.4
                                    # Other misc
                                    echo "bamboo.sh: Load libphx"
                                    module load libphx/libphx-2014-MAY-08_gcc-4.6.4

                                    # load MPI
                                    case $2 in
                                        ompi_default|openmpi-1.8)
                                            echo "OpenMPI 1.8 (openmpi-1.8) selected"
                                            module add mpi/openmpi-1.8_gcc-4.6.4
                                            ;;
                                        *)
                                            echo "Default MPI option, loading mpi/openmpi-1.8"
                                            module load mpi/openmpi-1.8_gcc-4.6.4 2>catch.err
                                            if [ -s catch.err ] 
                                            then
                                                cat catch.err
                                                exit 0
                                            fi
                                            ;;
                                    esac

                                    # load corresponding Boost
                                    case $3 in
                                        boost_default|boost-1.54)
                                            echo "Boost 1.54 selected"
                                            module add boost/boost-1.54.0_ompi-1.8_gcc-4.6.4
                                            ;;
                                        boost_default|boost-1.56)
                                            echo "Boost 1.56 selected"
                                            module add boost/boost-1.56.0_ompi-1.8_gcc-4.6.4
                                            ;;
                                        *)
                                            echo "bamboo.sh: \"Default\" Boost selected"
                                            echo "Third argument was $3"
                                            echo "Loading boost/Boost 1.56"
                                            module add boost/boost-1.56.0_ompi-1.8_gcc-4.6.4 2>catch.err
                                            if [ -s catch.err ] 
                                            then
                                                cat catch.err
                                                exit 0
                                            fi
                                            ;;
                                    esac
                                    export CC=`which gcc`
                                    export CXX=`which g++`
                                    module list
                                    ;;
                                    
                                clang-503.0.38)
                                    # Use Boost and MPI built with CLANG from Xcode 5.1
                                    module unload mpi
                                    module unload boost

                                    # Load other modules for clang-503.0.38
                                    # GNU Linear Programming Kit (GLPK)
                                    echo "bamboo.sh: Load GLPK"
                                    module load glpk/glpk-4.54_clang-503.0.38
                                    # System C
                                    echo "bamboo.sh: Load System C"
                                    module load systemc/systemc-2.3.0_clang-503.0.38
                                    # METIS 5.1.0
                                    echo "bamboo.sh: Load METIS 5.1.0"
                                    module load metis/metis-5.1.0_clang-503.0.38
                                    # Other misc
                                    echo "bamboo.sh: Load libphx"
                                    module load libphx/libphx-2014-MAY-08_clang-503.0.38

                                    # load MPI
                                    case $2 in
                                        ompi_default|openmpi-1.8)
                                            echo "OpenMPI 1.8 (openmpi-1.8) selected"
                                            module add mpi/openmpi-1.8_clang-503.0.38
                                            ;;
                                        *)
                                            echo "Default MPI option, loading mpi/openmpi-1.8"
                                            module load mpi/openmpi-1.8_clang-503.0.38 2>catch.err
                                            if [ -s catch.err ] 
                                            then
                                                cat catch.err
                                                exit 0
                                            fi
                                            ;;
                                    esac
                                                        
                                    # load corresponding Boost
                                    case $3 in
                                        boost_default|boost-1.54)
                                            echo "Boost 1.54 selected"
                                            module add boost/boost-1.54.0_ompi-1.8_clang-503.0.38
                                            ;;
                                        boost_default|boost-1.56)
                                            echo "Boost 1.56 selected"
                                            module add boost/boost-1.56.0_ompi-1.8_clang-503.0.38
                                            ;;
                                        *)
                                            echo "bamboo.sh: \"Default\" Boost selected"
                                            echo "Third argument was $3"
                                            echo "Loading boost/Boost 1.56"
                                            module load boost/boost-1.56.0_ompi-1.8_clang-503.0.38 2>catch.err
                                            if [ -s catch.err ] 
                                            then
                                                cat catch.err
                                                exit 0
                                            fi
                                            ;;
                                    esac
                                    export CC=`which clang`
                                    export CXX=`which clang++`
                                    module list
                                    ;;

                                *)
                                    # unknown compiler, use default
                                    echo "bamboo.sh: Unknown compiler selection. Assuming gcc."
                                    module unload boost
                                    module unload mpi
                                    module add mpi/openmpi-1.8_gcc-4.6.4
                                    module add boost/boost-1.56.0_ompi-1.8_gcc-4.6.4
                                    module list
                                    ;;  
################################################################################
                            esac
                            ;;
                        *) # unknown
                            echo "bamboo.sh: Unknown Mac OS version."
                            ;;
                    esac

                    echo "bamboo.sh: BOOST_HOME=${BOOST_HOME}"
                    export SST_DEPS_INSTALL_BOOST=${BOOST_HOME}

                fi

                echo "bamboo.sh: MacOS build."
                echo "bamboo.sh:   MPI = $2, Boost = $3"
            fi

            # if Intel PIN module is available, load it
            module avail 2>&1 >/dev/null | egrep -q "pin-2.13-61206-gcc.4.4.7-linux"
            if [ $? == 0 ] 
            then
                echo "Loading Intel PIN environment module"
                module load pin/pin-2.13-61206-gcc.4.4.7-linux
            else
                echo "Intel PIN environment module not found on this host."
            fi

            echo "bamboo.sh: LISTING LOADED MODULES"
            module list

            # Build type given as argument to this script
            export SST_BUILD_TYPE=$1

            if [ $SST_BUILD_TYPE = "documentation" ]
            then
                # build documentation, create list of undocumented files
                ./autogen.sh
                ./configure --disable-silent-rules --prefix=$HOME/local --with-boost=$BOOST_HOME
                make html 2> $SST_ROOT/doc/makeHtmlErrors.txt
                egrep "is not documented" $SST_ROOT/doc/makeHtmlErrors.txt | sort > $SST_ROOT/doc/undoc.txt
                retval=0
            else
                dobuild -t $SST_BUILD_TYPE -a $arch -k $kernel
                retval=$?
            fi

            ;;

        *)
            echo "$0 : unknown action \"$1\""
            retval=1
            ;;
    esac
fi
   
if [ $retval -eq 0 ]
then
    if [ $SST_BUILD_TYPE = "documentation" ]
    then
        # dump list of undocumented files
        echo "============================== DOXYGEN UNDOCUMENTED FILES =============================="
        sed -e 's/^/#doxygen /' $SST_ROOT/doc/undoc.txt
        echo "============================== DOXYGEN UNDOCUMENTED FILES =============================="
        retval=0
    else
        # Build was successful, so run tests, providing command line args
        # as a convenience. SST binaries must be generated before testing.

        if [ $buildtype == "sst_config_dist_test" ] ; then  
             echo "Setting up to build from the tar created by make dist"
             echo "---   PWD  `pwd`"           ## Original trunk
             Package=`ls| grep 'sst-.*tar.gz' | awk -F'.tar' '{print $1}'`
             echo  PACKAGE is $Package
             tarName=${Package}.tar.gz
             ls $tarFile
             if [ $? != 0 ] ; then
                 ls
                 echo Can NOT find Tar File $Package .tar.gz
                 exit 1
             fi
             mkdir $SST_ROOT/distTestDir
             cd $SST_ROOT/distTestDir
             mv $SST_ROOT/$tarName .
             if [ $? -ne 0 ] ; then
                  echo "Move failed  \$SST_ROOT/$tarName to ."
                  exit 1
             fi
             echo "   Untar the created file, $tarName"
             tar xzf $tarName
             if [ $? -ne 0 ] ; then
                  echo "Untar of $tarName failed"
                  exit 1
             fi
             mv $Package trunk
             echo "Move in items not in the trunk, that are need for the bamboo build and test"
             cp  $SST_ROOT/bamboo.sh trunk
             cp -r $SST_ROOT/deps trunk          ## the deps scripts
             cd trunk
             echo "                   List the directories in sst/elements"
             ls sst/elements
             echo ' '
             ln -s ../../test              ## the subtree of tests
             ls -l
             echo SST_INSTALL_DEPS =  $SST_INSTALL_DEPS
                ## pristine is not at the same relative depth on Jenkins as it is for me.
             echo "  Find pristine"
             if [ $SST_BASE == "/home/jwilso" ] ; then
                 PRISTINE="/home/jwilso/sstDeps/src/pristine"
             else 
                 find $SST_BASE -name pristine
                 PRISTINE=`find $SST_BASE -name pristine`
             fi
             echo "\$PRISTINE = $PRISTINE"
             ls $PRISTINE/*
             if [[ $? != 0 ]] ; then
                 echo " Failed to find pristine "
                 exit 1
             fi
             export SST_BASE=$SST_ROOT
             export SST_DEPS_USER_DIR=$SST_ROOT
             export SST_DEPS_USER_MODE=1
             export SST_INSTALL_DEPS=$SST_BASE/local
             mkdir -p ../../sstDeps/src
             pushd ../../sstDeps/src
             ln -s $PRISTINE .
             ls -l pristine
             popd
             echo SST_DEPS_USER_DIR= $SST_DEPS_USER_DIR
                      ##  Here is the bamboo invocation within bamboo
             echo "         INVOKE bamboo for the build from the dist tar"
             ./bamboo.sh sstmainline_config_all $SST_DIST_MPI $SST_DIST_BOOST $SST_DIST_PARAM4
             retval=$?
             echo "         Returned from bamboo.sh $retval"
             if [ $retval != 0 ] ; then
                 echo "bamboo build reports failure  retval = $reval"
                 exit 1
             fi
             exit 0                  #  Normal Exit for make dist
        else          #  not make dist
            #    ---  These are probably temporary, but let's line them up properly anyway
            pwd
            echo "            CHECK ENVIRONMENT VARIABLES "
            env | grep SST
            echo "            End of SST Environs"
            pwd
            ls
            #    ---
            if [ -d "test" ] ; then
                echo " \"test\" is a directory"
                echo " ############################  ENTER dotests ################## "
                dotests $1
            fi
        fi               #   End of sst_config_dist_test  conditional
    fi
fi

if [ $retval -eq 0 ]
then
    echo "$0 : exit success."
else
    echo "$0 : exit failure."
fi

exit $retval
