PROJECT(librpbase)

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
			SET(ENABLE_DECRYPTION OFF CACHE "" INTERNAL FORCE)
		ENDIF(HAVE_NETTLE)
	ENDIF(ENABLE_DECRYPTION)
ENDIF(NOT WIN32)

# ZLIB and libpng are checked in the top-level CMakeLists.txt.

# Check for C library functions.
INCLUDE(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(strnlen "string.h" HAVE_STRNLEN)
CHECK_SYMBOL_EXISTS(memmem "string.h" HAVE_MEMMEM)

# Check for C headers.
CHECK_INCLUDE_FILES("features.h" HAVE_FEATURES_H)

# Sources.
SET(librpbase_SRCS
	byteswap.c
	TextFuncs.cpp
	RomData.cpp
	RomFields.cpp
	SystemRegion.cpp
	file/IRpFile.cpp
	file/RpMemFile.cpp
	file/FileSystem_common.cpp
	file/RelatedFile.cpp
	img/rp_image.cpp
	img/rp_image_backend.cpp
	img/rp_image_ops.cpp
	img/RpImageLoader.cpp
	img/ImageDecoder_Linear.cpp
	img/ImageDecoder_GCN.cpp
	img/ImageDecoder_NDS.cpp
	img/ImageDecoder_N3DS.cpp
	img/ImageDecoder_S3TC.cpp
	img/ImageDecoder_DC.cpp
	img/ImageDecoder_ETC1.cpp
	img/un-premultiply.cpp
	img/RpPng.cpp
	img/RpPngWriter.cpp
	img/IconAnimHelper.cpp
	img/pngcheck/pngcheck.cpp
	disc/IDiscReader.cpp
	disc/DiscReader.cpp
	disc/PartitionFile.cpp
	disc/SparseDiscReader.cpp
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
	TextFuncs.hpp
	TextFuncs_wchar.hpp
	RomData.hpp
	RomData_decl.hpp
	RomData_p.hpp
	RomFields.hpp
	SystemRegion.hpp
	bitstuff.h
	file/IRpFile.hpp
	file/RpFile.hpp
	file/RpMemFile.hpp
	file/FileSystem.hpp
	file/RelatedFile.hpp
	img/rp_image.hpp
	img/rp_image_p.hpp
	img/rp_image_backend.hpp
	img/RpImageLoader.hpp
	img/ImageDecoder.hpp
	img/ImageDecoder_p.hpp
	img/RpPng.hpp
	img/RpPngWriter.hpp
	img/IconAnimData.hpp
	img/IconAnimHelper.hpp
	img/APNG_dlopen.h
	disc/IDiscReader.hpp
	disc/DiscReader.hpp
	disc/IPartition.hpp
	disc/IFst.hpp
	disc/PartitionFile.hpp
	disc/SparseDiscReader.hpp
	disc/SparseDiscReader_p.hpp
	crypto/KeyManager.hpp
	config/ConfReader.hpp
	config/Config.hpp
	config/AboutTabText.hpp
	)

IF(NOT USE_INTERNAL_PNG OR (USE_INTERNAL_PNG AND USE_INTERNAL_PNG_DLL))
	SET(librpbase_SRCS ${librpbase_SRCS} img/APNG_dlopen.c)
	SET(librpbase_NEEDS_DL 1)
ENDIF(NOT USE_INTERNAL_PNG OR (USE_INTERNAL_PNG AND USE_INTERNAL_PNG_DLL))

IF(JPEG_FOUND)
	SET(librpbase_SRCS ${librpbase_SRCS} img/RpJpeg.cpp)
	SET(librpbase_H ${librpbase_H} img/RpJpeg.hpp)
ENDIF(JPEG_FOUND)

IF(WIN32)
	SET(librpbase_OS_SRCS
		TextFuncs_win32.cpp
		file/win32/FileSystem_win32.cpp
		file/win32/RpFile_win32.cpp
		file/win32/IStreamWrapper.cpp
		img/GdiplusHelper.cpp
		img/RpGdiplusBackend.cpp
		)
	SET(librpbase_OS_H
		TextFuncs_wchar.hpp
		file/win32/IStreamWrapper.hpp
		img/GdiplusHelper.hpp
		img/RpGdiplusBackend.hpp
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

# Find the system threading library.
FIND_PACKAGE(Threads REQUIRED)
# Threading implementation.
SET(librpbase_THREAD_H
	threads/Atomics.h
	threads/Semaphore.hpp
	threads/Mutex.hpp
	threads/pthread_once.h
	)
IF(CMAKE_USE_WIN32_THREADS_INIT)
	SET(HAVE_WIN32_THREADS 1)
	SET(librpbase_THREAD_SRCS
		threads/SemaphoreWin32.cpp
		threads/MutexWin32.cpp
		threads/pthread_once.c
		)
ELSEIF(CMAKE_USE_PTHREADS_INIT)
	SET(HAVE_PTHREADS 1)
	SET(librpbase_THREAD_SRCS
		threads/SemaphorePosix.cpp
		threads/MutexPosix.cpp
		)
ELSE()
	MESSAGE(FATAL_ERROR "No threading model is supported on this system.")
ENDIF()

# CPU-specific and optimized sources.
# TODO: Move arch detection to platform.cmake.
STRING(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" arch)
IF(arch MATCHES "^(i.|x)86$|^x86_64$|^amd64$")
	SET(librpbase_CPU_SRCS cpuflags_x86.c)
	SET(librpbase_CPU_H cpuflags_x86.h)

	# MSVC does not support MMX intrinsics in 64-bit builds.
	# Reference: https://msdn.microsoft.com/en-us/library/08x3t697(v=vs.110).aspx
	# TODO: Disable MMX on all 64-bit builds?
	IF(NOT MSVC OR NOT CMAKE_CL_64)
		SET(librpbase_MMX_SRCS byteswap_mmx.c)
	ENDIF(NOT MSVC OR NOT CMAKE_CL_64)

	SET(librpbase_SSE2_SRCS
		byteswap_sse2.c
		img/ImageDecoder_Linear_sse2.cpp
		img/rp_image_ops_sse2.cpp
		)
	SET(librpbase_SSSE3_SRCS
		byteswap_ssse3.c
		img/RpJpeg_ssse3.cpp
		img/ImageDecoder_Linear_ssse3.cpp
		)

	# IFUNC requires glibc.
	# We're not checking for glibc here, but we do have preprocessor
	# checks, so even if this does get compiled on a non-glibc system,
	# it won't do anything.
	# TODO: Might be supported on other Unix-like operating systems...
	IF(UNIX AND NOT APPLE)
		SET(librpbase_IFUNC_SRCS
			byteswap_ifunc.c
			img/ImageDecoder_ifunc.cpp
			)
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
		FOREACH(mmx_file ${librpbase_MMX_SRCS})
			SET_SOURCE_FILES_PROPERTIES(${mmx_file}
				APPEND_STRING PROPERTIES COMPILE_FLAGS " ${MMX_FLAG} ")
		ENDFOREACH()
	ENDIF(MMX_FLAG)

	IF(SSE2_FLAG)
		FOREACH(sse2_file ${librpbase_SSE2_SRCS})
			SET_SOURCE_FILES_PROPERTIES(${sse2_file}
				APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSE2_FLAG} ")
		ENDFOREACH()
	ENDIF(SSE2_FLAG)

	IF(SSSE3_FLAG)
		FOREACH(ssse3_file ${librpbase_SSSE3_SRCS})
			SET_SOURCE_FILES_PROPERTIES(${ssse3_file}
				APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
		ENDFOREACH()
	ENDIF(SSSE3_FLAG)
ENDIF()
UNSET(arch)

# Aligned malloc() functions.
# NOTE: MinGW-w64 only has _aligned_malloc().
# It doesn't even have inline function wrappers
# for the other functions.
CHECK_SYMBOL_EXISTS(_aligned_malloc	"malloc.h"		HAVE_MSVC_ALIGNED_MALLOC)
CHECK_SYMBOL_EXISTS(aligned_alloc	"stdlib.h"		HAVE_ALIGNED_ALLOC)
CHECK_SYMBOL_EXISTS(posix_memalign	"stdlib.h"		HAVE_POSIX_MEMALIGN)
CHECK_SYMBOL_EXISTS(memalign		"stdlib.h;malloc.h"	HAVE_MEMALIGN)
IF(NOT HAVE_MSVC_ALIGNED_MALLOC AND NOT HAVE_ALIGNED_ALLOC AND NOT HAVE_POSIX_MEMALIGN AND NOT HAVE_MEMALIGN)
	MESSAGE(FATAL_ERROR "System does not have an aligned malloc() function.")
ENDIF()

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.librpbase.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.librpbase.h")

######################
# Build the library. #
######################

ADD_LIBRARY(rpbase STATIC
	${librpbase_SRCS} ${librpbase_H}
	${librpbase_OS_SRCS} ${librpbase_OS_H}
	${librpbase_CRYPTO_SRCS} ${librpbase_CRYPTO_H}
	${librpbase_CRYPTO_OS_SRCS} ${librpbase_CRYPTO_OS_H}
	${librpbase_THREAD_SRCS} ${librpbase_THREAD_H}
	${librpbase_CPU_SRCS} ${librpbase_CPU_H}
	${librpbase_IFUNC_SRCS}
	${librpbase_MMX_SRCS}
	${librpbase_SSE2_SRCS}
	${librpbase_SSSE3_SRCS}
	)

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
IF(MSVC)
	TARGET_LINK_LIBRARIES(rpbase delayimp)
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
	TARGET_LINK_LIBRARIES(rpbase ${CMAKE_DL_LIBS})
ENDIF(librpbase_NEEDS_DL AND CMAKE_DL_LIBS)

# Other libraries.
TARGET_LINK_LIBRARIES(rpbase inih)
IF(ICONV_LIBRARY)
	TARGET_LINK_LIBRARIES(rpbase ${ICONV_LIBRARY})
ENDIF(ICONV_LIBRARY)
IF(ZLIB_FOUND)
	TARGET_LINK_LIBRARIES(rpbase ${ZLIB_LIBRARIES})
	TARGET_INCLUDE_DIRECTORIES(rpbase PRIVATE ${ZLIB_INCLUDE_DIRS})
	TARGET_COMPILE_DEFINITIONS(rpbase PRIVATE ${ZLIB_DEFINITIONS})
ENDIF(ZLIB_FOUND)
IF(PNG_FOUND)
	TARGET_LINK_LIBRARIES(rpbase ${PNG_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(rpbase PRIVATE ${PNG_INCLUDE_DIRS})
	TARGET_COMPILE_DEFINITIONS(rpbase PRIVATE ${PNG_DEFINITIONS})
ENDIF(PNG_FOUND)
IF(JPEG_FOUND)
	TARGET_LINK_LIBRARIES(rpbase ${JPEG_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(rpbase PRIVATE ${JPEG_INCLUDE_DIRS})
ENDIF(JPEG_FOUND)
IF(NETTLE_FOUND)
	TARGET_LINK_LIBRARIES(rpbase ${NETTLE_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(rpbase PRIVATE ${NETTLE_INCLUDE_DIRS})
ENDIF(NETTLE_FOUND)
IF(WIN32)
	# libwin32common
	TARGET_LINK_LIBRARIES(rpbase win32common)
	# Windows system libraries
	TARGET_LINK_LIBRARIES(rpbase gdiplus)
	IF(ENABLE_DECRYPTION)
		TARGET_LINK_LIBRARIES(rpbase advapi32)
	ENDIF(ENABLE_DECRYPTION)
ELSE(WIN32)
	# libunixcommon
	TARGET_LINK_LIBRARIES(rpbase unixcommon)
ENDIF(WIN32)
IF(CMAKE_THREAD_LIBS_INIT)
	TARGET_LINK_LIBRARIES(rpbase ${CMAKE_THREAD_LIBS_INIT})
ENDIF(CMAKE_THREAD_LIBS_INIT)

IF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rpbase i18n)
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
