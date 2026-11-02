#!/usr/bin/env bash

# set-env.sh: Write the environment of the build and test steps to
# GITHUB_ENV from the inputs of sst-ci-job.yml.
#
# Usage: set-env.sh
#
# Reads SST_MPI (true or false), SST_TOOLCHAIN, SST_TEST_RANKS,
# SST_TEST_THREADS, SST_TEST_CATEGORIES and SST_TEST_OVERSUBSCRIBE (true or
# false) from the environment, which the workflow step sets from its inputs.

set -euo pipefail

# The script names use "-nompi", and the install directory "_nompi".
if [[ "${SST_MPI}" == "true" ]]; then
    mpi_suffix=""
else
    mpi_suffix="nompi"
fi
MAKEFLAGS="-j$(nproc)"
{
    echo "MAKEFLAGS=${MAKEFLAGS}"
    echo "TERM=dumb"
    # https://github.com/open-mpi/ompi/issues/11295
    echo "PSM3_DEVICES=self"
    # Runners have 2 cores with 2 hardware threads each; let
    # "--map-by numa:PE=N" count hardware threads (OpenMPI 4).
    echo "OMPI_MCA_hwloc_base_use_hwthreads_as_cpus=1"
    echo "SST_MPI_SUFFIX=${mpi_suffix:+-${mpi_suffix}}"
    echo "SST_TOOLCHAIN=${SST_TOOLCHAIN}"
    echo "SST_INSTALL_DIR=${GITHUB_WORKSPACE}/install_autotools_noflags_nodeps${mpi_suffix:+_${mpi_suffix}}_${SST_TOOLCHAIN}"
    echo "SST_TEST_RANKS=${SST_TEST_RANKS}"
    echo "SST_TEST_THREADS=${SST_TEST_THREADS}"
    echo "SST_TEST_CATEGORIES=${SST_TEST_CATEGORIES}"
    if [[ "${SST_TEST_OVERSUBSCRIBE}" == "true" ]]; then
        # Ranks x threads is more than the hardware threads of the runner.
        # Let OpenMPI place the extra PEs instead of refusing to start.
        echo "OMPI_MCA_rmaps_base_oversubscribe=1"
        echo "OMPI_MCA_hwloc_base_binding_policy=core:overload-allowed"
    fi
} >> "${GITHUB_ENV}"
