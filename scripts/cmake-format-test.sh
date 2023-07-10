#!/bin/bash

set -o pipefail

# This script will run cmake-format on directories in sst-core to test/verify format

# Check for running in the root dir of SST-Core
if [[ ! -f ./scripts/cmake-format-test.sh  ]]; then
    echo "ERROR: This script must be run from the top level root directory of SST-Core..."
    exit 1
fi

CMAKE_FORMAT_EXE="$(command -v cmake-format)"
CMAKE_FORMAT_ARG="--check"
CMAKE_FORMAT_OUTFILE="cmake_format_results.txt"
CMAKE_CHECK_FILES=".cmake_format_files.txt"
CMAKE_LINT_EXE="$(command -v cmake-lint)"
CMAKE_LINT_ARG="--suppress-decoration"
CMAKE_LINT_OUTFILE="cmake_lint_results.txt"

while :; do
    case $1 in
        -h | --help)
            echo "Run as scripts/cmake-format-test.sh [-i]"
            exit
            ;;
        -i)
            CMAKE_FORMAT_ARG="-i"
            shift
            ;;
        * )
            break
    esac
done

# Setup SST-Core Directories to be skipped for cmake-format checks
DIRS_TO_SKIP="-path ./build "
DIRS_TO_SKIP="$DIRS_TO_SKIP -or -path ./src/sst/core/libltdl"
DIRS_TO_SKIP="$DIRS_TO_SKIP -or -path ./external"
# Add additional directories to skip here...

echo "============================"
echo "=== FINDING CMAKE FILES  ==="
echo "============================"

# So that our hard work can be reused.
find . -type d \( $DIRS_TO_SKIP \) -prune -false \
     -o \( -type f -name '*.cmake' -o -name 'CMakeLists.txt' \) \
     -print > "${CMAKE_CHECK_FILES}"
rtncode=$?

echo "=== FIND FINISHED CHECKS WITH RTN CODE $rtncode"

echo "Using cmake-format '${CMAKE_FORMAT_EXE}' with arguments '${CMAKE_FORMAT_ARG}'."

echo "======================================="
echo "=== PERFORMING CMAKE-FORMAT TESTING ==="
echo "======================================="

< "${CMAKE_CHECK_FILES}" xargs "${CMAKE_FORMAT_EXE}" "${CMAKE_FORMAT_ARG}" \
  "--config-files=./experimental/.cmake-format.yaml" \
  > "${CMAKE_FORMAT_OUTFILE}" 2>&1
rtncode=$?
echo "=== CMAKE-FORMAT FINISHED CHECKS WITH RTN CODE $rtncode"

echo "Using cmake-lint '${CMAKE_LINT_EXE}' with arguments '${CMAKE_LINT_ARG}'."

echo "======================================="
echo "=== PERFORMING CMAKE-LINT TESTING ==="
echo "======================================="

< "${CMAKE_CHECK_FILES}" xargs "${CMAKE_LINT_EXE}" "${CMAKE_LINT_ARG}" \
  "--config-files=./experimental/.cmake-format.yaml" \
  > "${CMAKE_LINT_OUTFILE}" 2>&1
rtncode=$?
touch "${CMAKE_LINT_OUTFILE}"
echo "=== CMAKE-LINT FINISHED CHECKS WITH RTN CODE $rtncode"

# Set a test result return to a default rtn
export FINAL_TEST_RESULT=0

# Evaluate the Results
echo
if [[ -s "${CMAKE_FORMAT_OUTFILE}" ]]; then
  echo "=== CMAKE FORMAT RESULT FILE IS IS NOT EMPTY - FAILURE"
  cat "${CMAKE_FORMAT_OUTFILE}"
  export FINAL_TEST_RESULT=1
else
  echo "=== CMAKE FORMAT RESULT FILE IS EMPTY - SUCCESS"
fi
if [[ -s "${CMAKE_LINT_OUTFILE}" ]]; then
  echo "=== CMAKE LINT RESULT FILE IS IS NOT EMPTY - FAILURE"
  cat "${CMAKE_LINT_OUTFILE}"
  export FINAL_TEST_RESULT=1
else
  echo "=== CMAKE LINT RESULT FILE IS EMPTY - SUCCESS"
fi

# Display the final results
echo
echo "========================================"
if [[ $FINAL_TEST_RESULT == 0 ]]; then
    echo "=== FINAL TEST RESULT = ($FINAL_TEST_RESULT) - PASSED ==="
else
    echo "=== FINAL TEST RESULT = ($FINAL_TEST_RESULT) - FAILED ==="
fi
echo "========================================"
echo

exit $FINAL_TEST_RESULT
