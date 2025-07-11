#!/bin/bash

# elements_autotools_noflags_nodeps_nompi.sh: TODO

# shellcheck disable=SC2086
# https://web.archive.org/web/20230401201759/https://wiki.bash-hackers.org/scripting/debuggingtips#making_xtrace_more_useful
export PS4='+(${BASH_SOURCE}:${LINENO}): ${FUNCNAME[0]:+${FUNCNAME[0]}(): }'
set -x

set -eo pipefail

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "${SCRIPTDIR}"/compilers.bash

toolchain="${1}"

source_compilers_nompi "${toolchain}"

suffix="$(clean_suffix autotools_noflags_nodeps_nompi_${toolchain})"

dir_src="${PWD}"/sst-elements
dir_build="${PWD}"/sst-elements-build-${suffix}
dir_core="${PWD}"/install_${suffix}
dir_install="${dir_core}"

\rm -rf "${dir_build}" || true
# \rm -rf "${dir_install}" || true

pushd "${dir_src}"
git clean -Xdf .
./autogen.sh
popd

mkdir -p "${dir_build}"
pushd "${dir_build}"

[ -n "${INTEL_PIN_DIRECTORY}" ] && PIN_TEXT="--with-pin=${INTEL_PIN_DIRECTORY}" || PIN_TEXT="--without-pin"
INSTALL="$(command -v install) -p" "${dir_src}"/configure \
       "${PIN_TEXT}" \
       --with-sst-core="${dir_core}" \
       --prefix="${dir_install}"

bear_make_install
ln -fsv "${dir_build}"/compile_commands.json "${dir_src}"/compile_commands.json
