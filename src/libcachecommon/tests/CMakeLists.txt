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
