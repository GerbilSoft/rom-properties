CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(src)

# Check for Large File Support.
# NOTE: This should have been done in platform.cmake, but we're
# including it here because the off_t/off64_t sizes are written
# to config.libc.h.
INCLUDE(CheckLargeFileSupport)
CHECK_LARGE_FILE_SUPPORT()

# Check for dirent.h. (for d_type.h)
CHECK_INCLUDE_FILE("dirent.h" HAVE_DIRENT_H)
# Check for sys/stat.h. (for d_type.h)
# Windows Universal CRT has this; older versions of MSVCRT do not.
CHECK_INCLUDE_FILE("sys/stat.h" HAVE_SYS_STAT_H)

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
	CHECK_SYMBOL_EXISTS_OR_INLINE(_mkgmtime32 "time.h" "struct tm tm; time_t x = _mkgmtime(&tm);" HAVE__MKGMTIME32)
	CHECK_SYMBOL_EXISTS_OR_INLINE(_mkgmtime64 "time.h" "struct tm tm; time_t x = _mkgmtime64(&tm);" HAVE__MKGMTIME64)
ENDIF(NOT HAVE_TIMEGM)
IF(NOT HAVE_TIMEGM AND NOT HAVE__MKGMTIME AND NOT HAVE__MKGMTIME32 AND NOT HAVE__MKGMTIME64)
	MESSAGE(FATAL_ERROR "timegm() or equivalent function not found.")
ENDIF(NOT HAVE_TIMEGM AND NOT HAVE__MKGMTIME AND NOT HAVE__MKGMTIME32 AND NOT HAVE__MKGMTIME64)

# Write the libc configuration file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.libc.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.libc.h")

# C++11 compatibility header.
# NOTE: This must be included regardless of C++11 support in the compiler.
# gcc-4.6 supports some C++11, but is missing explicit virtual overrides.
IF(MSVC)
	SET(RP_CONFIG_LIBC_H "-FI${CMAKE_CURRENT_BINARY_DIR}/config.libc.h")
	SET(RP_CXX11_COMPAT_H "-FI${CMAKE_CURRENT_SOURCE_DIR}/c++11-compat.h")
ELSE(MSVC)
	SET(RP_CONFIG_LIBC_H "-include ${CMAKE_CURRENT_BINARY_DIR}/config.libc.h")
	SET(RP_CXX11_COMPAT_H "-include ${CMAKE_CURRENT_SOURCE_DIR}/c++11-compat.h")
ENDIF(MSVC)

# Add the C++11 compatibility header to CFLAGS/CXXFLAGS.
SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} ${RP_CONFIG_LIBC_H} ${RP_CXX11_COMPAT_H}")
SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} ${RP_CONFIG_LIBC_H} ${RP_CXX11_COMPAT_H}")

# Check for hidden visibility flags.
INCLUDE(CheckHiddenVisibility)
CHECK_HIDDEN_VISIBILITY()

# Check for stdbool.h.
INCLUDE(CheckIncludeFile)
CHECK_INCLUDE_FILE("stdbool.h" HAVE_STDBOOL_H)

# glib-2.32 (released 2012/03/24) added GResource and GMenuModel.
# glib-2.34 (released 2012/09/23) added g_clear_object().
# Ubuntu 14.04 has glib-2.40.
SET(GLIB_MIN_VERSION 2.34.0)

# Write the version number to config.version.h.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.version.h")

# Write stdbool.h.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/stdboolx.h.in" "${CMAKE_CURRENT_BINARY_DIR}/stdboolx.h")

# Build certain libraries as shared libraries on Linux.
IF(UNIX AND NOT APPLE)
	SET(RP_LIBROMDATA_IS_DLL 1)
	ADD_DEFINITIONS(-DRP_LIBROMDATA_IS_DLL=1)
ENDIF(UNIX AND NOT APPLE)

# Source Code subdirectories.
ADD_SUBDIRECTORY(librpsecure)
ADD_SUBDIRECTORY(libi18n)
ADD_SUBDIRECTORY(librpthreads)
ADD_SUBDIRECTORY(libcachecommon)
ADD_SUBDIRECTORY(rp-download)
ADD_SUBDIRECTORY(librpcpu)
ADD_SUBDIRECTORY(librpfile)
ADD_SUBDIRECTORY(librpbase)
ADD_SUBDIRECTORY(librptexture)
ADD_SUBDIRECTORY(amiibo-data)
ADD_SUBDIRECTORY(libromdata)

IF(BUILD_CLI)
	ADD_SUBDIRECTORY(rpcli)
ENDIF(BUILD_CLI)

IF(UNIX AND NOT APPLE)
	IF(BUILD_KDE4 OR BUILD_KF5)
		ADD_SUBDIRECTORY(kde)
	ENDIF(BUILD_KDE4 OR BUILD_KF5)
	IF(BUILD_GTK2 OR BUILD_GTK3)
		ADD_SUBDIRECTORY(res)
		ADD_SUBDIRECTORY(gtk)
	ENDIF(BUILD_GTK2 OR BUILD_GTK3)
	ADD_SUBDIRECTORY(rp-stub)
ELSEIF(WIN32)
	IF(BUILD_WIN32)
		ADD_SUBDIRECTORY(win32)
		ADD_SUBDIRECTORY(svrplus)
	ENDIF(BUILD_WIN32)
ENDIF()

# NOTE: libunixcommon must be *after* the UI frontends.
# Otherwise, the UI frontend checks won't work right, and on
# Unix systems, the qtpaths check will fail if Qt5 isn't
# installed and KF5 isn't explicitly disabled.
IF(WIN32)
	ADD_SUBDIRECTORY(libwin32common)
ELSE(WIN32)
	ADD_SUBDIRECTORY(libunixcommon)
ENDIF(WIN32)
