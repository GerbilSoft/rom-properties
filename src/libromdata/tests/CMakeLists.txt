PROJECT(libromdata-tests)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

IF(ENABLE_DECRYPTION)
	# CtrKeyScrambler test.
	ADD_EXECUTABLE(CtrKeyScramblerTest
		../../librpbase/tests/gtest_init.cpp
		CtrKeyScramblerTest.cpp
		)
	TARGET_LINK_LIBRARIES(CtrKeyScramblerTest romdata rpbase)
	TARGET_LINK_LIBRARIES(CtrKeyScramblerTest gtest)
	DO_SPLIT_DEBUG(CtrKeyScramblerTest)
	SET_WINDOWS_SUBSYSTEM(CtrKeyScramblerTest CONSOLE)
	ADD_TEST(NAME CtrKeyScramblerTest COMMAND CtrKeyScramblerTest)
ENDIF(ENABLE_DECRYPTION)

# GcnFstPrint. (Not a test, but a useful program.)
ADD_EXECUTABLE(GcnFstPrint
	disc/FstPrint.cpp
	disc/FstPrint.hpp
	disc/GcnFstPrint.cpp
	)
TARGET_LINK_LIBRARIES(GcnFstPrint romdata rpbase)
IF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(GcnFstPrint i18n)
ENDIF(ENABLE_NLS)
DO_SPLIT_DEBUG(GcnFstPrint)
SET_WINDOWS_SUBSYSTEM(GcnFstPrint CONSOLE)

# GcnFstTest.
# NOTE: We can't disable NLS here due to its usage
# in FstPrint.cpp. gtest_init.cpp will set LC_ALL=C.
ADD_EXECUTABLE(GcnFstTest
	../../librpbase/tests/gtest_init.cpp
	disc/FstPrint.cpp
	disc/FstPrint.hpp
	disc/GcnFstTest.cpp
	)
TARGET_LINK_LIBRARIES(GcnFstTest romdata rpbase)
IF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(GcnFstPrint i18n)
ENDIF(ENABLE_NLS)
TARGET_LINK_LIBRARIES(GcnFstTest gtest)
TARGET_LINK_LIBRARIES(GcnFstTest libminizip ${ZLIB_LIBRARY})
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

# ImageDecoder test.
ADD_EXECUTABLE(ImageDecoderTest
	../../librpbase/tests/gtest_init.cpp
	img/ImageDecoderTest.cpp
	)
IF(WIN32)
	TARGET_LINK_LIBRARIES(ImageDecoderTest win32common)
ENDIF(WIN32)
TARGET_LINK_LIBRARIES(ImageDecoderTest romdata rpbase)
TARGET_LINK_LIBRARIES(ImageDecoderTest gtest ${ZLIB_LIBRARY})
TARGET_INCLUDE_DIRECTORIES(ImageDecoderTest PRIVATE ${ZLIB_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS(ImageDecoderTest PRIVATE ${ZLIB_DEFINITIONS})
IF(PNG_LIBRARY)
	TARGET_LINK_LIBRARIES(ImageDecoderTest ${PNG_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(ImageDecoderTest PRIVATE ${PNG_INCLUDE_DIRS})
	TARGET_COMPILE_DEFINITIONS(ImageDecoderTest PRIVATE ${PNG_DEFINITIONS})
ENDIF(PNG_LIBRARY)
DO_SPLIT_DEBUG(ImageDecoderTest)
SET_WINDOWS_SUBSYSTEM(ImageDecoderTest CONSOLE)
ADD_TEST(NAME ImageDecoderTest COMMAND ImageDecoderTest "--gtest_filter=-*benchmark*")

# Copy the reference images to:
# - bin/ImageDecoder_data/ (TODO: Subdirectory?)
# - ${CMAKE_CURRENT_BINARY_DIR}/ImageDecoder_data/
# NOTE: Although the test executable is in bin/, CTest still
# uses ${CMAKE_CURRENT_BINARY_DIR} as the working directory.
# Hence, we have to copy the files to both places.
FOREACH(test_dir Alpha ARGB Luma RGB S3TC PVR GVR KTX VTF tctest)
	ADD_CUSTOM_COMMAND(TARGET ImageDecoderTest POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E make_directory
			"$<TARGET_FILE_DIR:ImageDecoderTest>/ImageDecoder_data/${test_dir}"
		)
	ADD_CUSTOM_COMMAND(TARGET ImageDecoderTest POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E make_directory
			"${CMAKE_CURRENT_BINARY_DIR}/ImageDecoder_data/${test_dir}"
		)
ENDFOREACH(test_dir ${ImageDecoderTest_dirs})

FILE(GLOB_RECURSE ImageDecoderTest_images RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/img/ImageDecoder_data"
	img/ImageDecoder_data/*.png
	img/ImageDecoder_data/*.gz
	)
FOREACH(test_image ${ImageDecoderTest_images})
	ADD_CUSTOM_COMMAND(TARGET ImageDecoderTest POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/img/ImageDecoder_data/${test_image}"
			"$<TARGET_FILE_DIR:ImageDecoderTest>/ImageDecoder_data/${test_image}"
		)
	ADD_CUSTOM_COMMAND(TARGET ImageDecoderTest POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/img/ImageDecoder_data/${test_image}"
			"${CMAKE_CURRENT_BINARY_DIR}/ImageDecoder_data/${test_image}"
		)
ENDFOREACH(test_image ${ImageDecoderTest_images})

# SuperMagicDrive test.
ADD_EXECUTABLE(SuperMagicDriveTest
	../../librpbase/tests/gtest_init.cpp
	utils/SuperMagicDriveTest.cpp
	utils/SuperMagicDriveTest_data.hpp
	)
TARGET_LINK_LIBRARIES(SuperMagicDriveTest romdata rpbase)
TARGET_LINK_LIBRARIES(SuperMagicDriveTest gtest)
TARGET_LINK_LIBRARIES(SuperMagicDriveTest ${ZLIB_LIBRARY})
TARGET_INCLUDE_DIRECTORIES(SuperMagicDriveTest PRIVATE ${ZLIB_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS(SuperMagicDriveTest PRIVATE ${ZLIB_DEFINITIONS})
DO_SPLIT_DEBUG(SuperMagicDriveTest)
SET_WINDOWS_SUBSYSTEM(SuperMagicDriveTest CONSOLE)
ADD_TEST(NAME SuperMagicDriveTest COMMAND SuperMagicDriveTest "--gtest_filter=-*benchmark*")
