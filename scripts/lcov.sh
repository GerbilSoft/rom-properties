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
coverage_base_info="$2.base.info"
coverage_test_info="$2.test.info"
coverage_info="$2.info"
coverage_cleaned="${coverage_info}.cleaned"
outputname="$3"

# Cleanup lcov.
lcov --directory . --zerocounters

# Create baseline coverage data file.
# This ensures we get data for files that aren't loaded.
# References:
# - https://stackoverflow.com/questions/44203156/can-lcov-genhtml-show-files-that-were-never-executed
# - https://stackoverflow.com/a/45105825
lcov -c -i -d . -o ${coverage_base_info}

# Run tests.
ctest -C "${CONFIG}"
if [ "$?" != "0" ]; then
	echo "*** WARNING: Some tests failed. Generating the lcov report anyway." >&2
fi

# Capture lcov output from the unit tests.
lcov -c -d . -o ${coverage_test_info}

# Combine baseline and unit test output.
lcov -a ${coverage_base_info} -a ${coverage_test_info} -o ${coverage_info}

# Remove third-party libraries and generated sources.
lcov -o ${coverage_cleaned} -r ${coverage_info} \
	'*/tests/*' '/usr/*' '*/extlib/*' \
	'*/moc_*.cpp' '*.moc' '*/ui_*.h' '*/qrc_*.cpp' \
	'*/glibresources.c' \
	'*/NetworkManager.c' \
	'*/networkmanagerinterface.cpp' \
	'*/networkmanagerinterface.h' \
	'*/Notifications.c' \
	'*/notificationsinterface.cpp' \
	'*/notificationsinterface.h' \
	'*/SpecializedThumbnailer1.c' \
	'*/SpecializedThumbnailer1.h' \
	'*/src/librpbase/img/pngcheck/pngcheck.cpp' \
	'*/libi18n/gettext.h'

# Generate the HTML report.
genhtml -o ${outputname} ${coverage_cleaned}
rm -f ${coverage_base_info} ${coverage_test_info} ${coverage_info} ${coverage_cleaned}
exit 0
