#!/bin/bash
# This script will run cmake-format on directories in sst-core to test/verify format

# Check for running in the root dir of SST-Core
if [ ! -f ./scripts/cmake-format-test.sh  ]; then
    echo "ERROR: This script must be run from the top level root directory of SST-Core..."
    exit 1
fi

CMAKE_FORMAT_EXE="$(command -v cmake-format)"
CMAKE_FORMAT_ARG="--check"

echo "Using cmake-format '${CMAKE_FORMAT_EXE}' with arguments '${CMAKE_FORMAT_ARG}'."

# Setup SST-Core Directories to be skipped for cmake-format checks
DIRS_TO_SKIP="-path ./build "
DIRS_TO_SKIP="$DIRS_TO_SKIP -or -path ./src/sst/core/libltdl"
DIRS_TO_SKIP="$DIRS_TO_SKIP -or -path ./external"
# Add additional directories to skip here...

echo "======================================="
echo "=== PERFORMING CMAKE-FORMAT TESTING ==="
echo "======================================="

find . -type d \( $DIRS_TO_SKIP \) -prune -false \
     -o \( -type f -name '*.cmake' -o -name 'CMakeLists.txt' \) \
     -exec "${CMAKE_FORMAT_EXE}" "${CMAKE_FORMAT_ARG}" '{}' + > cmake_format_results.txt 2>&1
rtncode=$?
echo "=== CMAKE-FORMAT FINISHED CHECKS WITH RTN CODE $rtncode"

# Set a test result return to a default rtn
export FINAL_TEST_RESULT=0

# Evaluate the Results
echo
if [ -s ./cmake_format_results_h.txt ]; then
  echo "=== CMAKE FORMAT RESULT FILE IS IS NOT EMPTY - FAILURE"
  cat ./cmake_format_results.txt
  export FINAL_TEST_RESULT=1
else
  echo "=== CMAKE FORMAT RESULT FILE IS EMPTY - SUCCESS"
fi

# Display the final results
echo
echo "========================================"
if [ $FINAL_TEST_RESULT == 0 ]; then
echo "=== FINAL TEST RESULT = ($FINAL_TEST_RESULT) - PASSED ==="
else
echo "=== FINAL TEST RESULT = ($FINAL_TEST_RESULT) - FAILED ==="
fi
echo "========================================"
echo

exit $FINAL_TEST_RESULT
