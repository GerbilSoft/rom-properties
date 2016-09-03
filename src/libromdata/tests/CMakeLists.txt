PROJECT(libromdata-tests)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# Google Test
INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIR})

# zlib
FIND_LIBRARY(ZLIB REQUIRED)
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIRS})
ADD_DEFINITIONS(-DZLIB_CONST)

# libpng
FIND_LIBRARY(PNG REQUIRED)
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIRS})
ADD_DEFINITIONS(${PNG_DEFINITIONS})

# Copy the reference images to the test directory.
FILE(COPY
	gl_quad.ARGB32.png
	gl_quad.RGB24.png
	gl_quad.RGB24.tRNS.png
	gl_triangle.ARGB32.png
	gl_triangle.RGB24.png
	gl_triangle.RGB24.tRNS.png
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
	)


# RpImageLoader test.
ADD_EXECUTABLE(RpImageLoaderTest RpImageLoaderTest.cpp)
TARGET_LINK_LIBRARIES(RpImageLoaderTest romdata8 ${GTEST_LIBRARY})
DO_SPLIT_DEBUG(RpImageLoaderTest)
ADD_TEST(NAME RpImageLoaderTest
	COMMAND RpImageLoaderTest
	)
