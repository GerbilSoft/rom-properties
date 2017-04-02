PROJECT(libromdata-tests)

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
TARGET_LINK_LIBRARIES(RpImageLoaderTest romdata8)
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
FILE(GLOB test_images RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/img/png_data" img/png_data/*.png img/png_data/*.bmp.gz)
FOREACH(test_image ${test_images})
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
ENDFOREACH(test_image test_images)

IF(ENABLE_DECRYPTION)
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

	# CtrKeyScrambler test.
	ADD_EXECUTABLE(CtrKeyScramblerTest
		gtest_init.cpp
		CtrKeyScramblerTest.cpp
		)
	TARGET_LINK_LIBRARIES(CtrKeyScramblerTest romdata8)
	TARGET_LINK_LIBRARIES(CtrKeyScramblerTest gtest)
	DO_SPLIT_DEBUG(CtrKeyScramblerTest)
	SET_WINDOWS_SUBSYSTEM(CtrKeyScramblerTest CONSOLE)
	ADD_TEST(NAME CtrKeyScramblerTest COMMAND CtrKeyScramblerTest)
ENDIF(ENABLE_DECRYPTION)

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

# Copy the reference FSTs to:
# - bin/fst_data/ (TODO: Subdirectory?)
# - ${CMAKE_CURRENT_BINARY_DIR}/fst_data/
# NOTE: Although the test executable is in bin/, CTest still
# uses ${CMAKE_CURRENT_BINARY_DIR} as the working directory.
# Hence, we have to copy the files to both places.
FILE(GLOB test_fsts RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/disc/fst_data" disc/fst_data/*.zip)
FOREACH(test_fst ${test_fsts})
	ADD_CUSTOM_COMMAND(TARGET GcnFstTest POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/disc/fst_data/${test_fst}"
			"$<TARGET_FILE_DIR:GcnFstTest>/fst_data/${test_fst}"
		)
	ADD_CUSTOM_COMMAND(TARGET GcnFstTest POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/disc/fst_data/${test_fst}"
			"${CMAKE_CURRENT_BINARY_DIR}/fst_data/${test_fst}"
		)
ENDFOREACH(test_fst test_fsts)

# TextFuncsTest. (libromdata8)
ADD_EXECUTABLE(TextFuncsTest8
	gtest_init.cpp
	TextFuncsTest.cpp
	)
TARGET_LINK_LIBRARIES(TextFuncsTest8 romdata8)
TARGET_LINK_LIBRARIES(TextFuncsTest8 gtest)
DO_SPLIT_DEBUG(TextFuncsTest8)
SET_WINDOWS_SUBSYSTEM(TextFuncsTest8 CONSOLE)
ADD_TEST(NAME TextFuncsTest8 COMMAND TextFuncsTest8)

# TextFuncsTest. (libromdata16)
ADD_EXECUTABLE(TextFuncsTest16
	gtest_init.cpp
	TextFuncsTest.cpp
	TextFuncsTest_data.hpp
	)
TARGET_LINK_LIBRARIES(TextFuncsTest16 romdata16)
TARGET_LINK_LIBRARIES(TextFuncsTest16 gtest)
DO_SPLIT_DEBUG(TextFuncsTest16)
SET_WINDOWS_SUBSYSTEM(TextFuncsTest16 CONSOLE)
ADD_TEST(NAME TextFuncsTest16 COMMAND TextFuncsTest16)
