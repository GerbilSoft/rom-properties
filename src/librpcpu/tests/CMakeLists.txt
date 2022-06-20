# librpcpu test suite
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(librpcpu-tests LANGUAGES CXX)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# ByteswapTest
ADD_EXECUTABLE(ByteswapTest
	ByteswapTest.cpp
	ByteswapTest_data.hpp
	)
TARGET_LINK_LIBRARIES(ByteswapTest PRIVATE rptest romdata)
TARGET_COMPILE_DEFINITIONS(ByteswapTest PRIVATE RP_BUILDING_FOR_DLL=1)
TARGET_LINK_LIBRARIES(ByteswapTest PRIVATE gtest)
DO_SPLIT_DEBUG(ByteswapTest)
SET_WINDOWS_SUBSYSTEM(ByteswapTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(ByteswapTest wmain OFF)
ADD_TEST(NAME ByteswapTest COMMAND ByteswapTest "--gtest_filter=-*benchmark*")
