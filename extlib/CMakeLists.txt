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
STRING(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" arch)
IF(arch MATCHES "^(i.|x)86$|^x86_64$|^amd64$")
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
ENDIF(arch MATCHES "^(i.|x)86$|^x86_64$|^amd64$")
UNSET(arch)

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
	ELSE()
		SET(SKIP_INSTALL_LIBRARIES ON)
		SET(SKIP_INSTALL_ALL ON)
		SET(BUILD_SHARED_LIBS OFF)
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
	ELSE()
		SET(PNG_SHARED OFF)
		SET(PNG_STATIC ON)
		SET(SKIP_INSTALL_LIBRARIES ON)
		SET(SKIP_INSTALL_ALL ON)
		SET(BUILD_SHARED_LIBS OFF)
	ENDIF()
	ADD_SUBDIRECTORY(libpng)
	# NOTE: Cannot remove targets from all builds
	# if they're being installed.
	#SET_EXTLIB_PROPERTIES(png)
	SET_EXTLIB_PROPERTIES(png_static)
ENDIF(USE_INTERNAL_PNG)

IF(USE_INTERNAL_JPEG)
	# Use the internal copy of libjpeg-turbo.
	# On Windows and Mac OS X, this will build DLLs.
	# On other systems, this will be statically-linked.
	# FIXME: libjpeg-turbo's CMakeLists.txt only officially supports Windows.
	IF(WIN32 OR APPLE)
		SET(ENABLE_SHARED ON)
		SET(ENABLE_STATIC OFF)
		SET(SKIP_INSTALL_LIBRARIES OFF)
		SET(SKIP_INSTALL_ALL OFF)
		SET(BUILD_SHARED_LIBS ON)
	ELSE()
		SET(ENABLE_SHARED OFF)
		SET(ENABLE_STATIC ON)
		SET(SKIP_INSTALL_LIBRARIES ON)
		SET(SKIP_INSTALL_ALL ON)
		SET(BUILD_SHARED_LIBS OFF)
	ENDIF()

	# libjpeg-turbo options.
	SET(WITH_SIMD TRUE)
	SET(WITH_ARITH_ENC TRUE)
	SET(WITH_ARITH_DEC TRUE)
	SET(WITH_JPEG7 FALSE)
	SET(WITH_JPEG8 FALSE)
	SET(WITH_MEM_SRCDST TRUE)
	SET(WITH_TURBOJPEG FALSE)
	SET(WITH_JAVA FALSE)
	SET(WITH_12BIT FALSE)

	# Build libjpeg-turbo.
	ADD_SUBDIRECTORY(libjpeg-turbo)

	# NOTE: Cannot remove targets from all builds
	# if they're being installed.
	#SET_EXTLIB_PROPERTIES(jpeg)
	SET_EXTLIB_PROPERTIES(jpeg-static)
	#SET_EXTLIB_PROPERTIES(turbojpeg)
	SET_EXTLIB_PROPERTIES(turbojpeg-static)
ENDIF(USE_INTERNAL_JPEG)

# MiniZip.
SET(SKIP_INSTALL_LIBRARIES ON)
SET(SKIP_INSTALL_ALL ON)
SET(BUILD_SHARED_LIBS OFF)
ADD_SUBDIRECTORY(minizip)

# Google Test
IF(BUILD_TESTING)
	# Reference: http://stackoverflow.com/questions/12540970/how-to-make-gtest-build-mdd-instead-of-mtd-by-default-using-cmake
	SET(gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll")
	SET(SKIP_INSTALL_LIBRARIES ON)
	SET(SKIP_INSTALL_ALL ON)
	ADD_SUBDIRECTORY(googletest)

	INCLUDE(SetMSVCDebugPath)
	SET_EXTLIB_PROPERTIES(
		gtest gtest_main gtest_no_exception gtest_main_no_exception
		gtest_main_no_rtti gtest_dll gtest_main_use_own_tuple
		gmock gmock_main gmock_main_no_exception
		gmock_main_no_rtti gmock_main_use_own_tuple
		)
ENDIF(BUILD_TESTING)
