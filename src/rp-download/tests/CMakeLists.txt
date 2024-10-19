# rp-download test suite
PROJECT(rp-download-tests LANGUAGES CXX)

# Top-level src directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# CacheKeyVerifyTest
# NOTE: We can't disable NLS here due to its usage
# in FstPrint.cpp. gtest_init.cpp will set LC_ALL=C.
ADD_EXECUTABLE(CacheKeyVerifyTest
	CacheKeyVerifyTest.cpp
	../CacheKeyVerify.cpp
	)
TARGET_LINK_LIBRARIES(CacheKeyVerifyTest PRIVATE cachecommon rptest)
IF(WIN32)
	TARGET_LINK_LIBRARIES(CacheKeyVerifyTest PRIVATE win32common)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(CacheKeyVerifyTest PRIVATE unixcommon)
ENDIF(WIN32)
DO_SPLIT_DEBUG(CacheKeyVerifyTest)
SET_WINDOWS_SUBSYSTEM(CacheKeyVerifyTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(CacheKeyVerifyTest wmain OFF)
ADD_TEST(NAME CacheKeyVerifyTest COMMAND CacheKeyVerifyTest --gtest_brief)
