# libromdata test suite
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(libromdata-tests LANGUAGES CXX)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

IF(ENABLE_DECRYPTION)
	# CtrKeyScrambler test.
	ADD_EXECUTABLE(CtrKeyScramblerTest CtrKeyScramblerTest.cpp)
	TARGET_LINK_LIBRARIES(CtrKeyScramblerTest PRIVATE rptest romdata)
	TARGET_LINK_LIBRARIES(CtrKeyScramblerTest PRIVATE gtest)
	DO_SPLIT_DEBUG(CtrKeyScramblerTest)
	SET_WINDOWS_SUBSYSTEM(CtrKeyScramblerTest CONSOLE)
	SET_WINDOWS_ENTRYPOINT(CtrKeyScramblerTest wmain OFF)
	ADD_TEST(NAME CtrKeyScramblerTest COMMAND CtrKeyScramblerTest)
ENDIF(ENABLE_DECRYPTION)

# GcnFstPrint. (Not a test, but a useful program.)
ADD_EXECUTABLE(GcnFstPrint
	disc/FstPrint.cpp
	disc/FstPrint.hpp
	disc/GcnFstPrint.cpp
	)
TARGET_LINK_LIBRARIES(GcnFstPrint PRIVATE rpsecure romdata)
IF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(GcnFstPrint PRIVATE i18n)
ENDIF(ENABLE_NLS)
IF(WIN32)
	TARGET_LINK_LIBRARIES(GcnFstPrint PRIVATE wmain)
ENDIF(WIN32)
DO_SPLIT_DEBUG(GcnFstPrint)
SET_WINDOWS_SUBSYSTEM(GcnFstPrint CONSOLE)
SET_WINDOWS_ENTRYPOINT(GcnFstPrint wmain OFF)

# GcnFstTest.
# NOTE: We can't disable NLS here due to its usage
# in FstPrint.cpp. gtest_init.cpp will set LC_ALL=C.
ADD_EXECUTABLE(GcnFstTest
	disc/FstPrint.cpp
	disc/FstPrint.hpp
	disc/GcnFstTest.cpp
	)
TARGET_LINK_LIBRARIES(GcnFstTest PRIVATE rptest romdata)
IF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(GcnFstTest PRIVATE i18n)
ENDIF(ENABLE_NLS)
TARGET_LINK_LIBRARIES(GcnFstTest PRIVATE gtest)
TARGET_LINK_LIBRARIES(GcnFstTest PRIVATE minizip ${ZLIB_LIBRARIES})
DO_SPLIT_DEBUG(GcnFstTest)
SET_WINDOWS_SUBSYSTEM(GcnFstTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(GcnFstTest wmain OFF)
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
ADD_EXECUTABLE(ImageDecoderTest img/ImageDecoderTest.cpp)
TARGET_LINK_LIBRARIES(ImageDecoderTest PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(ImageDecoderTest PRIVATE gtest ${ZLIB_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(ImageDecoderTest PRIVATE ${ZLIB_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS(ImageDecoderTest PRIVATE ${ZLIB_DEFINITIONS})
IF(PNG_LIBRARY)
	TARGET_LINK_LIBRARIES(ImageDecoderTest PRIVATE ${PNG_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(ImageDecoderTest PRIVATE ${PNG_INCLUDE_DIRS})
	TARGET_COMPILE_DEFINITIONS(ImageDecoderTest PRIVATE ${PNG_DEFINITIONS})
ENDIF(PNG_LIBRARY)
DO_SPLIT_DEBUG(ImageDecoderTest)
SET_WINDOWS_SUBSYSTEM(ImageDecoderTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(ImageDecoderTest wmain OFF)
ADD_TEST(NAME ImageDecoderTest COMMAND ImageDecoderTest "--gtest_filter=-*Benchmark*")

# Copy the reference images to:
# - bin/ImageDecoder_data/ (TODO: Subdirectory?)
# - ${CMAKE_CURRENT_BINARY_DIR}/ImageDecoder_data/
# NOTE: Although the test executable is in bin/, CTest still
# uses ${CMAKE_CURRENT_BINARY_DIR} as the working directory.
# Hence, we have to copy the files to both places.
FOREACH(test_dir Alpha ARGB Luma RGB S3TC Misc BC7 PVR GVR KTX KTX2 VTF VTF3
	tctest DidjTex PowerVR3 TGA TGA/conformance TGA/tga-go STEX3 STEX4 XPR)

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
	img/ImageDecoder_data/*.dds
	img/ImageDecoder_data/*.tex
	img/ImageDecoder_data/*.VMS

	# Godot textures with embedded PNG and/or WebP images
	img/ImageDecoder_data/*.ctex
	img/ImageDecoder_data/*.stex
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

# Nintendo System ID test.
ADD_EXECUTABLE(NintendoSystemIDTest NintendoSystemIDTest.cpp)
TARGET_LINK_LIBRARIES(NintendoSystemIDTest PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(NintendoSystemIDTest PRIVATE gtest)
DO_SPLIT_DEBUG(NintendoSystemIDTest)
SET_WINDOWS_SUBSYSTEM(NintendoSystemIDTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(NintendoSystemIDTest wmain OFF)
ADD_TEST(NAME NintendoSystemIDTest COMMAND NintendoSystemIDTest)

# SuperMagicDrive test.
ADD_EXECUTABLE(SuperMagicDriveTest
	utils/SuperMagicDriveTest.cpp
	utils/SuperMagicDriveTest_data.hpp
	)
TARGET_LINK_LIBRARIES(SuperMagicDriveTest PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(SuperMagicDriveTest PRIVATE gtest)
TARGET_LINK_LIBRARIES(SuperMagicDriveTest PRIVATE ${ZLIB_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(SuperMagicDriveTest PRIVATE ${ZLIB_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS(SuperMagicDriveTest PRIVATE ${ZLIB_DEFINITIONS})
DO_SPLIT_DEBUG(SuperMagicDriveTest)
SET_WINDOWS_SUBSYSTEM(SuperMagicDriveTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(SuperMagicDriveTest wmain OFF)
ADD_TEST(NAME SuperMagicDriveTest COMMAND SuperMagicDriveTest "--gtest_filter=-*benchmark*")
