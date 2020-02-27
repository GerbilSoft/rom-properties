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

# librptest library.
ADD_LIBRARY(rptest STATIC gtest_init.cpp)
TARGET_LINK_LIBRARIES(rptest PRIVATE rpsecure)
IF(WIN32)
	# rptexture is only used for GDI+ rp_image backend registration on Windows.
	TARGET_LINK_LIBRARIES(rptest PRIVATE rptexture)
ENDIF(WIN32)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(rptest)

# RpImageLoader test.
ADD_EXECUTABLE(RpImageLoaderTest
	img/RpImageLoaderTest.cpp
	img/RpPngFormatTest.cpp
	)
TARGET_LINK_LIBRARIES(RpImageLoaderTest PRIVATE rptest rpbase)
TARGET_LINK_LIBRARIES(RpImageLoaderTest PRIVATE gtest ${ZLIB_LIBRARY})
TARGET_INCLUDE_DIRECTORIES(RpImageLoaderTest PRIVATE ${ZLIB_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS(RpImageLoaderTest PRIVATE ${ZLIB_DEFINITIONS})
IF(PNG_LIBRARY)
	TARGET_LINK_LIBRARIES(RpImageLoaderTest PRIVATE ${PNG_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(RpImageLoaderTest PRIVATE ${PNG_INCLUDE_DIRS})
	TARGET_COMPILE_DEFINITIONS(RpImageLoaderTest PRIVATE ${PNG_DEFINITIONS})
ENDIF(PNG_LIBRARY)
DO_SPLIT_DEBUG(RpImageLoaderTest)
SET_WINDOWS_SUBSYSTEM(RpImageLoaderTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(RpImageLoaderTest wmain OFF)
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
	ADD_EXECUTABLE(AesCipherTest AesCipherTest.cpp)
	TARGET_LINK_LIBRARIES(AesCipherTest PRIVATE rptest rpbase)
	TARGET_LINK_LIBRARIES(AesCipherTest PRIVATE gtest)
	IF(WIN32)
		TARGET_LINK_LIBRARIES(AesCipherTest PRIVATE advapi32)
	ENDIF(WIN32)
	IF(NETTLE_LIBRARY)
		TARGET_LINK_LIBRARIES(AesCipherTest PRIVATE ${NETTLE_LIBRARY})
		TARGET_INCLUDE_DIRECTORIES(AesCipherTest PRIVATE ${NETTLE_INCLUDE_DIRS})
	ENDIF(NETTLE_LIBRARY)
	DO_SPLIT_DEBUG(AesCipherTest)
	SET_WINDOWS_SUBSYSTEM(AesCipherTest CONSOLE)
	SET_WINDOWS_ENTRYPOINT(AesCipherTest wmain OFF)
	ADD_TEST(NAME AesCipherTest COMMAND AesCipherTest)
ENDIF(ENABLE_DECRYPTION)

# TextFuncsTest.
ADD_EXECUTABLE(TextFuncsTest
	TextFuncsTest.cpp
	TextFuncsTest_data.hpp
	)
TARGET_LINK_LIBRARIES(TextFuncsTest PRIVATE rptest rpbase)
TARGET_LINK_LIBRARIES(TextFuncsTest PRIVATE gtest)
DO_SPLIT_DEBUG(TextFuncsTest)
SET_WINDOWS_SUBSYSTEM(TextFuncsTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(TextFuncsTest wmain OFF)
ADD_TEST(NAME TextFuncsTest COMMAND TextFuncsTest)

# ByteswapTest.
ADD_EXECUTABLE(ByteswapTest
	ByteswapTest.cpp
	ByteswapTest_data.hpp
	)
TARGET_LINK_LIBRARIES(ByteswapTest PRIVATE rptest rpbase)
TARGET_LINK_LIBRARIES(ByteswapTest PRIVATE gtest)
DO_SPLIT_DEBUG(ByteswapTest)
SET_WINDOWS_SUBSYSTEM(ByteswapTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(ByteswapTest wmain OFF)
ADD_TEST(NAME ByteswapTest COMMAND ByteswapTest "--gtest_filter=-*benchmark*")
