# librptext test suite
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(librpbase-tests LANGUAGES CXX)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# TextFuncsTest
ADD_EXECUTABLE(TextFuncsTest
	TextFuncsTest.cpp
	TextFuncsTest_data.hpp
	)
TARGET_LINK_LIBRARIES(TextFuncsTest PRIVATE rptest rptext)
TARGET_LINK_LIBRARIES(TextFuncsTest PRIVATE gtest)
DO_SPLIT_DEBUG(TextFuncsTest)
SET_WINDOWS_SUBSYSTEM(TextFuncsTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(TextFuncsTest wmain OFF)
ADD_TEST(NAME TextFuncsTest COMMAND TextFuncsTest --gtest_brief)
