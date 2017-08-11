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
			CHECK_SYMBOL_EXISTS(aes128_set_decrypt_key "nettle/aes.h" HAVE_NETTLE_3)
			IF(HAVE_NETTLE_3)
				# Check for Nettle versioning symbols.
				# Nettle 3.1 added version.h.
				SET(OLD_CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES}")
				SET(OLD_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
				SET(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES} ${NETTLE_INCLUDE_DIRS}")
				SET(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES} ${NETTLE_LIBRARY}")
				CHECK_SYMBOL_EXISTS(NETTLE_VERSION_MAJOR "nettle/version.h" HAVE_NETTLE_VERSION_H)
				CHECK_SYMBOL_EXISTS(nettle_version_major "nettle/version.h" HAVE_NETTLE_VERSION_FUNCTIONS)
				SET(CMAKE_REQUIRED_INCLUDES "${OLD_CMAKE_REQUIRED_INCLUDES}")
				SET(CMAKE_REQUIRED_LIBRARIES "${OLD_CMAKE_REQUIRED_LIBRARIES}")
				UNSET(OLD_CMAKE_REQUIRED_INCLUDES)
				UNSET(OLD_CMAKE_REQUIRED_LIBRARIES)
			ENDIF(HAVE_NETTLE_3)
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

# Check if non-capturing lambda functions can be cast to function pointers.
# NOTE: Used by libromdata, not librpbase, but we're
# keeping the check here because libromdata doesn't
# have any other unique checks.
INCLUDE(CheckCXX11LambdaAsFunctionPointer)
CHECK_CXX11_LAMBDA_AS_FUNCTION_POINTER(HAVE_LAMBDA_AS_FUNCTION_POINTER)

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
	TextFuncs_utf8.hpp
	TextFuncs_wchar.hpp
	RomData.hpp
	RomData_p.hpp
	RomFields.hpp
	SystemRegion.hpp
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
	img/un-premultiply.hpp
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

	SET(libromdata_OPTIMIZE_SRCS
		img/RpJpeg_ssse3.cpp
		img/ImageDecoder_Linear_ssse3.cpp
		)

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSSE3_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		SET(SSSE3_FLAG "-mssse3")
	ENDIF()
	IF(SSSE3_FLAG)
		FOREACH(ssse3_file ${libromdata_OPTIMIZE_SRCS})
			SET_SOURCE_FILES_PROPERTIES(${ssse3_file}
				APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
		ENDFOREACH(ssse3_file ${libromdata_OPTIMIZE_SRCS})
	ENDIF(SSSE3_FLAG)
ENDIF()
UNSET(arch)

# Aligned malloc() functions.
# NOTE: MinGW-w64 only has _aligned_malloc().
# It doesn't even have inline function wrappers
# for the other functions.
CHECK_FUNCTION_EXISTS(_aligned_malloc	HAVE_MSVC_ALIGNED_MALLOC)
CHECK_FUNCTION_EXISTS(aligned_alloc	HAVE_ALIGNED_ALLOC)
CHECK_FUNCTION_EXISTS(posix_memalign	HAVE_POSIX_MEMALIGN)
CHECK_FUNCTION_EXISTS(memalign		HAVE_MEMALIGN)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.librpbase.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.librpbase.h")

######################
# Build the library. #
######################

# UTF-8 version.
ADD_LIBRARY(rpbase8 STATIC
	${librpbase_SRCS} ${librpbase_H}
	${librpbase_OS_SRCS} ${librpbase_OS_H}
	${librpbase_CRYPTO_SRCS} ${librpbase_CRYPTO_H}
	${librpbase_CRYPTO_OS_SRCS} ${librpbase_CRYPTO_OS_H}
	${librpbase_THREAD_SRCS} ${librpbase_THREAD_H}
	${librpbase_CPU_SRCS} ${librpbase_CPU_H}
	${libromdata_OPTIMIZE_SRCS}
	)
TARGET_COMPILE_DEFINITIONS(rpbase8 PUBLIC -DRP_UTF8)

# UTF-16 version.
ADD_LIBRARY(rpbase16 STATIC
	${librpbase_SRCS} ${librpbase_H}
	${librpbase_OS_SRCS} ${librpbase_OS_H}
	${librpbase_CRYPTO_SRCS} ${librpbase_CRYPTO_H}
	${librpbase_CRYPTO_OS_SRCS} ${librpbase_CRYPTO_OS_H}
	${librpbase_THREAD_SRCS} ${librpbase_THREAD_H}
	${librpbase_CPU_SRCS} ${librpbase_CPU_H}
	${libromdata_OPTIMIZE_SRCS}
	)
TARGET_COMPILE_DEFINITIONS(rpbase16 PUBLIC -DRP_UTF16)

# Common properties for rpbase8 and rpbase16.
FOREACH(_target rpbase8 rpbase16)
	# Include paths:
	# - Public: Current source and binary directories.
	# - Private: Parent source and binary directories,
	#            and top-level binary directory for git_version.h.
	TARGET_INCLUDE_DIRECTORIES(${_target}
		PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# librpbase
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# librpbase
		PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
		)
	IF(MSVC)
		TARGET_LINK_LIBRARIES(${_target} delayimp)
	ENDIF(MSVC)
	# Exclude from ALL builds.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
	# Make sure git_version.h is created before compiling this target.
	IF(TARGET git_version)
		ADD_DEPENDENCIES(${_target} git_version)
	ENDIF(TARGET git_version)

	# Link in libdl if it's required for dlopen()
	# and we have a component that uses it.
	IF(librpbase_NEEDS_DL AND CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(${_target} ${CMAKE_DL_LIBS})
	ENDIF(librpbase_NEEDS_DL AND CMAKE_DL_LIBS)

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
	IF(WIN32)
		# libwin32common
		TARGET_LINK_LIBRARIES(${_target} win32common)
		# Windows system libraries
		TARGET_LINK_LIBRARIES(${_target} gdiplus)
		IF(ENABLE_DECRYPTION)
			TARGET_LINK_LIBRARIES(${_target} advapi32)
		ENDIF(ENABLE_DECRYPTION)
	ELSE(WIN32)
		# libunixcommon
		TARGET_LINK_LIBRARIES(${_target} unixcommon)
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
