PROJECT(libromdata-tests)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

IF(WIN32)
	# Use extlib/zlib/.
	SET(ZLIB_LIBRARY zlibstatic)
ELSE(WIN32)
	FIND_LIBRARY(ZLIB REQUIRED)
	FIND_LIBRARY(PNG REQUIRED)

	IF(ENABLE_DECRYPTION)
		FIND_LIBRARY(Nettle REQUIRED)
	ENDIF(ENABLE_DECRYPTION)
ENDIF(WIN32)

# Copy the reference images to the test directory.
FILE(GLOB test_images img/*.png img/*.bmp.gz)
FILE(COPY ${test_images} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

# RpImageLoader test.
ADD_EXECUTABLE(RpImageLoaderTest
	gtest_init.cpp
	img/RpImageLoaderTest.cpp
	img/RpPngFormatTest.cpp
	)
TARGET_LINK_LIBRARIES(RpImageLoaderTest romdata8)
TARGET_LINK_LIBRARIES(RpImageLoaderTest gtest ${ZLIB_LIBRARY})
TARGET_COMPILE_DEFINITIONS(RpImageLoaderTest PRIVATE ZLIB_CONST)
IF(PNG_LIBRARY)
	TARGET_LINK_LIBRARIES(RpImageLoaderTest ${PNG_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(RpImageLoaderTest PRIVATE ${PNG_INCLUDE_DIRS})
ENDIF(PNG_LIBRARY)
DO_SPLIT_DEBUG(RpImageLoaderTest)
SET_WINDOWS_SUBSYSTEM(RpImageLoaderTest CONSOLE)
ADD_TEST(NAME RpImageLoaderTest COMMAND RpImageLoaderTest)

# AesCipher test.
ADD_EXECUTABLE(AesCipherTest
	gtest_init.cpp
	AesCipherTest.cpp
	)
TARGET_LINK_LIBRARIES(AesCipherTest romdata8)
TARGET_LINK_LIBRARIES(AesCipherTest gtest)
IF(WIN32)
	TARGET_LINK_LIBRARIES(AesCipherTest advapi32)
ENDIF(WIN32)
IF(NETTLE_LIBRARY)
	TARGET_LINK_LIBRARIES(AesCipherTest ${NETTLE_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(AesCipherTest PRIVATE ${NETTLE_INCLUDE_DIRS})
ENDIF(NETTLE_LIBRARY)
DO_SPLIT_DEBUG(AesCipherTest)
SET_WINDOWS_SUBSYSTEM(AesCipherTest CONSOLE)
ADD_TEST(NAME AesCipherTest COMMAND AesCipherTest)

# GcnFstPrint. (Not a test, but a useful program.)
ADD_EXECUTABLE(GcnFstPrint
	gtest_init.cpp
	disc/FstPrint.cpp
	disc/FstPrint.hpp
	disc/GcnFstPrint.cpp
	)
TARGET_LINK_LIBRARIES(GcnFstPrint romdata8)
DO_SPLIT_DEBUG(GcnFstPrint)
SET_WINDOWS_SUBSYSTEM(GcnFstPrint CONSOLE)

# Copy the FST test files to the test directory.
FILE(GLOB test_fst disc/*.fst.bin.zip disc/*.fst.txt.zip)
FILE(COPY ${test_fst} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

# GcnFstTest.
ADD_EXECUTABLE(GcnFstTest
	gtest_init.cpp
	disc/FstPrint.cpp
	disc/FstPrint.hpp
	disc/GcnFstTest.cpp
	)
TARGET_LINK_LIBRARIES(GcnFstTest romdata8)
TARGET_LINK_LIBRARIES(GcnFstTest gtest)
TARGET_LINK_LIBRARIES(GcnFstTest minizip ${ZLIB_LIBRARY})
DO_SPLIT_DEBUG(GcnFstTest)
SET_WINDOWS_SUBSYSTEM(GcnFstTest CONSOLE)
ADD_TEST(NAME GcnFstTest COMMAND GcnFstTest)
