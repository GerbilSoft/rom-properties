# rom-properties base library
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(librpbase LANGUAGES C CXX)

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
		INCLUDE(CheckNettle2or3)
		CHECK_NETTLE_2_OR_3()
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

# Check for reentrant time functions.
# NOTE: May be _gmtime32_s() or _gmtime64_s() on MSVC 2005+.
# The "inline" part will detect that.
INCLUDE(CheckSymbolExistsOrInline)
CHECK_SYMBOL_EXISTS_OR_INLINE(gmtime_r "time.h" "time_t tm; gmtime_r(&tm, NULL);" HAVE_GMTIME_R)
IF(NOT HAVE_GMTIME_R)
	CHECK_SYMBOL_EXISTS_OR_INLINE(gmtime_s "time.h" "time_t tm; gmtime_s(NULL, &tm);" HAVE_GMTIME_S)
ENDIF(NOT HAVE_GMTIME_R)
CHECK_SYMBOL_EXISTS_OR_INLINE(localtime_r "time.h" "time_t tm; localtime_r(&tm, NULL);" HAVE_LOCALTIME_R)
IF(NOT HAVE_LOCALTIME_R)
	CHECK_SYMBOL_EXISTS_OR_INLINE(localtime_s "time.h" "time_t tm; localtime_s(NULL, &tm);" HAVE_LOCALTIME_S)
ENDIF(NOT HAVE_LOCALTIME_R)

# Other time functions.
CHECK_SYMBOL_EXISTS_OR_INLINE(timegm "time.h" "struct tm tm; time_t x = timegm(&tm);" HAVE_TIMEGM)
IF(NOT HAVE_TIMEGM)
	# NOTE: MSVCRT's _mkgmtime64() has a range of [1970/01/01, 3000/12/31].
	# glibc and boost both support arbitrary ranges.
	CHECK_SYMBOL_EXISTS_OR_INLINE(_mkgmtime "time.h" "struct tm tm; time_t x = _mkgmtime(&tm);" HAVE__MKGMTIME)
	CHECK_SYMBOL_EXISTS_OR_INLINE(_mkgmtime64 "time.h" "struct tm tm; time_t x = _mkgmtime64(&tm);" HAVE__MKGMTIME64)
ENDIF(NOT HAVE_TIMEGM)
IF(NOT HAVE_TIMEGM AND NOT HAVE__MKGMTIME AND NOT HAVE__MKGMTIME64)
	SET(USE_BOOST_TIMEGM 1)
ENDIF(NOT HAVE_TIMEGM AND NOT HAVE__MKGMTIME AND NOT HAVE__MKGMTIME64)

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
	TextFuncs.cpp
	TextFuncs_libc.c
	TextFuncs_conv.cpp
	RomData.cpp
	RomFields.cpp
	RomMetaData.cpp
	SystemRegion.cpp
	TextOut_common.cpp
	TextOut_text.cpp
	TextOut_json.cpp
	Achievements.cpp
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
	uvector.h
	aligned_malloc.h
	TextFuncs.hpp
	TextFuncs_wchar.hpp
	TextFuncs_libc.h
	RomData.hpp
	RomData_decl.hpp
	RomData_p.hpp
	RomFields.hpp
	RomMetaData.hpp
	SystemRegion.hpp
	d_type.h
	TextOut.hpp
	Achievements.hpp
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

# timegm.c for systems that don't have timegm() or 64-bit _mkgmtime().
IF(USE_BOOST_TIMEGM)
	SET(librpbase_SRCS ${librpbase_SRCS} timegm.c)
ENDIF(USE_BOOST_TIMEGM)

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

IF(WIN32)
	SET(librpbase_OS_SRCS TextFuncs_win32.cpp)
	SET(librpbase_OS_H    TextFuncs_wchar.hpp)
ELSE(WIN32)
	SET(librpbase_OS_SRCS TextFuncs_iconv.cpp)
ENDIF(WIN32)

IF(ENABLE_DECRYPTION)
	SET(librpbase_CRYPTO_SRCS crypto/AesCipherFactory.cpp)
	SET(librpbase_CRYPTO_H    crypto/IAesCipher.hpp crypto/MD5Hash.hpp)
	IF(WIN32)
		SET(librpbase_CRYPTO_OS_SRCS
			crypto/AesCAPI.cpp
			crypto/AesCAPI_NG.cpp
			crypto/MD5HashCAPI.cpp
			)
		SET(librpbase_CRYPTO_OS_H
			crypto/AesCAPI.hpp
			crypto/AesCAPI_NG.hpp
			)
	ELSE(WIN32)
		SET(librpbase_CRYPTO_OS_SRCS crypto/AesNettle.cpp crypto/MD5HashNettle.cpp)
		SET(librpbase_CRYPTO_OS_H    crypto/AesNettle.hpp)
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
	IF(JPEG_FOUND AND NOT WIN32)
		SET(librpbase_SSSE3_SRCS
			${librpbase_SSSE3_SRCS}
			img/RpJpeg_ssse3.cpp
			)
	ENDIF(JPEG_FOUND AND NOT WIN32)

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSSE3_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		SET(SSSE3_FLAG "-mssse3")
	ENDIF()

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

# Write the config.h files.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.librpbase.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.librpbase.h")

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
		${RAPIDJSON_INCLUDE_DIRS}				# rapidjson
	)
ADD_DEFINITIONS(-DRAPIDJSON_HAS_STDSTRING)

# Exclude from ALL builds.
SET_TARGET_PROPERTIES(rpbase PROPERTIES EXCLUDE_FROM_ALL TRUE)
# Make sure git_version.h is created before compiling this target.
IF(TARGET git_version)
	ADD_DEPENDENCIES(rpbase git_version)
ENDIF(TARGET git_version)

IF(MSVC)
	# Delay-load implementation.
	TARGET_LINK_LIBRARIES(rpbase PRIVATE delayimp)
ENDIF(MSVC)

IF(NOT ZLIB_LIBRARY)
	MESSAGE(FATAL_ERROR "ZLIB_LIBRARY has not been set by CheckZLIB.cmake.")
ENDIF(NOT ZLIB_LIBRARY)
TARGET_LINK_LIBRARIES(rpbase PRIVATE ${ZLIB_LIBRARY})

# Link in libdl if it's required for dlopen()
# and we have a component that uses it.
IF(librpbase_NEEDS_DL AND CMAKE_DL_LIBS)
	TARGET_LINK_LIBRARIES(rpbase PRIVATE ${CMAKE_DL_LIBS})
ENDIF(librpbase_NEEDS_DL AND CMAKE_DL_LIBS)

# Other libraries.
TARGET_LINK_LIBRARIES(rpbase PRIVATE rpcpu rpfile rptexture rpfile inih rpthreads cachecommon)
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

# rapidjson: Enable std::string support.
ADD_DEFINITIONS(-DRAPIDJSON_HAS_STDSTRING)

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
