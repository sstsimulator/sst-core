#!/bin/bash
# This script will run clang-format on directories in sst-core to test/verify format

# Check for running in the root dir of SST-Core
if [ ! -f ./scripts/clang-format-test.sh  ]; then
    echo "ERROR: This script must be run from the top level root directory of SST-Core..."
    exit 1
fi


pwd

# Setup SST-Core Directories to be skipped for clang-format checks
DIRS_TO_SKIP="-path ./build "
DIRS_TO_SKIP="$DIRS_TO_SKIP -or -path ./src/sst/core/libltdl"
DIRS_TO_SKIP="$DIRS_TO_SKIP -or -path ./src/sst/core/tinyxml"
# Add additional directories to skip here...

echo "======================================="
echo "=== PERFORMING CLANG-FORMAT TESTING ==="
echo "======================================="

# Run clang-format on all specific .h files
echo
find . -type d \( $DIRS_TO_SKIP \) -prune -false -o -name '*.h' -exec clang-format --dry-run {} \;  > clang_format_results_h.txt 2>&1
rtncode=$?
echo "=== CLANG-FORMAT FINISHED *.h CHECKS WITH RTN CODE $rtncode"

# Run clang-format on all specific .cc files
echo
find . -type d \( $DIRS_TO_SKIP \) -prune -false -o -name '*.cc' -exec clang-format --dry-run {} \; > clang_format_results_cc.txt 2>&1
rtncode=$?
echo "=== CLANG-FORMAT FINISHED *.cc CHECKS WITH RTN CODE $rtncode"

# Set a test result return to a default rtn
export FINAL_TEST_RESULT=0

# Evaluate the Results
echo
if [ -s ./clang_format_results_h.txt ]; then
  echo "=== CLANG FORMAT RESULT FILE FOR .h FILES IS IS NOT EMPTY - FAILURE"
  cat ./clang_format_results_h.txt
  export FINAL_TEST_RESULT=1
else
  echo "=== CLANG FORMAT RESULT FILE FOR .h FILES IS EMPTY - SUCCESS"
fi

echo
if [ -s ./clang_format_results_cc.txt ]; then
  echo "=== CLANG FORMAT RESULT FILE FOR .cc FILES IS IS NOT EMPTY - FAILURE"
  cat ./clang_format_results_cc.txt
  export FINAL_TEST_RESULT=1
else
  echo "=== CLANG FORMAT RESULT FILE FOR .cc FILES IS EMPTY - SUCCESS"
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
