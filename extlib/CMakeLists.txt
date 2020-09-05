PROJECT(extlib)

# Set common properties for extlib targets.
INCLUDE(SetMSVCDebugPath)
FUNCTION(SET_EXTLIB_PROPERTIES)
	FOREACH(_target ${ARGV})
		IF(TARGET ${_target})
			# Exclude from ALL builds.
			SET_TARGET_PROPERTIES(${_target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
		ENDIF(TARGET ${_target})
	ENDFOREACH(_target ${ARGV})
ENDFUNCTION(SET_EXTLIB_PROPERTIES)

# zlib: Assembly optimizations.
# TODO: Test on all archs and fix Z_PREFIX issues.
# TODO: Add more assembly files from contrib/ besides the
# default ones that were listed in zlib's CMakeLists.txt.
IF(CPU_i386 OR CPU_amd64)
	# i386/amd64. Check sizeof(void*) for the actual architecture,
	# since building 32-bit on 64-bit isn't considered "cross-compiling",
	# so CMAKE_SYSTEM_PROCESSOR might not be accurate.
	# NOTE: Checking CMAKE_CL_64 instead of sizeof(void*) for MSVC builds.
	IF(MSVC AND CMAKE_CL_64)
		SET(AMD64 "ON")
	ELSEIF(NOT MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
		SET(AMD64 "ON")
	ELSE()
		SET(ASM686 "ON")
	ENDIF()
ENDIF(CPU_i386 OR CPU_amd64)

# FIXME: zlib-1.2.9's asm optimizations break on 32-bit Windows...
# Reference:  https://github.com/madler/zlib/issues/41
UNSET(AMD64)
UNSET(ASM686)

# Don't install any of these libraries by default.
# zlib and libpng will be installed if they're
# built as shared libraries.
SET(SKIP_INSTALL_LIBRARIES ON)
SET(SKIP_INSTALL_HEADERS ON)
SET(SKIP_INSTALL_FILES ON)
SET(SKIP_INSTALL_ALL ON)
SET(BUILD_SHARED_LIBS OFF)

INCLUDE(DirInstallPaths)
SET(INSTALL_BIN_DIR "${DIR_INSTALL_DLL}")
SET(INSTALL_LIB_DIR "${DIR_INSTALL_LIB}")

IF(USE_INTERNAL_ZLIB)
	# Use the internal copy of zlib.
	# On Windows and Mac OS X, this will build DLLs.
	# On other systems, this will be statically-linked.
	IF(WIN32 OR APPLE)
		SET(SKIP_INSTALL_LIBRARIES OFF)
		SET(SKIP_INSTALL_ALL OFF)
		SET(BUILD_SHARED_LIBS ON)
		SET(BUILD_STATIC_LIBS OFF)
	ELSE()
		SET(SKIP_INSTALL_LIBRARIES ON)
		SET(SKIP_INSTALL_ALL ON)
		SET(BUILD_SHARED_LIBS OFF)
		SET(BUILD_STATIC_LIBS ON)
	ENDIF()
	ADD_SUBDIRECTORY(zlib)
	# NOTE: Cannot remove targets from all builds
	# if they're being installed.
	#SET_EXTLIB_PROPERTIES(zlib)
	SET_EXTLIB_PROPERTIES(zlibstatic)
ENDIF(USE_INTERNAL_ZLIB)

IF(USE_INTERNAL_PNG)
	IF(USE_INTERNAL_ZLIB)
		# Using internal ZLIB.
		# Don't search for ZLIB using FIND_PACKAGE().
		SET(PNG_BUILD_ZLIB ON)
	ENDIF(USE_INTERNAL_ZLIB)

	# Use the internal copy of libpng.
	# On Windows and Mac OS X, this will build DLLs.
	# On other systems, this will be statically-linked.
	IF(WIN32 OR APPLE)
		SET(PNG_SHARED ON)
		SET(PNG_STATIC OFF)
		SET(SKIP_INSTALL_LIBRARIES OFF)
		SET(SKIP_INSTALL_ALL OFF)
		SET(BUILD_SHARED_LIBS ON)
		SET(BUILD_STATIC_LIBS OFF)
	ELSE()
		SET(PNG_SHARED OFF)
		SET(PNG_STATIC ON)
		SET(SKIP_INSTALL_LIBRARIES ON)
		SET(SKIP_INSTALL_ALL ON)
		SET(BUILD_SHARED_LIBS OFF)
		SET(BUILD_STATIC_LIBS ON)
	ENDIF()
	ADD_SUBDIRECTORY(libpng)
	# NOTE: Cannot remove targets from all builds
	# if they're being installed.
	#SET_EXTLIB_PROPERTIES(png)
	SET_EXTLIB_PROPERTIES(png_static)
ENDIF(USE_INTERNAL_PNG)

IF(USE_INTERNAL_ZSTD)
	# Use the internal copy of zstd.
	# On Windows and Mac OS X, this will build DLLs.
	# On other systems, this will be statically-linked.
	IF(WIN32 OR APPLE)
		SET(SKIP_INSTALL_LIBRARIES ON)
		SET(SKIP_INSTALL_ALL OFF)
		SET(BUILD_SHARED_LIBS ON)
		SET(BUILD_STATIC_LIBS OFF)
	ELSE()
		SET(SKIP_INSTALL_LIBRARIES ON)
		SET(SKIP_INSTALL_ALL ON)
		SET(BUILD_SHARED_LIBS OFF)
		SET(BUILD_STATIC_LIBS ON)
	ENDIF()
	ADD_SUBDIRECTORY(zstd)
	# NOTE: Cannot remove targets from all builds
	# if they're being installed.
	#SET_EXTLIB_PROPERTIES(zstd)
	SET_EXTLIB_PROPERTIES(zstdstatic)
ENDIF(USE_INTERNAL_ZSTD)

IF(USE_INTERNAL_LZ4)
	# Use the internal copy of LZ4.
	# On Windows and Mac OS X, this will build DLLs.
	# On other systems, this will be statically-linked.
	IF(WIN32 OR APPLE)
		SET(SKIP_INSTALL_LIBRARIES ON)
		SET(SKIP_INSTALL_ALL OFF)
		SET(BUILD_SHARED_LIBS ON)
		SET(BUILD_STATIC_LIBS OFF)
	ELSE()
		SET(SKIP_INSTALL_LIBRARIES ON)
		SET(SKIP_INSTALL_ALL ON)
		SET(BUILD_SHARED_LIBS OFF)
		SET(BUILD_STATIC_LIBS ON)
	ENDIF()
	ADD_SUBDIRECTORY(lz4/contrib/cmake_unofficial)
	# NOTE: Cannot remove targets from all builds
	# if they're being installed.
	#SET_EXTLIB_PROPERTIES(lz4_shared)
	SET_EXTLIB_PROPERTIES(lz4_static)
	# lz4 programs that we don't need.
	SET_EXTLIB_PROPERTIES(lz4cli lz4c)
ENDIF(USE_INTERNAL_LZ4)

# MiniZip
SET(SKIP_INSTALL_LIBRARIES ON)
SET(SKIP_INSTALL_ALL ON)
SET(BUILD_SHARED_LIBS OFF)
SET(BUILD_STATIC_LIBS ON)
SET(MZ_COMPAT ON)
SET(MZ_ZLIB ON)
SET(MZ_BZIP2 OFF)
SET(MZ_LZMA OFF)
SET(MZ_ZSTD ${ENABLE_ZSTD})
SET(MZ_PKCRYPT OFF)
SET(MZ_WZAES OFF)
SET(MZ_LIBCOMP OFF)
SET(MZ_OPENSSL OFF)
SET(MZ_BRG OFF)
SET(MZ_SIGNING OFF)
SET(MZ_COMPRESS_ONLY OFF)
SET(MZ_DECOMPRESS_ONLY OFF)
SET(MZ_BUILD_TEST ON)	# Allow manually building test executables.
SET(MZ_BUILD_UNIT_TEST OFF)
SET(MZ_BUILD_FUZZ_TEST OFF)
SET(MZ_CODE_COVERAGE OFF)
SET(MZ_FILE32_API OFF)
SET(ZLIB_FORCE_FETCH OFF)
SET(ZSTD_FORCE_FETCH OFF)
ADD_SUBDIRECTORY(minizip)
SET_EXTLIB_PROPERTIES(minizip)
SET_EXTLIB_PROPERTIES(minigzip_cmd)
SET_EXTLIB_PROPERTIES(minizip_cmd)
SET_EXTLIB_PROPERTIES(test_cmd)
UNSET(USE_COMPAT)
UNSET(COMPRESS_ONLY)
UNSET(DECOMPRESS_ONLY)

# inih
SET(SKIP_INSTALL_LIBRARIES ON)
SET(SKIP_INSTALL_ALL ON)
SET(BUILD_SHARED_LIBS OFF)
SET(BUILD_STATIC_LIBS ON)
ADD_SUBDIRECTORY(inih)
SET_EXTLIB_PROPERTIES(inih)

# TinyXML2
IF(USE_INTERNAL_XML)
	# Use the internal copy of TinyXML2.
	# On Windows, this will build DLLs.
	# On other systems, this will be statically-linked.
	# TODO: Build a dylib for Mac OS X.
	IF(WIN32)
		SET(SKIP_INSTALL_LIBRARIES OFF)
		SET(SKIP_INSTALL_ALL OFF)
		SET(BUILD_STATIC_LIBS OFF)
		SET(BUILD_SHARED_LIBS ON)
	ELSE()
		SET(SKIP_INSTALL_LIBRARIES ON)
		SET(SKIP_INSTALL_ALL ON)
		SET(BUILD_STATIC_LIBS ON)
		SET(BUILD_SHARED_LIBS OFF)
	ENDIF()

	# Build TinyXML2.
	ADD_SUBDIRECTORY(tinyxml2)

	# NOTE: Cannot remove targets from all builds
	# if they're being installed.
	SET_EXTLIB_PROPERTIES(tinyxml2_static)
	#SET_EXTLIB_PROPERTIES(tinyxml2)

	# TODO: Set TinyXML2::tinyxml2 and use it in libromdata?
ENDIF(USE_INTERNAL_XML)

# UnICE68 for Atari ST SNDH files.
IF(ENABLE_UNICE68)
	SET(BUILD_UNICE68_CLI ON) # only builds if unice68_exe is explicitly built
	SET(BUILD_STATIC_LIBS ON)
	SET(BUILD_SHARED_LIBS OFF)
	ADD_SUBDIRECTORY(unice68)
	SET_EXTLIB_PROPERTIES(unice68_lib)
	SET_EXTLIB_PROPERTIES(unice68_exe)
ENDIF(ENABLE_UNICE68)

# libmspack-xenia for LZX-compressed Xbox 360 executables.
IF(ENABLE_LIBMSPACK)
	SET(BUILD_STATIC_LIBS ON)
	SET(BUILD_SHARED_LIBS OFF)
	ADD_SUBDIRECTORY(libmspack-xenia)
	SET_EXTLIB_PROPERTIES(libmspack)
ENDIF(ENABLE_LIBMSPACK)

# PowerVR texture decompression.
IF(ENABLE_PVRTC)
	SET(BUILD_STATIC_LIBS ON)
	SET(BUILD_SHARED_LIBS OFF)
	ADD_SUBDIRECTORY(PowerVR)
	SET_EXTLIB_PROPERTIES(pvrtc)
ENDIF(ENABLE_PVRTC)

# Compatibility library for older versions of Windows.
IF(ENABLE_OLDWINCOMPAT)
	SET(BUILD_STATIC_LIBS ON)
	SET(BUILD_SHARED_LIBS OFF)
	ADD_SUBDIRECTORY(oldwincompat)
	SET_EXTLIB_PROPERTIES(oldwincompat)
ENDIF(ENABLE_OLDWINCOMPAT)

# Google Test
IF(BUILD_TESTING)
	# Reference: http://stackoverflow.com/questions/12540970/how-to-make-gtest-build-mdd-instead-of-mtd-by-default-using-cmake
	SET(gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll")
	SET(SKIP_INSTALL_LIBRARIES ON)
	SET(SKIP_INSTALL_ALL ON)

	# Use shared libraries on Windows.
	IF(WIN32)
		SET(BUILD_STATIC_LIBS OFF)
		SET(BUILD_SHARED_LIBS ON)
	ELSE(WIN32)
		SET(BUILD_STATIC_LIBS ON)
		SET(BUILD_SHARED_LIBS OFF)
	ENDIF(WIN32)

	ADD_SUBDIRECTORY(googletest)
	INCLUDE(SetMSVCDebugPath)
	SET_EXTLIB_PROPERTIES(
		gtest gtest_main gtest_no_exception gtest_main_no_exception
		gtest_main_no_rtti gtest_dll gtest_main_use_own_tuple
		gmock gmock_main gmock_main_no_exception
		gmock_main_no_rtti gmock_main_use_own_tuple
		)
	IF(WIN32)
		# GTest is a DLL, so we need to set this.
		TARGET_COMPILE_DEFINITIONS(gtest INTERFACE GTEST_LINKED_AS_SHARED_LIBRARY=1)
		TARGET_COMPILE_DEFINITIONS(gtest_main INTERFACE GTEST_LINKED_AS_SHARED_LIBRARY=1)
	ENDIF(WIN32)
ENDIF(BUILD_TESTING)
