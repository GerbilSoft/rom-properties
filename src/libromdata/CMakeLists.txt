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

# ZLIB and libpng are checked in the top-level CMakeLists.txt.

# Check for C library functions.
INCLUDE(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(strnlen "string.h" HAVE_STRNLEN)

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
	MegaDriveRegions.cpp
	GameCube.cpp
	NintendoDS.cpp
	RomDataFactory.cpp
	DMG.cpp
	GameBoyAdvance.cpp
	SystemRegion.cpp
	GameCubeSave.cpp
	N64.cpp
	SNES.cpp
	DreamcastSave.cpp
	PlayStationSave.cpp
	VirtualBoy.cpp
	Amiibo.cpp
	NES.cpp
	EXE.cpp
	WiiU.cpp
	Nintendo3DS.cpp
	data/MegaDrivePublishers.cpp
	data/NintendoPublishers.cpp
	data/AmiiboData.cpp
	data/WiiSystemMenuVersion.cpp
	data/NESMappers.cpp
	data/EXEData.cpp
	data/WiiUData.cpp
	file/IRpFile.cpp
	file/RpMemFile.cpp
	file/FileSystem_common.cpp
	img/rp_image.cpp
	img/rp_image_backend.cpp
	img/rp_image_ops.cpp
	img/RpImageLoader.cpp
	img/ImageDecoder.cpp
	img/RpPng.cpp
	img/IconAnimHelper.cpp
	#img/TCreateThumbnail.cpp	# NOT listed here due to template stuff.
	img/pngcheck/pngcheck.cpp
	disc/IDiscReader.cpp
	disc/DiscReader.cpp
	disc/WbfsReader.cpp
	disc/CisoGcnReader.cpp
	disc/GcnPartition.cpp
	disc/GcnPartitionPrivate.cpp
	disc/WiiPartition.cpp
	disc/GcnFst.cpp
	disc/PartitionFile.cpp
	disc/SparseDiscReader.cpp
	disc/PEResourceReader.cpp
	disc/NEResourceReader.cpp
	disc/NCCHReader.cpp
	config/ConfReader.cpp
	config/Config.cpp
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
	MegaDriveRegions.hpp
	GameCube.hpp
	NintendoDS.hpp
	RomDataFactory.hpp
	DMG.hpp
	uvector.h
	GameBoyAdvance.hpp
	CopierFormats.h
	SystemRegion.hpp
	GameCubeSave.hpp
	PlayStationSave.hpp
	N64.hpp
	SNES.hpp
	DreamcastSave.hpp
	VirtualBoy.hpp
	Amiibo.hpp
	NES.hpp
	EXE.hpp
	EXE_p.hpp
	WiiU.hpp
	Nintendo3DS.hpp
	md_structs.h
	gcn_structs.h
	gcn_card.h
	nds_structs.h
	n64_structs.h
	dmg_structs.h
	gba_structs.h
	ps1_structs.h
	snes_structs.h
	dc_structs.h
	vb_structs.h
	nfp_structs.h
	nes_structs.h
	wiiu_structs.h
	n3ds_structs.h
	data/MegaDrivePublishers.hpp
	data/NintendoPublishers.hpp
	data/AmiiboData.hpp
	data/WiiSystemMenuVersion.hpp
	data/NESMappers.hpp
	data/EXEData.hpp
	data/WiiUData.hpp
	file/IRpFile.hpp
	file/RpFile.hpp
	file/RpMemFile.hpp
	file/FileSystem.hpp
	img/rp_image.hpp
	img/rp_image_p.hpp
	img/rp_image_backend.hpp
	img/RpImageLoader.hpp
	img/ImageDecoder.hpp
	img/RpPng.hpp
	img/IconAnimData.hpp
	img/IconAnimHelper.hpp
	img/APNG_dlopen.h
	img/TCreateThumbnail.hpp
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
	disc/SparseDiscReader.hpp
	disc/SparseDiscReader_p.hpp
	disc/IResourceReader.hpp
	disc/PEResourceReader.hpp
	disc/NEResourceReader.hpp
	disc/NCCHReader.hpp
	disc/NCCHReader_p.hpp
	config/ConfReader.hpp
	config/Config.hpp
	)

IF(NOT USE_INTERNAL_PNG OR (USE_INTERNAL_PNG AND USE_INTERNAL_PNG_DLL))
	SET(libromdata_SRCS ${libromdata_SRCS} img/APNG_dlopen.c)
	SET(libromdata_NEEDS_DL 1)
ENDIF(NOT USE_INTERNAL_PNG OR (USE_INTERNAL_PNG AND USE_INTERNAL_PNG_DLL))

IF(JPEG_FOUND)
	SET(libromdata_SRCS ${libromdata_SRCS} img/RpJpeg.cpp)
	SET(libromdata_H ${libromdata_H} img/RpJpeg.hpp)
ENDIF(JPEG_FOUND)

IF(ENABLE_XML)
	SET(libromdata_SRCS ${libromdata_SRCS} EXE_manifest.cpp)
ENDIF(ENABLE_XML)

IF(WIN32)
	SET(libromdata_OS_SRCS
		RpWin32.cpp
		TextFuncs_win32.cpp
		file/win32/FileSystem_win32.cpp
		file/win32/RpFile_win32.cpp
		file/win32/IStreamWrapper.cpp
		img/GdiplusHelper.cpp
		img/RpGdiplusBackend.cpp
		threads/InitOnceExecuteOnceXP.c
		)
	SET(libromdata_OS_H
		RpWin32.hpp
		RpWin32_sdk.h
		file/win32/IStreamWrapper.hpp
		img/GdiplusHelper.hpp
		img/RpGdiplusBackend.hpp
		threads/InitOnceExecuteOnceXP.h
		Win32_ExeInit.hpp
		)
ELSE(WIN32)
	SET(libromdata_OS_SRCS
		TextFuncs_iconv.cpp
		file/FileSystem_posix.cpp
		file/RpFile_stdio.cpp
		)
ENDIF(WIN32)

IF(ENABLE_DECRYPTION)
	SET(libromdata_CRYPTO_SRCS
		crypto/AesCipherFactory.cpp
		crypto/KeyManager.cpp
		crypto/CtrKeyScrambler.cpp
		disc/NCCHReader_VerifyKeys.cpp
		)
	SET(libromdata_CRYPTO_H
		crypto/IAesCipher.hpp
		crypto/KeyManager.hpp
		crypto/CtrKeyScrambler.hpp
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

# Find the system threading library.
FIND_PACKAGE(Threads REQUIRED)
# Threading implementation.
SET(libromdata_THREAD_H
	threads/Atomics.h
	threads/Semaphore.hpp
	threads/Mutex.hpp
	)
IF(CMAKE_USE_WIN32_THREADS_INIT)
	SET(libromdata_THREAD_SRCS
		threads/SemaphoreWin32.cpp
		threads/MutexWin32.cpp
		)
ELSEIF(CMAKE_USE_PTHREADS_INIT)
	SET(libromdata_THREAD_SRCS
		threads/SemaphorePosix.cpp
		threads/MutexPosix.cpp
		)
ELSE()
	MESSAGE(FATAL_ERROR "No threading model is supported on this system.")
ENDIF()

######################
# Build the library. #
######################

# UTF-8 version.
ADD_LIBRARY(romdata8 STATIC
	${libromdata_SRCS} ${libromdata_H}
	${libromdata_OS_SRCS} ${libromdata_OS_H}
	${libromdata_CRYPTO_SRCS} ${libromdata_CRYPTO_H}
	${libromdata_CRYPTO_OS_SRCS} ${libromdata_CRYPTO_OS_H}
	${libromdata_THREAD_SRCS} ${libromdata_THREAD_H}
	)
TARGET_COMPILE_DEFINITIONS(romdata8 PUBLIC -DRP_UTF8)

# UTF-16 version.
ADD_LIBRARY(romdata16 STATIC
	${libromdata_SRCS} ${libromdata_H}
	${libromdata_OS_SRCS} ${libromdata_OS_H}
	${libromdata_CRYPTO_SRCS} ${libromdata_CRYPTO_H}
	${libromdata_CRYPTO_OS_SRCS} ${libromdata_CRYPTO_OS_H}
	${libromdata_THREAD_SRCS} ${libromdata_THREAD_H}
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
	SET_TARGET_PROPERTIES(${_target} PROPERTIES EXCLUDE_FROM_ALL TRUE)

	# Link in libdl if it's required for dlopen()
	# and we have a component that uses it.
	IF(libromdata_NEEDS_DL AND CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(${_target} ${CMAKE_DL_LIBS})
	ENDIF(libromdata_NEEDS_DL AND CMAKE_DL_LIBS)

	# Other libraries.
	TARGET_LINK_LIBRARIES(${_target} inih)
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
	IF(JPEG_FOUND)
		TARGET_LINK_LIBRARIES(${_target} ${JPEG_LIBRARY})
		TARGET_INCLUDE_DIRECTORIES(${_target} PRIVATE ${JPEG_INCLUDE_DIRS})
	ENDIF(JPEG_FOUND)
	IF(NETTLE_FOUND)
		TARGET_LINK_LIBRARIES(${_target} ${NETTLE_LIBRARY})
		TARGET_INCLUDE_DIRECTORIES(${_target} PRIVATE ${NETTLE_INCLUDE_DIRS})
	ENDIF(NETTLE_FOUND)
	IF(ENABLE_XML)
		TARGET_LINK_LIBRARIES(${_target} ${TinyXML2_LIBRARY})
	ENDIF(ENABLE_XML)
	IF(WIN32)
		TARGET_LINK_LIBRARIES(${_target} gdiplus)
		IF(ENABLE_DECRYPTION)
			TARGET_LINK_LIBRARIES(${_target} advapi32)
		ENDIF(ENABLE_DECRYPTION)
	ENDIF(WIN32)
	IF(CMAKE_THREAD_LIBS_INIT)
		TARGET_LINK_LIBRARIES(${_target} ${CMAKE_THREAD_LIBS_INIT})
	ENDIF(CMAKE_THREAD_LIBS_INIT)
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
