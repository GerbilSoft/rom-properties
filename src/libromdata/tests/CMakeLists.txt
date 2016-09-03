PROJECT(libromdata-tests)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# Google Test
INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIR})

IF(NOT WIN32)
	# zlib
	# FIXME: crc32() replacement for the Windows build.
	FIND_LIBRARY(ZLIB REQUIRED)
	INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIRS})
	ADD_DEFINITIONS(-DZLIB_CONST)

	# libpng
	FIND_LIBRARY(PNG REQUIRED)
	INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIRS})
	ADD_DEFINITIONS(${PNG_DEFINITIONS})
ENDIF(NOT WIN32)

# Copy the reference images to the test directory.
FILE(COPY
	gl_triangle.ARGB32.png
	gl_triangle.RGB24.png
	gl_triangle.RGB24.tRNS.png
	gl_triangle.gray.png
	gl_quad.ARGB32.png
	gl_quad.RGB24.png
	gl_quad.RGB24.tRNS.png
	gl_quad.gray.png
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
	)


# RpImageLoader test.
IF(NOT WIN32)
# FIXME: Enable for Windows after implementing RpPng_gdiplus.cpp.
ADD_EXECUTABLE(RpImageLoaderTest RpImageLoaderTest.cpp)
TARGET_LINK_LIBRARIES(RpImageLoaderTest
	romdata8
	${PNG_LIBRARY}
	${ZLIB_LIBRARY}
	${GTEST_LIBRARY}
	)
DO_SPLIT_DEBUG(RpImageLoaderTest)
ADD_TEST(NAME RpImageLoaderTest
	COMMAND RpImageLoaderTest
	)
ENDIF(NOT WIN32)
