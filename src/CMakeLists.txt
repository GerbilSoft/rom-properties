PROJECT(src)

# C++11 compatibility header.
# NOTE: This must be included regardless of C++11 support in the compiler.
# gcc-4.6 supports some C++11, but is missing explicit virtual overrides.
IF(MSVC)
	SET(RP_CXX11_COMPAT_HEADER "-FI${CMAKE_CURRENT_SOURCE_DIR}/c++11-compat.h")
ELSE(MSVC)
	SET(RP_CXX11_COMPAT_HEADER "-include ${CMAKE_CURRENT_SOURCE_DIR}/c++11-compat.h")
ENDIF(MSVC)

# Add the C++11 compatibility header to CFLAGS/CXXFLAGS.
SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} ${RP_CXX11_COMPAT_HEADER}")
SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} ${RP_CXX11_COMPAT_HEADER}")

# Check for hidden visibility flags.
INCLUDE(CheckHiddenVisibility)
CHECK_HIDDEN_VISIBILITY()

# Write the version number to config.version.h.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.version.h")

# Source Code subdirectories.
IF(WIN32)
	ADD_SUBDIRECTORY(libwin32common)
ENDIF(WIN32)
ADD_SUBDIRECTORY(librpbase)
ADD_SUBDIRECTORY(libromdata)
ADD_SUBDIRECTORY(libcachemgr)

IF(BUILD_CLI)
	ADD_SUBDIRECTORY(rpcli)
ENDIF(BUILD_CLI)

IF(UNIX AND NOT APPLE)
	IF(BUILD_KDE4 OR BUILD_KDE5)
		ADD_SUBDIRECTORY(kde)
	ENDIF(BUILD_KDE4 OR BUILD_KDE5)
	IF(BUILD_GTK2 OR BUILD_GTK3)
		ADD_SUBDIRECTORY(gtk)
	ENDIF(BUILD_GTK2 OR BUILD_GTK3)
ELSEIF(WIN32)
	IF(BUILD_WIN32)
		ADD_SUBDIRECTORY(win32)
		ADD_SUBDIRECTORY(svrplus)
	ENDIF(BUILD_WIN32)
ENDIF()
