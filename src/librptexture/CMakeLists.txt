PROJECT(librptexture)

# Sources.
SET(librptexture_SRCS
	FileFormatFactory.cpp

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
	fileformat/DidjTex.cpp
	fileformat/DirectDrawSurface.cpp
	fileformat/SegaPVR.cpp
	fileformat/ValveVTF.cpp
	fileformat/ValveVTF3.cpp
	fileformat/XboxXPR.cpp

	data/DX10Formats.cpp
	)
# Headers.
SET(librptexture_H
	FileFormatFactory.hpp

	img/rp_image.hpp
	img/rp_image_p.hpp
	img/rp_image_backend.hpp

	decoder/ImageDecoder.hpp
	decoder/ImageDecoder_p.hpp
	decoder/PixelConversion.hpp

	fileformat/FileFormat.hpp
	fileformat/FileFormat_p.hpp
	fileformat/FileFormat_decl.hpp
	fileformat/DidjTex.hpp
	fileformat/DirectDrawSurface.hpp
	fileformat/SegaPVR.hpp
	fileformat/ValveVTF.hpp
	fileformat/ValveVTF3.hpp
	fileformat/XboxXPR.hpp

	fileformat/dds_structs.h
	fileformat/didj_tex_structs.h
	fileformat/pvr_structs.h
	fileformat/vtf_structs.h
	fileformat/vtf3_structs.h
	fileformat/xbox_xpr_structs.h

	data/DX10Formats.hpp
	)

IF(ENABLE_GL)
	SET(librptexture_GL_SRCS
		fileformat/KhronosKTX.cpp
		data/GLenumStrings.cpp
		)
	SET(librptexture_GL_H
		fileformat/KhronosKTX.hpp
		fileformat/ktx_structs.h
		fileformat/gl_defs.h
		data/GLenumStrings.hpp
		)
ENDIF(ENABLE_GL)

IF(WIN32)
	SET(librptexture_OS_SRCS
		img/GdiplusHelper.cpp
		img/RpGdiplusBackend.cpp
		)
	SET(librptexture_OS_H
		img/GdiplusHelper.hpp
		img/RpGdiplusBackend.hpp
		)
ENDIF(WIN32)

# CPU-specific and optimized sources.
IF(CPU_i386 OR CPU_amd64)
	# MSVC does not support MMX intrinsics in 64-bit builds.
	# Reference: https://msdn.microsoft.com/en-us/library/08x3t697(v=vs.110).aspx
	#
	# amd64 CPUs support SSE2 as a minimum anyway, so there's
	# no point in building MMX code for 64-bit.
	SET(librptexture_SSE2_SRCS
		img/rp_image_ops_sse2.cpp
		decoder/ImageDecoder_Linear_sse2.cpp
		)
	SET(librptexture_SSSE3_SRCS
		decoder/ImageDecoder_Linear_ssse3.cpp
		)
	# TODO: Disable SSE 4.1 if not supported by the compiler?
	SET(librptexture_SSE41_SRCS
		img/un-premultiply_sse41.cpp
		)

	# IFUNC requires glibc.
	# We're not checking for glibc here, but we do have preprocessor
	# checks, so even if this does get compiled on a non-glibc system,
	# it won't do anything.
	# TODO: Might be supported on other Unix-like operating systems...
	IF(UNIX AND NOT APPLE)
		SET(librptexture_IFUNC_SRCS decoder/ImageDecoder_ifunc.cpp)
		# Disable LTO on the IFUNC files if LTO is known to be broken.
		IF(GCC_5xx_LTO_ISSUES)
			SET_SOURCE_FILES_PROPERTIES(${librptexture_IFUNC_SRCS}
				APPEND_STRING PROPERTIES COMPILE_FLAGS " -fno-lto ")
		ENDIF(GCC_5xx_LTO_ISSUES)
	ENDIF(UNIX AND NOT APPLE)

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSE2_FLAG "/arch:SSE2")
		SET(SSSE3_FLAG "/arch:SSE2")
		SET(SSE41_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		SET(MMX_FLAG "-mmmx")
		SET(SSE2_FLAG "-msse2")
		SET(SSSE3_FLAG "-mssse3")
		SET(SSE41_FLAG "-msse4.1")
	ENDIF()

	IF(MMX_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${librptexture_MMX_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${MMX_FLAG} ")
	ENDIF(MMX_FLAG)

	IF(SSE2_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${librptexture_SSE2_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSE2_FLAG} ")
	ENDIF(SSE2_FLAG)

	IF(SSSE3_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${librptexture_SSSE3_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
	ENDIF(SSSE3_FLAG)

	IF(SSE41_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${librptexture_SSE41_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSE41_FLAG} ")
	ENDIF(SSE41_FLAG)
ENDIF()
UNSET(arch)

# Write the config.h files.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.librptexture.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.librptexture.h")

######################
# Build the library. #
######################

ADD_LIBRARY(rptexture STATIC
	${librptexture_SRCS} ${librptexture_H}
	${librptexture_OS_SRCS} ${librptexture_OS_H}
	${librptexture_GL_SRCS} ${librptexture_GL_H}
	${librptexture_IFUNC_SRCS}
	${librptexture_MMX_SRCS}
	${librptexture_SSE2_SRCS}
	${librptexture_SSSE3_SRCS}
	${librptexture_SSE41_SRCS}
	)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(rptexture)

# TODO: More public/private library stuff.
TARGET_LINK_LIBRARIES(rptexture PUBLIC rpbase)
TARGET_LINK_LIBRARIES(rptexture PRIVATE rpthreads)

# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(rptexture
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# librptexture
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# librptexture
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)
IF(MSVC)
	TARGET_LINK_LIBRARIES(rptexture PRIVATE delayimp)
ENDIF(MSVC)
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(rptexture PROPERTIES EXCLUDE_FROM_ALL TRUE)

# zlib
IF(NOT ZLIB_LIBRARY)
	MESSAGE(FATAL_ERROR "ZLIB_LIBRARY has not been set by CheckZLIB.cmake.")
ENDIF(NOT ZLIB_LIBRARY)
TARGET_LINK_LIBRARIES(rptexture PRIVATE ${ZLIB_LIBRARY})

# Other libraries.
IF(WIN32)
	# libwin32common
	TARGET_LINK_LIBRARIES(rptexture PRIVATE win32common)
	# Windows system libraries
	TARGET_LINK_LIBRARIES(rptexture PRIVATE gdiplus)
ENDIF(WIN32)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

# Test suite.
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
