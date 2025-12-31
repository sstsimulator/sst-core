#!/bin/bash

set -eu -o pipefail

if ! root=$(git rev-parse --show-toplevel); then
    echo "$0 needs to be run in a SST Core Git working tree" >&2
    exit 2
fi

cd "${root}"

rebuild=0
fix=0
picky=0
checks=""

while [[ $# -ne 0 ]]; do
    case $1 in
        --checks) shift; checks=${1:?--checks needs an argument} ;;
        --fix) fix=1 ;;
        --rebuild) rebuild=1 ;;
        --picky) picky=1 ;;
        *) cat <<EOF >&2
Run clang-tidy on the SST Core sources

    $0 [ options ]

Options:

    --checks <checks>   Set the checks to use, similar to clang-tidy --checks

    --picky             Enable more picky checks; ignored if --checks is used
                        Default checks are reasonable for SST but not too picky

    --fix               Make clang-tidy automatically fix found warnings
                        (disables parallelism because it can corrupt files)

    --rebuild           Rebuild the compile_commands.json with Bear
                        (implied if compile_commands.json is not found)

EOF
           exit 2 ;;
    esac
    shift
done

if [[ ! -e "build/compile_commands.json" || ${rebuild} -ne 0 ]]; then
    if ! command -v bear >/dev/null 2>&1; then
        cat <<EOF >&2
The Bear utility needs to be installed (e.g. sudo apt install bear, brew install
bear, etc.). The Bear utility creates compile_commands.json from Autotools builds.
https://github.com/rizsotto/Bear
EOF
        exit 2
    fi
    if [[ ! -e build/config.status ]]; then
        echo "The build subdirectory needs to be created, autogen.sh run, and configure run." >&2
        exit 2
    fi
    cd build
    rm -rf clang-tidy.out compile_commands.json src
    make -j clean
    bear -- make install
    cd ..
fi

if [[ ${fix} -ne 0 ]]; then
    clang_tidy_fix="--fix --fix-errors --fix-notes"
    njobs=1
else
    clang_tidy_fix=""
    njobs=0
fi

if [[ -n ${checks} ]]; then
    # Many clang-analyzer-* checks have to be individually disabled. If wildcard -clang-analyzer-*
    # is used, clang-tidy will fail with "no checks" even if additional checks appear after them.
    clang_tidy_checks="-clang-analyzer-apiModeling.*,-clang-analyzer-cplusplus.*,-clang-analyzer-deadcode.*,-clang-analyzer-fuchsia.*,-clang-analyzer-nullability.*,-clang-analyzer-optin.*,-clang-analyzer-osx.*,-clang-analyzer-security.*,-clang-analyzer-unix.*,-clang-analyzer-valist.*,-clang-analyzer-webkit.*,${checks}"
else
    # These are the default checks for --picky. Even for --picky, many of them are disabled.
    clang_tidy_checks="bugprone-*,performance-*,modernize-*,readability-redundant-*,-modernize-use-trailing-return-type,-modernize-avoid-c-arrays,-modernize-use-override,-modernize-use-nodiscard,-modernize-macro-to-enum,-performance-enum-size,-modernize-use-auto,-bugprone-easily-swappable-parameters,-bugprone-reserved-identifier,google-explicit-constructor,misc-static-assert,misc-header-include-cycle,misc-definitions-in-headers,-modernize-return-braced-init-list,-clang-analyzer-cplusplus.NewDeleteLeaks"

    if [[ ${picky} -eq 0 ]]; then
        # This disables more checks when --picky is not used.
        clang_tidy_checks="${clang_tidy_checks},-readability-redundant-inline-specifier,-modernize-use-equals-default,-performance-avoid-endl,-modernize-pass-by-value,-modernize-use-default-member-init,-readability-redundant-string-init,-modernize-use-equals-delete,-performance-unnecessary-copy-initialization,-readability-redundant-access-specifiers,-modernize-use-bool-literals,-readability-redundant-member-init,-performance-unnecessary-value-param,-bugprone-narrowing-conversions"
    fi
fi

cd build
output=$(realpath clang-tidy.out)

echo "Using clang-tidy --checks=${clang_tidy_checks}"

perl -ne 'print "$1\0" if /"file":\s*"([^"]+)"/ && !m:/external/:' compile_commands.json | \
    xargs -0 -P "${njobs}" -I {} timeout 30 sh -c 'echo "Checking {}"; clang-tidy '"${clang_tidy_fix}"' --quiet --format-style=file -p . --system-headers=0 --checks="'"${clang_tidy_checks}"'" "{}"' | tee "${output}" || true

echo "Results are in ${output}"
