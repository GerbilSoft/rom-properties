# librptext test suite
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
DO_SPLIT_DEBUG(TextFuncsTest)
SET_WINDOWS_SUBSYSTEM(TextFuncsTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(TextFuncsTest wmain OFF)
ADD_TEST(NAME TextFuncsTest COMMAND TextFuncsTest --gtest_brief)
