PROJECT(libromdata)

IF(NOT WIN32)
	# Non-Windows library checks.

	# On Win32, we use MultiByteToWideChar() and WideCharToMultiByte().
	# On other systems, we use iconv(), which could be in libc or libiconv.
	# Figure out which library has iconv().

	# First, check libc for iconv().
	INCLUDE(CheckLibraryExists)
	CHECK_LIBRARY_EXISTS(c iconv "" HAVE_ICONV_C)	# libc: iconv()
	IF(HAVE_ICONV_C)
		# iconv() found in libc.
		UNSET(ICONV_LIBRARY)
		SET(HAVE_ICONV 1)
	ELSE(HAVE_ICONV_C)
		# iconv() not found in libc.
		# Check in libiconv.
		CHECK_LIBRARY_EXISTS(iconv iconv "" HAVE_ICONV_LIBICONV)	# libiconv: iconv()
		IF(HAVE_ICONV_LIBICONV)
			# iconv() found in libiconv.
			SET(ICONV_LIBRARY "iconv")
			SET(HAVE_ICONV 1)
		ENDIF(HAVE_ICONV_LIBICONV)
	ENDIF(HAVE_ICONV_C)

	IF(NOT HAVE_ICONV)
		MESSAGE(FATAL_ERROR "iconv() not found, cannot continue.")
	ENDIF(NOT HAVE_ICONV)

	# Check for libpng.
	FIND_PACKAGE(PNG REQUIRED)
	SET(HAVE_LIBPNG ${PNG_FOUND})
ENDIF(NOT WIN32)

# Check for C library functions.
INCLUDE(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(strnlen HAVE_STRNLEN)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.libromdata.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.libromdata.h")

SET(libromdata_SRCS
	TextFuncs.cpp
	RomData.cpp
	RomFields.cpp
	MegaDrive.cpp
	MegaDrivePublishers.cpp
	GameCube.cpp
	NintendoDS.cpp
	NintendoPublishers.cpp
	RomDataFactory.cpp
	IDiscReader.cpp
	DiscReader.cpp
	WbfsReader.cpp
	DMG.cpp
	GameBoyAdvance.cpp
	file/RpMemFile.cpp
	file/FileSystem.cpp
	img/rp_image.cpp
	img/RpImageLoader.cpp
	)
SET(libromdata_H
	byteorder.h
	byteswap.h
	common.h
	RomData.hpp
	RomFields.hpp
	TextFuncs.hpp
	MegaDrive.hpp
	MegaDrivePublishers.hpp
	GameCube.hpp
	NintendoDS.hpp
	NintendoPublishers.hpp
	RomDataFactory.hpp
	IDiscReader.hpp
	DiscReader.hpp
	WbfsReader.hpp
	libwbfs.h
	gcn_structs.h
	DMG.hpp
	uvector.h
	GameBoyAdvance.hpp
	file/IRpFile.hpp
	file/RpFile.hpp
	file/RpMemFile.hpp
	file/FileSystem.hpp
	img/rp_image.hpp
	img/RpImageLoader.hpp
	img/RpPng.hpp
	)

IF(WIN32)
	SET(libromdata_OS_SRCS
		file/RpFile_Win32.cpp
		file/RP_IStream_Win32.cpp
		img/RpPng_gdiplus.cpp
		img/GdiplusHelper.cpp
		)
	SET(libromdata_OS_H
		file/RP_IStream_Win32.hpp
		img/GdiplusHelper.hpp
		)
ELSE(WIN32)
	SET(libromdata_OS_SRCS
		file/RpFile_stdio.cpp
		img/RpPng_libpng.cpp
		)
ENDIF(WIN32)

IF(ENABLE_DECRYPTION)
	SET(libromdata_CRYPTO_SRCS
		crypto/KeyManager.cpp
		)
	SET(libromdata_CRYPTO_H
		crypto/KeyManager.hpp
		)
ENDIF(ENABLE_DECRYPTION)

######################
# Build the library. #
######################

# UTF-8 version.
ADD_LIBRARY(romdata8 STATIC
	${libromdata_SRCS} ${libromdata_H}
	${libromdata_OS_SRCS} ${libromdata_OS_H}
	${libromdata_CRYPTO_SRCS} ${libromdata_CRYPTO_H}
	)
TARGET_COMPILE_DEFINITIONS(romdata8 PUBLIC -DRP_UTF8)

# UTF-16 version.
ADD_LIBRARY(romdata16 STATIC
	${libromdata_SRCS} ${libromdata_H}
	${libromdata_OS_SRCS} ${libromdata_OS_H}
	${libromdata_CRYPTO_SRCS} ${libromdata_CRYPTO_H}
	)
TARGET_COMPILE_DEFINITIONS(romdata16 PUBLIC -DRP_UTF16)

# Common properties for romdata8 and romdata16.
FOREACH(_target romdata8 romdata16)
	TARGET_INCLUDE_DIRECTORIES(${_target}
		PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
		)
	SET_TARGET_PROPERTIES(${_target} PROPERTIES
		EXCLUDE_FROM_ALL TRUE
		EXCLUDE_FROM_DEFAULT_BUILD TRUE
		)
	IF(ICONV_LIBRARY)
		TARGET_LINK_LIBRARIES(${_target} ${ICONV_LIBRARY})
	ENDIF(ICONV_LIBRARY)
	IF(PNG_FOUND)
		TARGET_LINK_LIBRARIES(${_target} ${PNG_LIBRARY})
		TARGET_INCLUDE_DIRECTORIES(${_target} PRIVATE ${PNG_INCLUDE_DIRS})
		TARGET_COMPILE_DEFINITIONS(${_target} PRIVATE ${PNG_DEFINITIONS})
	ENDIF(PNG_FOUND)
	IF(WIN32)
		TARGET_LINK_LIBRARIES(${_target} gdiplus)
	ENDIF(WIN32)
ENDFOREACH()

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

# Test suite.
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
