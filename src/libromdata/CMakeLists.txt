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

	# Check for zlib.
	FIND_PACKAGE(ZLIB REQUIRED)
	SET(HAVE_ZLIB ${ZLIB_FOUND})
	SET(ZLIB_DEFINITIONS ZLIB_CONST)

	# Check for libpng.
	FIND_PACKAGE(PNG REQUIRED)
	SET(HAVE_LIBPNG ${PNG_FOUND})

	IF(ENABLE_DECRYPTION)
		# TODO: Show warning if not found, and recommend
		# setting -DENABLE_DECRYPTION=Off?
		FIND_PACKAGE(Nettle REQUIRED)
		SET(HAVE_NETTLE ${NETTLE_FOUND})

		# Check if this is Nettle 3.x.
		# Nettle 3.1 added version.h, which isn't available
		# in older verisons, so we can't simply check that.
		INCLUDE(CheckSymbolExists)
		CHECK_SYMBOL_EXISTS(aes128_set_decrypt_key "nettle/aes.h" HAVE_NETTLE_3)
	ENDIF(ENABLE_DECRYPTION)
ENDIF(NOT WIN32)

# Check for C library functions.
INCLUDE(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(strnlen HAVE_STRNLEN)

# Check if non-capturing lambda functions can be cast to function pointers.
INCLUDE(CheckCXX11LambdaAsFunctionPointer)
CHECK_CXX11_LAMBDA_AS_FUNCTION_POINTER(HAVE_LAMBDA_AS_FUNCTION_POINTER)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.libromdata.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.libromdata.h")

SET(libromdata_SRCS
	byteswap.c
	TextFuncs.cpp
	RomData.cpp
	RomFields.cpp
	MegaDrive.cpp
	MegaDrivePublishers.cpp
	MegaDriveRegions.cpp
	GameCube.cpp
	NintendoDS.cpp
	NintendoPublishers.cpp
	RomDataFactory.cpp
	DMG.cpp
	VirtualBoy.cpp
	GameBoyAdvance.cpp
	SystemRegion.cpp
	GameCubeSave.cpp
	N64.cpp
	SNES.cpp
	DreamcastSave.cpp
	PlayStationSave.cpp
	file/IRpFile.cpp
	file/RpMemFile.cpp
	file/FileSystem.cpp
	img/rp_image.cpp
	img/rp_image_backend.cpp
	img/RpImageLoader.cpp
	img/ImageDecoder.cpp
	img/IconAnimHelper.cpp
	disc/DiscReader.cpp
	disc/WbfsReader.cpp
	disc/CisoGcnReader.cpp
	disc/GcnPartition.cpp
	disc/GcnPartitionPrivate.cpp
	disc/WiiPartition.cpp
	disc/GcnFst.cpp
	disc/PartitionFile.cpp
	img/pngcheck/pngcheck.cpp
	)
SET(libromdata_H
	byteorder.h
	byteswap.h
	common.h
	RomData.hpp
	RomFields.hpp
	TextFuncs.hpp
	TextFuncs_int.hpp
	MegaDrive.hpp
	MegaDrivePublishers.hpp
	MegaDriveRegions.hpp
	GameCube.hpp
	NintendoDS.hpp
	NintendoPublishers.hpp
	RomDataFactory.hpp
	gcn_structs.h
	DMG.hpp
	VirtualBoy.hpp
	uvector.h
	GameBoyAdvance.hpp
	CopierFormats.h
	SystemRegion.hpp
	GameCubeSave.hpp
	PlayStationSave.hpp
	gcn_card.h
	nds_structs.h
	N64.hpp
	n64_structs.h
	dmg_structs.h
	gba_structs.h
	vb_structs.h
	SNES.hpp
	snes_structs.h
	DreamcastSave.hpp
	dc_structs.h
	file/IRpFile.hpp
	file/RpFile.hpp
	file/RpMemFile.hpp
	file/FileSystem.hpp
	img/rp_image.hpp
	img/rp_image_backend.hpp
	img/RpImageLoader.hpp
	img/ImageDecoder.hpp
	img/RpPng.hpp
	img/IconAnimData.hpp
	img/IconAnimHelper.hpp
	disc/IDiscReader.hpp
	disc/DiscReader.hpp
	disc/WbfsReader.hpp
	disc/libwbfs.h
	disc/CisoGcnReader.hpp
	disc/ciso_gcn.h
	disc/IPartition.hpp
	disc/GcnPartition.hpp
	disc/GcnPartitionPrivate.hpp
	disc/WiiPartition.hpp
	disc/IFst.hpp
	disc/GcnFst.hpp
	disc/PartitionFile.hpp
	)

IF(WIN32)
	SET(libromdata_OS_SRCS
		RpWin32.hpp
		RpWin32_sdk.h
		TextFuncs_win32.cpp
		file/win32/RpFile_win32.cpp
		file/win32/IStreamWrapper.cpp
		img/RpPng_gdiplus.cpp
		img/GdiplusHelper.cpp
		img/RpGdiplusBackend.cpp
		)
	SET(libromdata_OS_H
		file/win32/IStreamWrapper.hpp
		img/GdiplusHelper.hpp
		img/RpGdiplusBackend.hpp
		)
ELSE(WIN32)
	SET(libromdata_OS_SRCS
		TextFuncs_iconv.cpp
		file/RpFile_stdio.cpp
		img/RpPng_libpng.cpp
		)
ENDIF(WIN32)

IF(ENABLE_DECRYPTION)
	SET(libromdata_CRYPTO_SRCS
		crypto/AesCipherFactory.cpp
		crypto/KeyManager.cpp
		)
	SET(libromdata_CRYPTO_H
		crypto/IAesCipher.hpp
		crypto/KeyManager.hpp
		)
	IF(WIN32)
		SET(libromdata_CRYPTO_OS_SRCS
			crypto/AesCAPI.cpp
			crypto/AesCAPI_NG.cpp
			)
		SET(libromdata_CRYPTO_OS_H
			crypto/AesCAPI.hpp
			crypto/AesCAPI_NG.hpp
			)
	ELSE(WIN32)
		SET(libromdata_CRYPTO_OS_SRCS
			crypto/AesNettle.cpp
			)
		SET(libromdata_CRYPTO_OS_H
			crypto/AesNettle.hpp
			)
	ENDIF(WIN32)
ENDIF(ENABLE_DECRYPTION)
IF(HAVE_ZLIB)
	# Enable zlib in pngcheck.
	SET_SOURCE_FILES_PROPERTIES(img/pngcheck/pngcheck.cpp
		PROPERTIES COMPILE_DEFINITIONS USE_ZLIB)
ENDIF(HAVE_ZLIB)
IF(CMAKE_COMPILER_IS_GNUCXX)
	# Disable some warnings for pngcheck.
	SET_SOURCE_FILES_PROPERTIES(img/pngcheck/pngcheck.cpp
		PROPERTIES COMPILE_FLAGS "-Wno-unused")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

######################
# Build the library. #
######################

# UTF-8 version.
ADD_LIBRARY(romdata8 STATIC
	${libromdata_SRCS} ${libromdata_H}
	${libromdata_OS_SRCS} ${libromdata_OS_H}
	${libromdata_CRYPTO_SRCS} ${libromdata_CRYPTO_H}
	${libromdata_CRYPTO_OS_SRCS} ${libromdata_CRYPTO_OS_H}
	)
TARGET_COMPILE_DEFINITIONS(romdata8 PUBLIC -DRP_UTF8)

# UTF-16 version.
ADD_LIBRARY(romdata16 STATIC
	${libromdata_SRCS} ${libromdata_H}
	${libromdata_OS_SRCS} ${libromdata_OS_H}
	${libromdata_CRYPTO_SRCS} ${libromdata_CRYPTO_H}
	${libromdata_CRYPTO_OS_SRCS}
	)
TARGET_COMPILE_DEFINITIONS(romdata16 PUBLIC -DRP_UTF16)

# Common properties for romdata8 and romdata16.
FOREACH(_target romdata8 romdata16)
	# Include paths:
	# - Public: Current source and binary directories.
	# - Private: Parent source and binary directories,
	#            and top-level binary directory for git_version.h.
	TARGET_INCLUDE_DIRECTORIES(${_target}
		PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
		)
	# Exclude from ALL builds.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES
		EXCLUDE_FROM_ALL TRUE
		)
	IF(ICONV_LIBRARY)
		TARGET_LINK_LIBRARIES(${_target} ${ICONV_LIBRARY})
	ENDIF(ICONV_LIBRARY)
	IF(ZLIB_FOUND)
		TARGET_LINK_LIBRARIES(${_target} ${ZLIB_LIBRARIES})
		TARGET_INCLUDE_DIRECTORIES(${_target} PRIVATE ${ZLIB_INCLUDE_DIRS})
		TARGET_COMPILE_DEFINITIONS(${_target} PRIVATE ${ZLIB_DEFINITIONS})
	ENDIF(ZLIB_FOUND)
	IF(PNG_FOUND)
		TARGET_LINK_LIBRARIES(${_target} ${PNG_LIBRARY})
		TARGET_INCLUDE_DIRECTORIES(${_target} PRIVATE ${PNG_INCLUDE_DIRS})
		TARGET_COMPILE_DEFINITIONS(${_target} PRIVATE ${PNG_DEFINITIONS})
	ENDIF(PNG_FOUND)
	IF(NETTLE_FOUND)
		TARGET_LINK_LIBRARIES(${_target} ${NETTLE_LIBRARY})
		TARGET_INCLUDE_DIRECTORIES(${_target} PRIVATE ${NETTLE_INCLUDE_DIRS})
	ENDIF(NETTLE_FOUND)
	IF(WIN32)
		TARGET_LINK_LIBRARIES(${_target} gdiplus)
		IF(ENABLE_DECRYPTION)
			TARGET_LINK_LIBRARIES(${_target} advapi32)
		ENDIF(ENABLE_DECRYPTION)
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
