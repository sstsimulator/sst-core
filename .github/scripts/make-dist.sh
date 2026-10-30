#!/usr/bin/env bash

# make-dist.sh: Replace the sst-core and sst-elements git checkouts with the
# source trees from their "make dist" tarballs which themselves are built and
# tested.
#
# Usage: make-dist.sh TOOLCHAIN
#
# Run it from the directory that contains sst-core/ and sst-elements/,
# before the normal compile steps.  It follows sst-sqe buildsys/bamboo.sh
# (sstmainline_config_make_dist_test):
#
#   1. core: autogen, configure, make dist, then build and install, because
#      elements needs an installed core to configure;
#   2. elements: autogen, configure, make dist;
#   3. delete the git checkouts and unpack the tarballs in their place.
#
# The tarballs are kept in dist/.  The unpacked trees have no .git, so the
# compile scripts run configure only, without autogen.sh.  The tests then
# read their reference files from the unpacked elements tarball.
#
# Run this script from a copy outside sst-core/, since it deletes
# sst-core/.

set -euo pipefail

if [[ "$#" -ne 1 ]]; then
    echo "usage: ${0} TOOLCHAIN" >&2
    exit 2
fi
toolchain="${1}"

scriptdir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# compilers.sh is not clean under "set -u".
set +u
# shellcheck source=SCRIPTDIR/compilers.sh
source "${scriptdir}/compilers.sh"
source_compilers_mpi "${toolchain}"
set -u

# The same directories as build-core-autotools-noflags-nodeps.sh, so that the
# second build of core gets ccache hits from the first one.
suffix="$(clean_suffix "autotools_noflags_nodeps_${toolchain}")"
dir_core_src="${PWD}/sst-core"
dir_core_build="${PWD}/sst-core-build-${suffix}"
dir_elements_src="${PWD}/sst-elements"
dir_elements_build="${PWD}/sst-elements-build-${suffix}"
dir_install="${PWD}/install_${suffix}"
dir_dist="${PWD}/dist"

rm -rf "${dir_core_build}" "${dir_elements_build}" "${dir_install}" "${dir_dist}"
mkdir -p "${dir_dist}"

# Print the only tarball in directory $1 whose name matches one of the
# patterns $2....
find_tarball() {
    local dir="${1}"
    shift
    local names=()
    local pattern
    for pattern in "$@"; do
        if [[ "${#names[@]}" -gt 0 ]]; then
            names+=(-o)
        fi
        names+=(-name "${pattern}")
    done
    local tarballs=()
    mapfile -t tarballs < <(find "${dir}" -maxdepth 1 \( "${names[@]}" \))
    if [[ "${#tarballs[@]}" -ne 1 ]]; then
        echo "expected one of $* in ${dir}, found ${#tarballs[@]}" >&2
        exit 1
    fi
    echo "${tarballs[0]}"
}

# Unpack tarball $1 and rename its top-level directory to $2.
unpack_as() {
    local tarball="${1}"
    local dest="${2}"
    local top
    top="$(tar -tzf "${tarball}" | sed -n 1p)"
    top="${top%%/*}"
    tar -xzf "${tarball}"
    mv "${top}" "${dest}"
}

echo "::group::make dist: core"
pushd "${dir_core_src}"
./autogen.sh
popd
mkdir -p "${dir_core_build}"
pushd "${dir_core_build}"
"${dir_core_src}/configure" --prefix="${dir_install}"
make dist
# AC_INIT([SSTCore], ...) names the tarball sstcore-VERSION.
core_tarball="$(find_tarball "${dir_core_build}" 'sstcore-*.tar.gz' 'sst-core-*.tar.gz')"
mv "${core_tarball}" "${dir_dist}/"
core_tarball="${dir_dist}/$(basename "${core_tarball}")"
make -j"$(max_cpus)"
make install
popd
echo "::endgroup::"

echo "::group::make dist: elements"
pushd "${dir_elements_src}"
./autogen.sh
popd
mkdir -p "${dir_elements_build}"
pushd "${dir_elements_build}"
"${dir_elements_src}/configure" \
    --prefix="${dir_install}" \
    --with-sst-core="${dir_install}" \
    --without-pin
make dist
# AC_INIT([SST Elements Library], ...) names the tarball
# sst-elements-library-VERSION.
elements_tarball="$(find_tarball "${dir_elements_build}" 'sst-elements-*.tar.gz')"
mv "${elements_tarball}" "${dir_dist}/"
elements_tarball="${dir_dist}/$(basename "${elements_tarball}")"
popd
echo "::endgroup::"

echo "::group::replace the git checkouts with the tarballs"
rm -rf "${dir_core_src}" "${dir_elements_src}"
rm -rf "${dir_core_build}" "${dir_elements_build}" "${dir_install}"
unpack_as "${core_tarball}" "${dir_core_src}"
unpack_as "${elements_tarball}" "${dir_elements_src}"
ls -l "${dir_dist}" "${dir_core_src}" "${dir_elements_src}"
echo "::endgroup::"
