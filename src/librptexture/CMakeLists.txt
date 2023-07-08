# rom-properties texture decoding library
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rptexture LANGUAGES CXX)

# OpenMP (TODO: AUTO/ON/OFF?)
OPTION(ENABLE_OPENMP "Enable OpenMP support if available." ON)
IF(ENABLE_OPENMP)
	FIND_PACKAGE(OpenMP)
	IF(OpenMP_FOUND)
		SET(HAVE_OPENMP 1)
		IF(NOT MSVC AND NOT OpenMP_CXX_LIB_NAMES)
			# Prior to CMake 3.9, FindOpenMP didn't set
			# library names. This breaks on Ubuntu 16.04,
			# which uses CMake 3.5.
			# NOTE: FIND_LIBRARY(gomp) doesn't work...
			FIND_PACKAGE(Threads REQUIRED)
			SET(OpenMP_CXX_LIB_NAMES -lgomp ${CMAKE_THREAD_LIBS_INIT})
		ENDIF(NOT MSVC AND NOT OpenMP_CXX_LIB_NAMES)
	ENDIF(OpenMP_FOUND)
ENDIF(ENABLE_OPENMP)

# Sources.
SET(${PROJECT_NAME}_SRCS
	FileFormatFactory.cpp
	ImageSizeCalc.cpp

	img/rp_image.cpp
	img/rp_image_backend.cpp
	img/rp_image_ops.cpp
	img/un-premultiply.cpp

	decoder/ImageDecoder_Linear.cpp
	decoder/ImageDecoder_GCN.cpp
	decoder/ImageDecoder_NDS.cpp
	decoder/ImageDecoder_N3DS.cpp
	decoder/ImageDecoder_S3TC.cpp
	decoder/ImageDecoder_DC.cpp
	decoder/ImageDecoder_ETC1.cpp
	decoder/ImageDecoder_BC7.cpp

	decoder/PixelConversion.cpp

	fileformat/FileFormat.cpp
	fileformat/ASTC.cpp
	fileformat/DidjTex.cpp
	fileformat/DirectDrawSurface.cpp
	fileformat/GodotSTEX.cpp
	fileformat/KhronosKTX.cpp
	fileformat/KhronosKTX2.cpp
	fileformat/PowerVR3.cpp
	fileformat/SegaPVR.cpp
	fileformat/TGA.cpp
	fileformat/ValveVTF.cpp
	fileformat/ValveVTF3.cpp
	fileformat/XboxXPR.cpp

	data/DX10Formats.cpp
	data/GLenumStrings.cpp
	data/VkEnumStrings.cpp
	)
# Headers.
SET(${PROJECT_NAME}_H
	FileFormatFactory.hpp
	argb32_t.hpp
	ImageSizeCalc.hpp

	img/rp_image.hpp
	img/rp_image_p.hpp
	img/rp_image_backend.hpp

	decoder/ImageDecoder_common.hpp
	decoder/ImageDecoder_p.hpp
	decoder/ImageDecoder_Linear.hpp
	decoder/ImageDecoder_GCN.hpp
	decoder/ImageDecoder_NDS.hpp
	decoder/ImageDecoder_N3DS.hpp
	decoder/ImageDecoder_S3TC.hpp
	decoder/ImageDecoder_DC.hpp
	decoder/ImageDecoder_ETC1.hpp
	decoder/ImageDecoder_BC7.hpp
	decoder/PixelConversion.hpp

	fileformat/FileFormat.hpp
	fileformat/FileFormat_p.hpp
	fileformat/FileFormat_decl.hpp
	fileformat/ASTC.hpp
	fileformat/DidjTex.hpp
	fileformat/DirectDrawSurface.hpp
	fileformat/GodotSTEX.hpp
	fileformat/KhronosKTX.hpp
	fileformat/KhronosKTX2.hpp
	fileformat/PowerVR3.hpp
	fileformat/SegaPVR.hpp
	fileformat/TGA.hpp
	fileformat/ValveVTF.hpp
	fileformat/ValveVTF3.hpp
	fileformat/XboxXPR.hpp

	fileformat/astc_structs.h
	fileformat/dds_structs.h
	fileformat/didj_tex_structs.h
	fileformat/godot_stex_structs.h
	fileformat/ktx_structs.h
	fileformat/ktx_structs.h
	fileformat/pvr_structs.h
	fileformat/pvr3_structs.h
	fileformat/vtf_structs.h
	fileformat/vtf3_structs.h
	fileformat/xbox_xpr_structs.h

	fileformat/gl_defs.h
	fileformat/vk_defs.h

	data/DX10Formats.hpp
	data/DX10Formats_data.h
	data/GLenumStrings.hpp
	data/VkEnumStrings.hpp
	data/vkEnum_base_data.h
	data/vkEnum_1000156xxx_data.h
	data/vkEnum_1000330xxx_data.h
	data/vkEnum_1000340xxx_data.h
	data/vkEnum_PVRTC_data.h
	data/vkEnum_ASTC_data.h
	)

IF(WIN32)
	SET(${PROJECT_NAME}_OS_SRCS
		img/GdiplusHelper.cpp
		img/RpGdiplusBackend.cpp
		)
	SET(${PROJECT_NAME}_OS_H
		img/GdiplusHelper.hpp
		img/RpGdiplusBackend.hpp
		)
ENDIF(WIN32)

IF(ENABLE_PVRTC)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} decoder/ImageDecoder_PVRTC.cpp)
	SET(${PROJECT_NAME}_H    ${${PROJECT_NAME}_H}    decoder/ImageDecoder_PVRTC.hpp)
ENDIF(ENABLE_PVRTC)
IF(ENABLE_ASTC)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} decoder/ImageDecoder_ASTC.cpp)
	SET(${PROJECT_NAME}_H    ${${PROJECT_NAME}_H}    decoder/ImageDecoder_ASTC.hpp)
ENDIF(ENABLE_ASTC)

# CPU-specific and optimized sources.
INCLUDE(CPUInstructionSetFlags)
IF(CPU_i386 OR CPU_amd64)
	# MSVC does not support MMX intrinsics in 64-bit builds.
	# Reference: https://docs.microsoft.com/en-us/cpp/cpp/m64?view=msvc-160
	#
	# amd64 CPUs support SSE2 as a minimum anyway, so there's
	# no point in building MMX code for 64-bit.
	SET(${PROJECT_NAME}_SSE2_SRCS
		img/rp_image_ops_sse2.cpp
		decoder/ImageDecoder_Linear_sse2.cpp
		)
	SET(${PROJECT_NAME}_SSSE3_SRCS
		img/rp_image_ops_ssse3.cpp
		decoder/ImageDecoder_Linear_ssse3.cpp
		)
	# TODO: Disable SSE 4.1 if not supported by the compiler?
	SET(${PROJECT_NAME}_SSE41_SRCS
		img/un-premultiply_sse41.cpp
		)

	# IFUNC functionality
	INCLUDE(CheckIfuncSupport)
	CHECK_IFUNC_SUPPORT()
	IF(HAVE_IFUNC)
		SET(${PROJECT_NAME}_IFUNC_SRCS decoder/ImageDecoder_ifunc.cpp)
	ENDIF(HAVE_IFUNC)

	IF(MMX_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${${PROJECT_NAME}_MMX_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${MMX_FLAG} ")
	ENDIF(MMX_FLAG)

	IF(SSE2_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${${PROJECT_NAME}_SSE2_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSE2_FLAG} ")
	ENDIF(SSE2_FLAG)

	IF(SSSE3_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${${PROJECT_NAME}_SSSE3_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
	ENDIF(SSSE3_FLAG)

	IF(SSE41_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${${PROJECT_NAME}_SSE41_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSE41_FLAG} ")
	ENDIF(SSE41_FLAG)
ENDIF()
UNSET(arch)

# Write the config.h files.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.lib${PROJECT_NAME}.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.lib${PROJECT_NAME}.h")

######################
# Build the library. #
######################

# NOTE: ${PROJECT_NAME} is the original target.
# ${PROJECT_NAME}-dll is the target with dllimport/dllexport for libromdata.
IF(RP_LIBROMDATA_IS_DLL AND WIN32)
	# On Win32, the dll target needs dllimport/dllexport.
	SET(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-dll)
ELSE()
	# On other platforms, the dll target will be an alias to the regular target.
	# That will be set up later, since the regular target doesn't exist yet.
	SET(TARGETS ${PROJECT_NAME})
ENDIF()

FOREACH(_target ${TARGETS})
	ADD_LIBRARY(${_target} STATIC
		${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_H}
		${${PROJECT_NAME}_OS_SRCS} ${${PROJECT_NAME}_OS_H}
		${${PROJECT_NAME}_IFUNC_SRCS}
		${${PROJECT_NAME}_MMX_SRCS}
		${${PROJECT_NAME}_SSE2_SRCS}
		${${PROJECT_NAME}_SSSE3_SRCS}
		${${PROJECT_NAME}_SSE41_SRCS}
		)
	IF(ENABLE_PCH)
		TARGET_PRECOMPILE_HEADERS(${_target} PRIVATE
			"${RP_CONFIG_LIBC_H}" "${RP_CXX11_COMPAT_H}"
			stdafx.h
			)
	ENDIF(ENABLE_PCH)
	INCLUDE(SetMSVCDebugPath)
	SET_MSVC_DEBUG_PATH(${_target})
	# Exclude from ALL builds.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES EXCLUDE_FROM_ALL TRUE)

	# DLL targets need the appropriate macros.
	IF(${_target} MATCHES "-dll$")
		SET(_target_suffix "-dll")
		TARGET_COMPILE_DEFINITIONS(${_target}
			PUBLIC		RP_BUILDING_FOR_DLL=1
			PRIVATE		RP_BUILDING_LIBROMDATA=1
			)
	ELSE()
		UNSET(_target_suffix)
	ENDIF()

	# TODO: More public/private library stuff.
	TARGET_LINK_LIBRARIES(${_target} PUBLIC rpcpu${_target_suffix} rpfile${_target_suffix} rptext${_target_suffix})
	TARGET_LINK_LIBRARIES(${_target} PRIVATE rpthreads${_target_suffix})

	# Include paths:
	# - Public: Current source and binary directories.
	# - Private: Parent source and binary directories,
	#            and top-level binary directory for git_version.h.
	TARGET_INCLUDE_DIRECTORIES(${_target}
		PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# librptexture
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# librptexture
		PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
		)
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE i18n${_target_suffix})
	ENDIF(ENABLE_NLS)
	IF(MSVC)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE delayimp)
	ENDIF(MSVC)
	IF(OpenMP_FOUND)
		TARGET_COMPILE_OPTIONS(${_target} PUBLIC ${OpenMP_CXX_FLAGS})
		TARGET_LINK_LIBRARIES(${_target} PUBLIC ${OpenMP_CXX_LIB_NAMES})
	ENDIF(OpenMP_FOUND)

	# zlib
	IF(ZLIB_FOUND)
		# NOTE: ZLIB must be PUBLIC in order for the Win32 UI frontend
		# to link properly.
		TARGET_LINK_LIBRARIES(${_target} PUBLIC ${ZLIB_LIBRARIES})
		TARGET_INCLUDE_DIRECTORIES(${_target} PUBLIC ${ZLIB_INCLUDE_DIRS})
		TARGET_COMPILE_DEFINITIONS(${_target} PUBLIC ${ZLIB_DEFINITIONS})
	ELSE(ZLIB_FOUND)
		MESSAGE(FATAL_ERROR "ZLIB_LIBRARIES has not been set by CheckZLIB.cmake.")
	ENDIF(ZLIB_FOUND)

	# PowerVR Native SDK
	IF(ENABLE_PVRTC)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE pvrtc)
	ENDIF(ENABLE_PVRTC)

	# ASTC decoder from Basis Universal
	IF(ENABLE_ASTC)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE basisu_astc)
	ENDIF(ENABLE_ASTC)

	# Other libraries.
	IF(WIN32)
		# libwin32common
		TARGET_LINK_LIBRARIES(${_target} PRIVATE win32common${_target_suffix})
		# Windows system libraries
		TARGET_LINK_LIBRARIES(${_target} PRIVATE gdiplus gdi32)
	ENDIF(WIN32)
ENDFOREACH(_target)
UNSET(_target_suffix)

IF(NOT TARGET ${PROJECT_NAME}-dll)
	# On other platforms, the dll target will be an alias to the regular target.
	ADD_LIBRARY(${PROJECT_NAME}-dll ALIAS ${PROJECT_NAME})
ENDIF()

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

# Test suite.
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
