# librptexture test suite
PROJECT(librptexture-tests LANGUAGES CXX)

# Top-level src directory.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# ImageDecoderLinear test
# TODO: Move to libromdata, or move libromdata stuff here?
ADD_EXECUTABLE(ImageDecoderLinearTest ImageDecoderLinearTest.cpp)
TARGET_LINK_LIBRARIES(ImageDecoderLinearTest PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(ImageDecoderLinearTest PRIVATE rpcpuid)	# for CPU dispatch
TARGET_COMPILE_DEFINITIONS(ImageDecoderLinearTest PRIVATE RP_BUILDING_FOR_DLL=1)
TARGET_LINK_LIBRARIES(ImageDecoderLinearTest PRIVATE ${ZLIB_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(ImageDecoderLinearTest PRIVATE ${ZLIB_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS(ImageDecoderLinearTest PRIVATE ${ZLIB_DEFINITIONS})
IF(PNG_LIBRARY)
	TARGET_LINK_LIBRARIES(ImageDecoderLinearTest PRIVATE ${PNG_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(ImageDecoderLinearTest PRIVATE ${PNG_INCLUDE_DIRS})
	TARGET_COMPILE_DEFINITIONS(ImageDecoderLinearTest PRIVATE ${PNG_DEFINITIONS})
ENDIF(PNG_LIBRARY)
# libfmt
IF(Fmt_FOUND)
	TARGET_LINK_LIBRARIES(ImageDecoderLinearTest PRIVATE ${Fmt_LIBRARY})
ENDIF(Fmt_FOUND)
DO_SPLIT_DEBUG(ImageDecoderLinearTest)
SET_WINDOWS_SUBSYSTEM(ImageDecoderLinearTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(ImageDecoderLinearTest wmain OFF)
ADD_TEST(NAME ImageDecoderLinearTest COMMAND ImageDecoderLinearTest --gtest_brief --gtest_filter=-*benchmark*)

# UnPremultiplyTest
ADD_EXECUTABLE(UnPremultiplyTest UnPremultiplyTest.cpp)
TARGET_LINK_LIBRARIES(UnPremultiplyTest PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(UnPremultiplyTest PRIVATE rpcpuid)	# for CPU dispatch
TARGET_COMPILE_DEFINITIONS(UnPremultiplyTest PRIVATE RP_BUILDING_FOR_DLL=1)
# libfmt
IF(Fmt_FOUND)
	TARGET_LINK_LIBRARIES(UnPremultiplyTest PRIVATE ${Fmt_LIBRARY})
ENDIF(Fmt_FOUND)
DO_SPLIT_DEBUG(UnPremultiplyTest)
SET_WINDOWS_SUBSYSTEM(UnPremultiplyTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(UnPremultiplyTest wmain OFF)
ADD_TEST(NAME UnPremultiplyTest COMMAND UnPremultiplyTest --gtest_brief --gtest_filter=-*benchmark*)
