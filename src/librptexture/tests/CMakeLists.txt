PROJECT(librptexture-tests)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

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
