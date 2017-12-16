PROJECT(librpbase-tests)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# ZLIB and libpng are checked in the top-level CMakeLists.txt.

IF(NOT WIN32)
	IF(ENABLE_DECRYPTION)
		FIND_LIBRARY(Nettle REQUIRED)
	ENDIF(ENABLE_DECRYPTION)
ENDIF(NOT WIN32)

# RpImageLoader test.
ADD_EXECUTABLE(RpImageLoaderTest
	gtest_init.cpp
	img/RpImageLoaderTest.cpp
	img/RpPngFormatTest.cpp
	)
IF(WIN32)
	TARGET_LINK_LIBRARIES(RpImageLoaderTest win32common)
ENDIF(WIN32)
TARGET_LINK_LIBRARIES(RpImageLoaderTest rpbase)
TARGET_LINK_LIBRARIES(RpImageLoaderTest gtest ${ZLIB_LIBRARY})
TARGET_INCLUDE_DIRECTORIES(RpImageLoaderTest PRIVATE ${ZLIB_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS(RpImageLoaderTest PRIVATE ${ZLIB_DEFINITIONS})
IF(PNG_LIBRARY)
	TARGET_LINK_LIBRARIES(RpImageLoaderTest ${PNG_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(RpImageLoaderTest PRIVATE ${PNG_INCLUDE_DIRS})
	TARGET_COMPILE_DEFINITIONS(RpImageLoaderTest PRIVATE ${PNG_DEFINITIONS})
ENDIF(PNG_LIBRARY)
DO_SPLIT_DEBUG(RpImageLoaderTest)
SET_WINDOWS_SUBSYSTEM(RpImageLoaderTest CONSOLE)
ADD_TEST(NAME RpImageLoaderTest COMMAND RpImageLoaderTest)

# Copy the reference images to:
# - bin/png_data/ (TODO: Subdirectory?)
# - ${CMAKE_CURRENT_BINARY_DIR}/png_data/
# NOTE: Although the test executable is in bin/, CTest still
# uses ${CMAKE_CURRENT_BINARY_DIR} as the working directory.
# Hence, we have to copy the files to both places.
FILE(GLOB RpImageLoaderTest_images RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/img/png_data" img/png_data/*.png img/png_data/*.bmp.gz)
FOREACH(test_image ${RpImageLoaderTest_images})
	ADD_CUSTOM_COMMAND(TARGET RpImageLoaderTest POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/img/png_data/${test_image}"
			"$<TARGET_FILE_DIR:RpImageLoaderTest>/png_data/${test_image}"
		)
	ADD_CUSTOM_COMMAND(TARGET RpImageLoaderTest POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/img/png_data/${test_image}"
			"${CMAKE_CURRENT_BINARY_DIR}/png_data/${test_image}"
		)
ENDFOREACH(test_image ${RpImageLoaderTest_images})

IF(ENABLE_DECRYPTION)
	# AesCipher test.
	ADD_EXECUTABLE(AesCipherTest
		gtest_init.cpp
		AesCipherTest.cpp
		)
	TARGET_LINK_LIBRARIES(AesCipherTest rpbase)
	TARGET_LINK_LIBRARIES(AesCipherTest gtest)
	IF(WIN32)
		TARGET_LINK_LIBRARIES(AesCipherTest win32common)
		TARGET_LINK_LIBRARIES(AesCipherTest advapi32)
	ENDIF(WIN32)
	IF(NETTLE_LIBRARY)
		TARGET_LINK_LIBRARIES(AesCipherTest ${NETTLE_LIBRARY})
		TARGET_INCLUDE_DIRECTORIES(AesCipherTest PRIVATE ${NETTLE_INCLUDE_DIRS})
	ENDIF(NETTLE_LIBRARY)
	DO_SPLIT_DEBUG(AesCipherTest)
	SET_WINDOWS_SUBSYSTEM(AesCipherTest CONSOLE)
	ADD_TEST(NAME AesCipherTest COMMAND AesCipherTest)
ENDIF(ENABLE_DECRYPTION)

# TextFuncsTest.
ADD_EXECUTABLE(TextFuncsTest
	gtest_init.cpp
	TextFuncsTest.cpp
	TextFuncsTest_data.hpp
	)
IF(WIN32)
	TARGET_LINK_LIBRARIES(TextFuncsTest win32common)
ENDIF(WIN32)
TARGET_LINK_LIBRARIES(TextFuncsTest rpbase)
TARGET_LINK_LIBRARIES(TextFuncsTest gtest)
DO_SPLIT_DEBUG(TextFuncsTest)
SET_WINDOWS_SUBSYSTEM(TextFuncsTest CONSOLE)
ADD_TEST(NAME TextFuncsTest COMMAND TextFuncsTest)

# ImageDecoderLinear test.
# TODO: Move to libromdata, or move libromdata stuff here?
ADD_EXECUTABLE(ImageDecoderLinearTest
	../../librpbase/tests/gtest_init.cpp
	img/ImageDecoderLinearTest.cpp
	)
IF(WIN32)
	TARGET_LINK_LIBRARIES(ImageDecoderLinearTest win32common)
ENDIF(WIN32)
TARGET_LINK_LIBRARIES(ImageDecoderLinearTest romdata)
TARGET_LINK_LIBRARIES(ImageDecoderLinearTest gtest ${ZLIB_LIBRARY})
TARGET_INCLUDE_DIRECTORIES(ImageDecoderLinearTest PRIVATE ${ZLIB_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS(ImageDecoderLinearTest PRIVATE ${ZLIB_DEFINITIONS})
IF(PNG_LIBRARY)
	TARGET_LINK_LIBRARIES(ImageDecoderLinearTest ${PNG_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(ImageDecoderLinearTest PRIVATE ${PNG_INCLUDE_DIRS})
	TARGET_COMPILE_DEFINITIONS(ImageDecoderLinearTest PRIVATE ${PNG_DEFINITIONS})
ENDIF(PNG_LIBRARY)
DO_SPLIT_DEBUG(ImageDecoderLinearTest)
SET_WINDOWS_SUBSYSTEM(ImageDecoderLinearTest CONSOLE)
ADD_TEST(NAME ImageDecoderLinearTest COMMAND ImageDecoderLinearTest "--gtest_filter=-*benchmark*")

# ByteswapTest.
ADD_EXECUTABLE(ByteswapTest
	gtest_init.cpp
	ByteswapTest.cpp
	ByteswapTest_data.hpp
	)
IF(WIN32)
	TARGET_LINK_LIBRARIES(ByteswapTest win32common)
ENDIF(WIN32)
TARGET_LINK_LIBRARIES(ByteswapTest rpbase)
TARGET_LINK_LIBRARIES(ByteswapTest gtest)
DO_SPLIT_DEBUG(ByteswapTest)
SET_WINDOWS_SUBSYSTEM(ByteswapTest CONSOLE)
ADD_TEST(NAME ByteswapTest COMMAND ByteswapTest)
