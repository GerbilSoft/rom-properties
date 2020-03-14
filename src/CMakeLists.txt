PROJECT(src)

# Check for Large File Support.
# NOTE: This should have been done in platform.cmake, but we're
# including it here because the off_t/off64_t sizes are written
# to config.libc.h.
INCLUDE(CheckLargeFileSupport)
CHECK_LARGE_FILE_SUPPORT()

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

# Write the version number to config.version.h.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.version.h")

# Write stdbool.h.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/stdboolx.h.in" "${CMAKE_CURRENT_BINARY_DIR}/stdboolx.h")

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
