PROJECT(extlib)

# Set common properties for extlib targets.
INCLUDE(SetMSVCDebugPath)
FUNCTION(SET_EXTLIB_PROPERTIES)
	FOREACH(_target ${ARGV})
		IF(TARGET ${_target})
			# Set the MSVC debug path.
			SET_MSVC_DEBUG_PATH(${_target})
			# Exclude from ALL builds.
			SET_TARGET_PROPERTIES(${_target} PROPERTIES
				EXCLUDE_FROM_ALL TRUE
				)
		ENDIF(TARGET ${_target})
	ENDFOREACH(_target ${ARGV})
ENDFUNCTION(SET_EXTLIB_PROPERTIES)

# zlib: Assembly optimizations.
# TODO: Test on all archs and fix Z_PREFIX issues.
# TODO: Add more assembly files from contrib/ besides the
# default ones that were listed in zlib's CMakeLists.txt.
# FIXME: Broken on AppVeyor. (MSVC 2015)
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

# Don't install any of these libraries.
SET(OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
SET(OLD_SKIP_INSTALL_LIBRARIES ${SKIP_INSTALL_LIBRARIES})
SET(OLD_SKIP_INSTALL_HEADERS ${SKIP_INSTALL_HEADERS})
SET(OLD_SKIP_INSTALL_FILES ${SKIP_INSTALL_FILES})
SET(OLD_SKIP_INSTALL_ALL ${SKIP_INSTALL_ALL})
SET(BUILD_SHARED_LIBS OFF)
SET(SKIP_INSTALL_LIBRARIES ON)
SET(SKIP_INSTALL_HEADERS ON)
SET(SKIP_INSTALL_FILES ON)
SET(SKIP_INSTALL_ALL ON)

# FIXME: Build static libraries with -fpic/-fPIC on Linux.

IF(USE_INTERNAL_ZLIB)
	# Use the internal copy of zlib.
	# On Windows and Mac OS X, this will build DLLs.
	# On other systems, this will be statically-linked.
	ADD_SUBDIRECTORY(zlib)
	SET_EXTLIB_PROPERTIES(zlib)
	SET_EXTLIB_PROPERTIES(zlibstatic)
ENDIF(USE_INTERNAL_ZLIB)

IF(USE_INTERNAL_PNG)
	# Use the internal copy of libpng.
	# On Windows and Mac OS X, this will build DLLs.
	# On other systems, this will be statically-linked.
	IF(WIN32 OR APPLE)
		SET(PNG_SHARED ON)
		SET(PNG_STATIC OFF)
	ELSE()
		SET(PNG_SHARED ON)
		SET(PNG_STATIC OFF)
	ENDIF()
	ADD_SUBDIRECTORY(libpng)
	SET_EXTLIB_PROPERTIES(png)
	SET_EXTLIB_PROPERTIES(png_static)
ENDIF(USE_INTERNAL_PNG)

# MiniZip.
ADD_SUBDIRECTORY(minizip)

# Google Test
IF(BUILD_TESTING)
	# Reference: http://stackoverflow.com/questions/12540970/how-to-make-gtest-build-mdd-instead-of-mtd-by-default-using-cmake
	SET(gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll")
	ADD_SUBDIRECTORY(googletest)

	INCLUDE(SetMSVCDebugPath)
	SET_EXTLIB_PROPERTIES(
		gtest gtest_main gtest_no_exception gtest_main_no_exception
		gtest_main_no_rtti gtest_dll gtest_main_use_own_tuple
		gmock gmock_main gmock_main_no_exception
		gmock_main_no_rtti gmock_main_use_own_tuple
		)
ENDIF(BUILD_TESTING)

# FIXME: Probably unnecessary, since these variables
# only apply to the extlib/ subdirectory.
SET(BUILD_SHARED_LIBS ${OLD_BUILD_SHARED_LIBS})
SET(SKIP_INSTALL_LIBRARIES ${OLD_SKIP_INSTALL_LIBRARIES})
SET(SKIP_INSTALL_HEADERS ${OLD_SKIP_INSTALL_HEADERS})
SET(SKIP_INSTALL_FILES ${OLD_SKIP_INSTALL_FILES})
SET(SKIP_INSTALL_ALL ${OLD_SKIP_INSTALL_ALL})
UNSET(OLD_BUILD_SHARED_LIBS)
UNSET(OLD_SKIP_INSTALL_LIBRARIES)
UNSET(OLD_SKIP_INSTALL_HEADERS)
UNSET(OLD_SKIP_INSTALL_FILES)
UNSET(OLD_SKIP_INSTALL_ALL)
