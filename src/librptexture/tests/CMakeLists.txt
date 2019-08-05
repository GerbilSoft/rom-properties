PROJECT(librptexture-tests)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# ZLIB and libpng are checked in the top-level CMakeLists.txt.

# RpImageLoader test
ADD_EXECUTABLE(RpImageLoaderTest
	../../librpbase/tests/gtest_init.cpp
	RpImageLoaderTest.cpp
	RpPngFormatTest.cpp
	)
IF(WIN32)
	TARGET_LINK_LIBRARIES(RpImageLoaderTest PRIVATE win32common)
ENDIF(WIN32)
TARGET_LINK_LIBRARIES(RpImageLoaderTest PRIVATE rptexture rpbase)
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
ADD_TEST(NAME RpImageLoaderTest COMMAND RpImageLoaderTest)

# Copy the reference images to:
# - bin/png_data/ (TODO: Subdirectory?)
# - ${CMAKE_CURRENT_BINARY_DIR}/png_data/
# NOTE: Although the test executable is in bin/, CTest still
# uses ${CMAKE_CURRENT_BINARY_DIR} as the working directory.
# Hence, we have to copy the files to both places.
FILE(GLOB RpImageLoaderTest_images RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/png_data" png_data/*.png png_data/*.bmp.gz)
FOREACH(test_image ${RpImageLoaderTest_images})
	ADD_CUSTOM_COMMAND(TARGET RpImageLoaderTest POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/png_data/${test_image}"
			"$<TARGET_FILE_DIR:RpImageLoaderTest>/png_data/${test_image}"
		)
	ADD_CUSTOM_COMMAND(TARGET RpImageLoaderTest POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/png_data/${test_image}"
			"${CMAKE_CURRENT_BINARY_DIR}/png_data/${test_image}"
		)
ENDFOREACH(test_image ${RpImageLoaderTest_images})

# ImageDecoderLinear test
# TODO: Move to libromdata, or move libromdata stuff here?
ADD_EXECUTABLE(ImageDecoderLinearTest
	../../librpbase/tests/gtest_init.cpp
	ImageDecoderLinearTest.cpp
	)
IF(WIN32)
	TARGET_LINK_LIBRARIES(ImageDecoderLinearTest PRIVATE win32common)
ENDIF(WIN32)
TARGET_LINK_LIBRARIES(ImageDecoderLinearTest PRIVATE romdata rptexture)
TARGET_LINK_LIBRARIES(ImageDecoderLinearTest PRIVATE gtest ${ZLIB_LIBRARY})
TARGET_INCLUDE_DIRECTORIES(ImageDecoderLinearTest PRIVATE ${ZLIB_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS(ImageDecoderLinearTest PRIVATE ${ZLIB_DEFINITIONS})
IF(PNG_LIBRARY)
	TARGET_LINK_LIBRARIES(ImageDecoderLinearTest PRIVATE ${PNG_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(ImageDecoderLinearTest PRIVATE ${PNG_INCLUDE_DIRS})
	TARGET_COMPILE_DEFINITIONS(ImageDecoderLinearTest PRIVATE ${PNG_DEFINITIONS})
ENDIF(PNG_LIBRARY)
DO_SPLIT_DEBUG(ImageDecoderLinearTest)
SET_WINDOWS_SUBSYSTEM(ImageDecoderLinearTest CONSOLE)
ADD_TEST(NAME ImageDecoderLinearTest COMMAND ImageDecoderLinearTest "--gtest_filter=-*benchmark*")

# UnPremultiplyTest
ADD_EXECUTABLE(UnPremultiplyTest
	../../librpbase/tests/gtest_init.cpp
	UnPremultiplyTest.cpp
	)
IF(WIN32)
	TARGET_LINK_LIBRARIES(UnPremultiplyTest PRIVATE win32common)
ENDIF(WIN32)
TARGET_LINK_LIBRARIES(UnPremultiplyTest PRIVATE rptexture rpbase)
TARGET_LINK_LIBRARIES(UnPremultiplyTest PRIVATE gtest)
DO_SPLIT_DEBUG(UnPremultiplyTest)
SET_WINDOWS_SUBSYSTEM(UnPremultiplyTest CONSOLE)
ADD_TEST(NAME UnPremultiplyTest COMMAND UnPremultiplyTest "--gtest_filter=-*benchmark*")
