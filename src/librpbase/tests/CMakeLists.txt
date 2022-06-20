# librpbase test suite
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
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

# ZLIB and libpng are checked in the top-level CMakeLists.txt.

IF(NOT WIN32)
	IF(ENABLE_DECRYPTION)
		INCLUDE(CheckNettle2or3)
		CHECK_NETTLE_2_OR_3()
	ENDIF(ENABLE_DECRYPTION)
ENDIF(NOT WIN32)

# librptest library
ADD_LIBRARY(rptest STATIC gtest_init.cpp)
TARGET_LINK_LIBRARIES(rptest PUBLIC rpsecure)
IF(WIN32)
	# rptexture is only used for GDI+ rp_image backend registration on Windows.
	# Since it's part of libromdata when linked as a shared library, we'll
	# need to link in romdata instead of rptexture.
	TARGET_LINK_LIBRARIES(rptest PRIVATE romdata)
	TARGET_COMPILE_DEFINITIONS(rptest PRIVATE RP_BUILDING_FOR_DLL=1)
ENDIF(WIN32)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(rptest)

# RpImageLoader test
ADD_EXECUTABLE(RpImageLoaderTest
	img/RpImageLoaderTest.cpp
	img/RpPngFormatTest.cpp
	)
TARGET_LINK_LIBRARIES(RpImageLoaderTest PRIVATE rptest romdata)
TARGET_COMPILE_DEFINITIONS(RpImageLoaderTest PRIVATE RP_BUILDING_FOR_DLL=1)
TARGET_LINK_LIBRARIES(RpImageLoaderTest PRIVATE gtest ${ZLIB_LIBRARIES})
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
	# Crypto tests
	ADD_EXECUTABLE(CryptoTests AesCipherTest.cpp MD5HashTest.cpp)
	TARGET_LINK_LIBRARIES(CryptoTests PRIVATE rptest romdata)
	TARGET_COMPILE_DEFINITIONS(CryptoTests PRIVATE RP_BUILDING_FOR_DLL=1)
	TARGET_LINK_LIBRARIES(CryptoTests PRIVATE gtest)
	IF(WIN32)
		TARGET_LINK_LIBRARIES(CryptoTests PRIVATE advapi32)
	ENDIF(WIN32)
	IF(NETTLE_LIBRARY)
		TARGET_LINK_LIBRARIES(CryptoTests PRIVATE ${NETTLE_LIBRARY})
		TARGET_INCLUDE_DIRECTORIES(CryptoTests PRIVATE ${NETTLE_INCLUDE_DIRS})
	ENDIF(NETTLE_LIBRARY)
	DO_SPLIT_DEBUG(CryptoTests)
	SET_WINDOWS_SUBSYSTEM(CryptoTests CONSOLE)
	SET_WINDOWS_ENTRYPOINT(CryptoTests wmain OFF)
	ADD_TEST(NAME CryptoTests COMMAND CryptoTests)
ENDIF(ENABLE_DECRYPTION)

# TextFuncsTest
ADD_EXECUTABLE(TextFuncsTest
	TextFuncsTest.cpp
	TextFuncsTest_data.hpp
	)
TARGET_LINK_LIBRARIES(TextFuncsTest PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(TextFuncsTest PRIVATE gtest)
DO_SPLIT_DEBUG(TextFuncsTest)
SET_WINDOWS_SUBSYSTEM(TextFuncsTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(TextFuncsTest wmain OFF)
ADD_TEST(NAME TextFuncsTest COMMAND TextFuncsTest)

# TimegmTest
ADD_EXECUTABLE(TimegmTest TimegmTest.cpp)
TARGET_LINK_LIBRARIES(TimegmTest PRIVATE rptest romdata)
TARGET_COMPILE_DEFINITIONS(TimegmTest PRIVATE RP_BUILDING_FOR_DLL=1)
TARGET_LINK_LIBRARIES(TimegmTest PRIVATE gtest)
DO_SPLIT_DEBUG(TimegmTest)
SET_WINDOWS_SUBSYSTEM(TimegmTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(TimegmTest wmain OFF)
ADD_TEST(NAME TimegmTest COMMAND TimegmTest)
