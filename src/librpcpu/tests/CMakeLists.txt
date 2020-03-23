PROJECT(librpcpu-tests)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# ByteswapTest
ADD_EXECUTABLE(ByteswapTest
	ByteswapTest.cpp
	ByteswapTest_data.hpp
	)
TARGET_LINK_LIBRARIES(ByteswapTest PRIVATE rptest rpcpu)
TARGET_LINK_LIBRARIES(ByteswapTest PRIVATE gtest)
DO_SPLIT_DEBUG(ByteswapTest)
SET_WINDOWS_SUBSYSTEM(ByteswapTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(ByteswapTest wmain OFF)
ADD_TEST(NAME ByteswapTest COMMAND ByteswapTest "--gtest_filter=-*benchmark*")
