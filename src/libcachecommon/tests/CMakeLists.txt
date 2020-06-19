CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(libcachecommon-tests LANGUAGES CXX)

IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# LibCacheCommon::filterCacheKey() test.
ADD_EXECUTABLE(FilterCacheKeyTest FilterCacheKeyTest.cpp)
TARGET_LINK_LIBRARIES(FilterCacheKeyTest PRIVATE rptest rpbase cachecommon)
TARGET_LINK_LIBRARIES(FilterCacheKeyTest PRIVATE gtest)
DO_SPLIT_DEBUG(FilterCacheKeyTest)
SET_WINDOWS_SUBSYSTEM(FilterCacheKeyTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(FilterCacheKeyTest wmain OFF)
ADD_TEST(NAME FilterCacheKeyTest COMMAND FilterCacheKeyTest)

# Delay-load shell32.dll and ole32.dll to prevent a performance penalty due to gdi32.dll.
# Reference: https://randomascii.wordpress.com/2018/12/03/a-not-called-function-can-cause-a-5x-slowdown/
# This is also needed when disabling direct Win32k syscalls,
# since loading gdi32.dll will crash in that case.
# NOTE: ole32.dll is indirectly linked through libwin32common. (CoTaskMemFree())
INCLUDE(../../libwin32common/DelayLoadHelper.cmake)
ADD_DELAYLOAD_FLAGS(FilterCacheKeyTest shell32.dll ole32.dll)
