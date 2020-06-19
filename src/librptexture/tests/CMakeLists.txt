# librptexture test suite
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(librptexture-tests LANGUAGES CXX)

IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# ImageDecoderLinear test
# TODO: Move to libromdata, or move libromdata stuff here?
ADD_EXECUTABLE(ImageDecoderLinearTest ImageDecoderLinearTest.cpp)
TARGET_LINK_LIBRARIES(ImageDecoderLinearTest PRIVATE rptest rpcpu romdata rptexture)
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
SET_WINDOWS_ENTRYPOINT(ImageDecoderLinearTest wmain OFF)
ADD_TEST(NAME ImageDecoderLinearTest COMMAND ImageDecoderLinearTest "--gtest_filter=-*benchmark*")

# UnPremultiplyTest
ADD_EXECUTABLE(UnPremultiplyTest UnPremultiplyTest.cpp)
TARGET_LINK_LIBRARIES(UnPremultiplyTest PRIVATE rptest rpcpu rptexture)
TARGET_LINK_LIBRARIES(UnPremultiplyTest PRIVATE gtest)
DO_SPLIT_DEBUG(UnPremultiplyTest)
SET_WINDOWS_SUBSYSTEM(UnPremultiplyTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(UnPremultiplyTest wmain OFF)
ADD_TEST(NAME UnPremultiplyTest COMMAND UnPremultiplyTest "--gtest_filter=-*benchmark*")
