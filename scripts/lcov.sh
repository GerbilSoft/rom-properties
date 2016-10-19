#!/bin/sh
# lcov code coverage script.
# Based on: https://github.com/bilke/cmake-modules/blob/master/CodeCoverage.cmake
# (commit 59f8ab8dded56b490dec388ac6ad449318de8779)
#
# Parameters:
# - $1: Build configuration.
# - $2: Base name for lcov files.
# - $3: Output directory.
#

if [ "$#" != "3" -o ! -f CMakeCache.txt ]; then
	echo "Syntax: $0 config basename output_directory"
	echo "Run this script from the top-level CMake build directory."
	exit 1
fi

CONFIG="$1"
coverage_info="$2.info"
coverage_cleaned="${coverage_info}.cleaned"
outputname="$3"

# Cleanup lcov.
lcov --directory . --zerocounters

# Run tests.
ctest -C "${CONFIG}"
if [ "$?" != "0" ]; then
	echo "*** WARNING: Some tests failed. Generating the lcov report anyway." >&2
fi

# Capturing lcov counters and generating report.
lcov --directory . --capture --output-file ${coverage_info}
lcov --remove ${coverage_info} 'tests/*' '/usr/*' --output-file ${coverage_cleaned}
genhtml -o ${outputname} ${coverage_cleaned}
rm -f ${coverage_info} ${coverage_cleaned}
exit 0
