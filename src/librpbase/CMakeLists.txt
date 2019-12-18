PROJECT(librpbase)

IF(NOT WIN32)
	# Non-Windows library checks.

	# On Win32, we use MultiByteToWideChar() and WideCharToMultiByte().
	# On other systems, we use iconv(), which could be in libc or libiconv.
	# Figure out which library has iconv().

	# Check for iconv().
	FIND_PACKAGE(Iconv REQUIRED)
	SET(HAVE_ICONV 1)
	IF(NOT Iconv_IS_BUILT_IN)
		SET(HAVE_ICONV_LIBICONV 1)
	ENDIF(NOT Iconv_IS_BUILT_IN)

	IF(ENABLE_DECRYPTION)
		FIND_PACKAGE(Nettle REQUIRED)
		SET(HAVE_NETTLE ${NETTLE_FOUND})
		IF(HAVE_NETTLE)
			# Check if this is Nettle 3.x.
			# Nettle 3.1 added version.h, which isn't available
			# in older verisons, so we can't simply check that.
			INCLUDE(CheckSymbolExists)
			SET(OLD_CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES}")
			SET(OLD_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
			SET(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES} ${NETTLE_INCLUDE_DIRS}")
			SET(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES} ${NETTLE_LIBRARY}")
			CHECK_SYMBOL_EXISTS(aes128_set_decrypt_key "nettle/aes.h" HAVE_NETTLE_3)
			IF(HAVE_NETTLE_3)
				# Check for Nettle versioning symbols.
				# Nettle 3.1 added version.h.
				CHECK_SYMBOL_EXISTS(NETTLE_VERSION_MAJOR "nettle/version.h" HAVE_NETTLE_VERSION_H)
				CHECK_SYMBOL_EXISTS(nettle_version_major "nettle/version.h" HAVE_NETTLE_VERSION_FUNCTIONS)
			ENDIF(HAVE_NETTLE_3)
			SET(CMAKE_REQUIRED_INCLUDES "${OLD_CMAKE_REQUIRED_INCLUDES}")
			SET(CMAKE_REQUIRED_LIBRARIES "${OLD_CMAKE_REQUIRED_LIBRARIES}")
			UNSET(OLD_CMAKE_REQUIRED_INCLUDES)
			UNSET(OLD_CMAKE_REQUIRED_LIBRARIES)
		ELSE(HAVE_NETTLE)
			# Disable decryption.
			SET(ENABLE_DECRYPTION OFF CACHE INTERNAL "Enable decryption for newer ROM and disc images." FORCE)
		ENDIF(HAVE_NETTLE)
	ENDIF(ENABLE_DECRYPTION)
ENDIF(NOT WIN32)

# ZLIB and libpng are checked in the top-level CMakeLists.txt.

# Check for C library functions.
INCLUDE(CheckSymbolExists)
INCLUDE(CheckStructHasMember)
CHECK_SYMBOL_EXISTS(strnlen "string.h" HAVE_STRNLEN)
CHECK_SYMBOL_EXISTS(memmem "string.h" HAVE_MEMMEM)
# MSVCRT doesn't have nl_langinfo() and probably never will.
IF(NOT WIN32)
	CHECK_SYMBOL_EXISTS(nl_langinfo "langinfo.h" HAVE_NL_LANGINFO)
ELSE(NOT WIN32)
	# Win32: MinGW's `struct lconv` doesn't have wchar_t fields.
	CHECK_STRUCT_HAS_MEMBER("struct lconv" _W_decimal_point "locale.h"
		HAVE_STRUCT_LCONV_WCHAR_T
		LANGUAGE C)
ENDIF(NOT WIN32)

# Check for reentrant C library functions.
INCLUDE(CheckSymbolExistsOrInline)
# putenv(), setenv(), and related.
# setenv() is preferred, so if it's found, skip everything else.
# FIXME: putenv() and setenv() aren't likely to be inline functions, but
# CHECK_SYMBOL_EXISTS() complains due to the embedded double-quotes.
CHECK_SYMBOL_EXISTS_OR_INLINE(putenv "stdlib.h" "putenv(\"abc123=def456\");" HAVE_PUTENV)
CHECK_SYMBOL_EXISTS_OR_INLINE(setenv "stdlib.h" "setenv(\"abc123\", \"def456\", 1);" HAVE_SETENV)
# Windows-specific: Unicode only.
# NOTE: These may be inline functions on MinGW.
CHECK_SYMBOL_EXISTS_OR_INLINE(_wputenv "stdlib.h" "_wputenv(L\"abc123=def456\");" HAVE__WPUTENV)
CHECK_SYMBOL_EXISTS_OR_INLINE(_wputenv_s "stdlib.h" "_wputenv_s(L\"abc123\", \"def456\");" HAVE__WPUTENV_S)

# Check for C headers.
CHECK_INCLUDE_FILES("features.h" HAVE_FEATURES_H)

# Check for unordered_map::reserve and unordered_set::reserve.
SET(OLD_CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
CHECK_CXX_SOURCE_COMPILES("#include <unordered_map>
int main(void) {
	std::unordered_map<int, int> testMap;
	testMap.reserve(4);
	return 0;
}" HAVE_UNORDERED_MAP_RESERVE)
CHECK_CXX_SOURCE_COMPILES("#include <unordered_set>
int main(void) {
	std::unordered_set<int> testSet;
	testSet.reserve(4);
	return 0;
}" HAVE_UNORDERED_SET_RESERVE)
SET(CMAKE_REQUIRED_INCLUDES ${OLD_CMAKE_REQUIRED_INCLUDES})
UNSET(OLD_CMAKE_REQUIRED_INCLUDES)

# Sources.
SET(librpbase_SRCS
	byteswap.c
	TextFuncs.cpp
	TextFuncs_libc.c
	TextFuncs_conv.cpp
	RomData.cpp
	RomFields.cpp
	RomMetaData.cpp
	SystemRegion.cpp
	file/IRpFile.cpp
	file/RpMemFile.cpp
	file/FileSystem_common.cpp
	file/RelatedFile.cpp
	file/scsi/RpFile_Kreon.cpp
	file/scsi/RpFile_scsi.cpp
	img/RpImageLoader.cpp
	img/RpPng.cpp
	img/RpPngWriter.cpp
	img/IconAnimHelper.cpp
	img/pngcheck/pngcheck.cpp
	disc/IDiscReader.cpp
	disc/DiscReader.cpp
	disc/PartitionFile.cpp
	disc/SparseDiscReader.cpp
	disc/CBCReader.cpp
	crypto/KeyManager.cpp
	config/ConfReader.cpp
	config/Config.cpp
	config/AboutTabText.cpp
	)
# Headers.
SET(librpbase_H
	byteorder.h
	byteswap.h
	common.h
	uvector.h
	aligned_malloc.h
	ctypex.h
	TextFuncs.hpp
	TextFuncs_wchar.hpp
	TextFuncs_libc.h
	RomData.hpp
	RomData_decl.hpp
	RomData_p.hpp
	RomFields.hpp
	RomMetaData.hpp
	SystemRegion.hpp
	bitstuff.h
	file/IRpFile.hpp
	file/RpFile.hpp
	file/RpFile_p.hpp
	file/RpMemFile.hpp
	file/FileSystem.hpp
	file/RelatedFile.hpp
	file/scsi/ata_protocol.h
	file/scsi/scsi_protocol.h
	file/scsi/scsi_ata_cmds.h
	img/RpPng.hpp
	img/RpPngWriter.hpp
	img/APNG_dlopen.h
	disc/IDiscReader.hpp
	disc/DiscReader.hpp
	disc/IPartition.hpp
	disc/IFst.hpp
	disc/PartitionFile.hpp
	disc/SparseDiscReader.hpp
	disc/SparseDiscReader_p.hpp
	disc/CBCReader.hpp
	crypto/KeyManager.hpp
	config/ConfReader.hpp
	config/Config.hpp
	config/AboutTabText.hpp
	)

IF(NOT USE_INTERNAL_PNG OR (USE_INTERNAL_PNG AND USE_INTERNAL_PNG_DLL))
	SET(librpbase_SRCS ${librpbase_SRCS} img/APNG_dlopen.c)
	SET(librpbase_NEEDS_DL 1)
ENDIF(NOT USE_INTERNAL_PNG OR (USE_INTERNAL_PNG AND USE_INTERNAL_PNG_DLL))

IF(WIN32)
	SET(librpbase_SRCS ${librpbase_SRCS} img/RpJpeg_gdiplus.cpp)
	SET(librpbase_H ${librpbase_H} img/RpJpeg.hpp)
ELSEIF(JPEG_FOUND)
	SET(librpbase_SRCS ${librpbase_SRCS} img/RpJpeg.cpp)
	SET(librpbase_H ${librpbase_H} img/RpJpeg.hpp)
ENDIF()

# SCSI implementation for Kreon disc drive support.
IF(WIN32)
	SET(librpbase_SRCS ${librpbase_SRCS} file/scsi/RpFile_scsi_win32.cpp)
	SET(RP_OS_SCSI_SUPPORTED 1)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	SET(librpbase_SRCS ${librpbase_SRCS} file/scsi/RpFile_scsi_linux.cpp)
	SET(RP_OS_SCSI_SUPPORTED 1)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "Dragonfly")
	# FIXME: Not working properly. (SCSI ILLEGAL REQUEST errors)
	# NOTE: The file is still needed for rereadDeviceSizeOS().
	SET(librpbase_SRCS ${librpbase_SRCS} file/scsi/RpFile_scsi_freebsd.cpp)

	IF(0) # FIXME
	# TODO: Check for header files. CHECK_INCLUDE_FILE() doesn't work
	# properly because these header files use FILE*, so we need to
	# #include <stdio.h>.
	INCLUDE(CheckIncludeFile)
	INCLUDE(CheckLibraryExists)
	#CHECK_INCLUDE_FILE("camlib.h" HAVE_CAMLIB_H)
	#CHECK_INCLUDE_FILE("cam/scsi/scsi_message.h" HAVE_CAM_SCSI_SCSI_MESSAGE_H)
	#CHECK_INCLUDE_FILE("cam/scsi/scsi_pass.h" HAVE_CAM_SCSI_SCSI_PASS_H)
	CHECK_LIBRARY_EXISTS(cam "cam_open_pass" "" HAVE_LIBCAM)
	IF(HAVE_LIBCAM)
		SET(SCSI_LIBRARY cam)
		SET(RP_OS_SCSI_SUPPORTED 1)
	ENDIF()
	ENDIF(0) # FIXME
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "NetBSD" OR CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
	SET(librpbase_SRCS ${librpbase_SRCS} file/scsi/RpFile_scsi_netbsd.cpp)
	SET(RP_OS_SCSI_SUPPORTED 1)
ELSE()
	# Dummy implementation for RpFile::rereadDeviceSizeOS().
	SET(librpbase_SRCS ${librpbase_SRCS} file/scsi/RpFile_scsi_dummy.cpp)
ENDIF()

IF(WIN32)
	SET(librpbase_OS_SRCS
		TextFuncs_win32.cpp
		file/win32/FileSystem_win32.cpp
		file/win32/RpFile_win32.cpp
		file/win32/IStreamWrapper.cpp
		)
	SET(librpbase_OS_H
		TextFuncs_wchar.hpp
		file/win32/IStreamWrapper.hpp
		)
ELSE(WIN32)
	SET(librpbase_OS_SRCS
		TextFuncs_iconv.cpp
		file/FileSystem_posix.cpp
		file/RpFile_stdio.cpp
		)
ENDIF(WIN32)

IF(ENABLE_DECRYPTION)
	SET(librpbase_CRYPTO_SRCS
		crypto/AesCipherFactory.cpp
		)
	SET(librpbase_CRYPTO_H
		crypto/IAesCipher.hpp
		)
	IF(WIN32)
		SET(librpbase_CRYPTO_OS_SRCS
			crypto/AesCAPI.cpp
			crypto/AesCAPI_NG.cpp
			)
		SET(librpbase_CRYPTO_OS_H
			crypto/AesCAPI.hpp
			crypto/AesCAPI_NG.hpp
			)
	ELSE(WIN32)
		SET(librpbase_CRYPTO_OS_SRCS
			crypto/AesNettle.cpp
			)
		SET(librpbase_CRYPTO_OS_H
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

# CPU-specific and optimized sources.
IF(CPU_i386 OR CPU_amd64)
	SET(librpbase_CPU_SRCS cpuflags_x86.c)
	SET(librpbase_CPU_H cpuflags_x86.h)

	# MSVC does not support MMX intrinsics in 64-bit builds.
	# Reference: https://msdn.microsoft.com/en-us/library/08x3t697(v=vs.110).aspx
	#
	# amd64 CPUs support SSE2 as a minimum anyway, so there's
	# no point in building MMX code for 64-bit.
	IF(CPU_i386)
		SET(librpbase_MMX_SRCS byteswap_mmx.c)
	ENDIF(CPU_i386)

	SET(librpbase_SSE2_SRCS byteswap_sse2.c)
	SET(librpbase_SSSE3_SRCS byteswap_ssse3.c)
	IF(JPEG_FOUND AND NOT WIN32)
		SET(librpbase_SSSE3_SRCS
			${librpbase_SSSE3_SRCS}
			img/RpJpeg_ssse3.cpp
			)
	ENDIF(JPEG_FOUND AND NOT WIN32)

	# IFUNC requires glibc.
	# We're not checking for glibc here, but we do have preprocessor
	# checks, so even if this does get compiled on a non-glibc system,
	# it won't do anything.
	# TODO: Might be supported on other Unix-like operating systems...
	IF(UNIX AND NOT APPLE)
		SET(librpbase_IFUNC_SRCS byteswap_ifunc.c)
		# Disable LTO on the IFUNC files if LTO is known to be broken.
		IF(GCC_5xx_LTO_ISSUES)
			SET_SOURCE_FILES_PROPERTIES(${librpbase_IFUNC_SRCS}
				APPEND_STRING PROPERTIES COMPILE_FLAGS " -fno-lto ")
		ENDIF(GCC_5xx_LTO_ISSUES)
	ENDIF(UNIX AND NOT APPLE)

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSE2_FLAG "/arch:SSE2")
		SET(SSSE3_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		SET(MMX_FLAG "-mmmx")
		SET(SSE2_FLAG "-msse2")
		SET(SSSE3_FLAG "-mssse3")
	ENDIF()

	IF(MMX_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${librpbase_MMX_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${MMX_FLAG} ")
	ENDIF(MMX_FLAG)

	IF(SSE2_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${librpbase_SSE2_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSE2_FLAG} ")
	ENDIF(SSE2_FLAG)

	IF(SSSE3_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${librpbase_SSSE3_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
	ENDIF(SSSE3_FLAG)
ENDIF()
UNSET(arch)

# Aligned malloc() functions.
# NOTE: MinGW-w64 only has _aligned_malloc().
# It doesn't even have inline function wrappers
# for the other functions.
CHECK_SYMBOL_EXISTS(_aligned_malloc	"malloc.h"		HAVE_MSVC_ALIGNED_MALLOC)
# FIXME: aligned_alloc isn't working on properly on Mac OS.
#CHECK_SYMBOL_EXISTS(aligned_alloc	"stdlib.h"		HAVE_ALIGNED_ALLOC)
CHECK_SYMBOL_EXISTS(posix_memalign	"stdlib.h"		HAVE_POSIX_MEMALIGN)
CHECK_SYMBOL_EXISTS(memalign		"stdlib.h;malloc.h"	HAVE_MEMALIGN)
IF(NOT HAVE_MSVC_ALIGNED_MALLOC AND NOT HAVE_ALIGNED_ALLOC AND NOT HAVE_POSIX_MEMALIGN AND NOT HAVE_MEMALIGN)
	MESSAGE(FATAL_ERROR "System does not have an aligned malloc() function.")
ENDIF()

# gcc byteswapping intrinsics.
IF(NOT MSVC)
	CHECK_C_SOURCE_COMPILES("int main(int argc, char *argv[]) { return __builtin_bswap16((unsigned short)argc); }" HAVE___BUILTIN_BSWAP16)
	CHECK_C_SOURCE_COMPILES("int main(int argc, char *argv[]) { return __builtin_bswap32((unsigned int)argc); }" HAVE___BUILTIN_BSWAP32)
	CHECK_C_SOURCE_COMPILES("int main(int argc, char *argv[]) { return __builtin_bswap64((unsigned long long)argc); }" HAVE___BUILTIN_BSWAP64)
ENDIF(NOT MSVC)

# Write the config.h files.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.librpbase.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.librpbase.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.byteswap.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.byteswap.h")

IF(ENABLE_PCH)
	# Precompiled headers.
	INCLUDE(PrecompiledHeader)
	SET(librpbase_PCH_C stdafx.c)
	SET(librpbase_PCH_CXX stdafx.cpp)
	SET(librpbase_PCH_SRC ${librpbase_PCH_C} ${librpbase_PCH_CXX})
	SET(librpbase_PCH_H stdafx.h)
ENDIF(ENABLE_PCH)

######################
# Build the library. #
######################

ADD_LIBRARY(rpbase STATIC
	${librpbase_PCH_SRC} ${librpbase_PCH_H}
	${librpbase_SRCS} ${librpbase_H}
	${librpbase_OS_SRCS} ${librpbase_OS_H}
	${librpbase_CRYPTO_SRCS} ${librpbase_CRYPTO_H}
	${librpbase_CRYPTO_OS_SRCS} ${librpbase_CRYPTO_OS_H}
	${librpbase_CPU_SRCS} ${librpbase_CPU_H}
	${librpbase_IFUNC_SRCS}
	${librpbase_MMX_SRCS}
	${librpbase_SSE2_SRCS}
	${librpbase_SSSE3_SRCS}
	)
IF(ENABLE_PCH)
	ADD_PRECOMPILED_HEADER(rpbase ${librpbase_PCH_H}
		SOURCE_C ${librpbase_PCH_C}
		SOURCE_CXX ${librpbase_PCH_CXX}
		)
ENDIF(ENABLE_PCH)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(rpbase)

# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(rpbase
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# librpbase
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# librpbase
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)
IF(NOT ZLIB_LIBRARY)
	MESSAGE(FATAL_ERROR "ZLIB_LIBRARY has not been set by CheckZLIB.cmake.")
ENDIF(NOT ZLIB_LIBRARY)
TARGET_LINK_LIBRARIES(rpbase PRIVATE ${ZLIB_LIBRARY})
IF(MSVC)
	TARGET_LINK_LIBRARIES(rpbase PRIVATE delayimp)
ENDIF(MSVC)
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(rpbase PROPERTIES EXCLUDE_FROM_ALL TRUE)
# Make sure git_version.h is created before compiling this target.
IF(TARGET git_version)
	ADD_DEPENDENCIES(rpbase git_version)
ENDIF(TARGET git_version)

# Link in libdl if it's required for dlopen()
# and we have a component that uses it.
IF(librpbase_NEEDS_DL AND CMAKE_DL_LIBS)
	TARGET_LINK_LIBRARIES(rpbase PRIVATE ${CMAKE_DL_LIBS})
ENDIF(librpbase_NEEDS_DL AND CMAKE_DL_LIBS)

# Other libraries.
TARGET_LINK_LIBRARIES(rpbase PRIVATE rptexture inih rpthreads)
IF(Iconv_LIBRARY AND NOT Iconv_IS_BUILT_IN)
	TARGET_LINK_LIBRARIES(rpbase PRIVATE Iconv::Iconv)
ENDIF(Iconv_LIBRARY AND NOT Iconv_IS_BUILT_IN)
IF(ZLIB_FOUND)
	TARGET_LINK_LIBRARIES(rpbase PRIVATE ${ZLIB_LIBRARIES})
	TARGET_INCLUDE_DIRECTORIES(rpbase PRIVATE ${ZLIB_INCLUDE_DIRS})
	TARGET_COMPILE_DEFINITIONS(rpbase PRIVATE ${ZLIB_DEFINITIONS})
ENDIF(ZLIB_FOUND)
IF(PNG_FOUND)
	TARGET_LINK_LIBRARIES(rpbase PRIVATE ${PNG_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(rpbase PRIVATE ${PNG_INCLUDE_DIRS})
	TARGET_COMPILE_DEFINITIONS(rpbase PRIVATE ${PNG_DEFINITIONS})
ENDIF(PNG_FOUND)
IF(JPEG_FOUND)
	TARGET_LINK_LIBRARIES(rpbase PRIVATE ${JPEG_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(rpbase PRIVATE ${JPEG_INCLUDE_DIRS})
ENDIF(JPEG_FOUND)
IF(NETTLE_FOUND)
	TARGET_LINK_LIBRARIES(rpbase PRIVATE ${NETTLE_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(rpbase PRIVATE ${NETTLE_INCLUDE_DIRS})
ENDIF(NETTLE_FOUND)
IF(WIN32)
	# libwin32common
	TARGET_LINK_LIBRARIES(rpbase PRIVATE win32common)
	# Windows system libraries
	TARGET_LINK_LIBRARIES(rpbase PRIVATE gdiplus)
	IF(ENABLE_DECRYPTION)
		TARGET_LINK_LIBRARIES(rpbase PRIVATE advapi32)
	ENDIF(ENABLE_DECRYPTION)
ELSE(WIN32)
	# libunixcommon
	TARGET_LINK_LIBRARIES(rpbase PRIVATE unixcommon)
ENDIF(WIN32)
IF(SCSI_LIBRARY)
	# An extra library is needed for SCSI support.
	TARGET_LINK_LIBRARIES(rpbase PRIVATE ${SCSI_LIBRARY})
ENDIF(SCSI_LIBRARY)

IF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rpbase PRIVATE i18n)
ENDIF(ENABLE_NLS)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

# Test suite.
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
