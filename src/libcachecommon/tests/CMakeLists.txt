PROJECT(libcachecommon-tests)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# LibCacheCommon::filterCacheKey() test.
ADD_EXECUTABLE(FilterCacheKeyTest
	../../librpbase/tests/gtest_init.cpp
	FilterCacheKeyTest.cpp
	)
IF(WIN32)
	TARGET_LINK_LIBRARIES(FilterCacheKeyTest PRIVATE win32common)
ENDIF(WIN32)
TARGET_LINK_LIBRARIES(FilterCacheKeyTest PRIVATE rpbase cachecommon)
TARGET_LINK_LIBRARIES(FilterCacheKeyTest PRIVATE gtest)
DO_SPLIT_DEBUG(FilterCacheKeyTest)
SET_WINDOWS_SUBSYSTEM(FilterCacheKeyTest CONSOLE)
ADD_TEST(NAME FilterCacheKeyTest COMMAND FilterCacheKeyTest)

# Delay-load shell32.dll to prevent a performance penalty due to gdi32.dll.
# Reference: https://randomascii.wordpress.com/2018/12/03/a-not-called-function-can-cause-a-5x-slowdown/
# This is also needed when disabling direct Win32k syscalls,
# since loading gdi32.dll will crash in that case.
INCLUDE(../../libwin32common/DelayLoadHelper.cmake)
ADD_DELAYLOAD_FLAGS(FilterCacheKeyTest shell32.dll)
