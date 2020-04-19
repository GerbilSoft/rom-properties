# Build options.

# Platform options.
# NOTE: If a platform is specified but it isn't found,
# that plugin will not be built. There doesn't seem to
# be a way to have "yes, no, auto" like in autotools.

# NOTE: OPTION() only supports BOOL values.
# Reference: https://cmake.org/pipermail/cmake/2016-October/064342.html
MACRO(OPTION_UI _pkg _desc)
	SET(BUILD_${_pkg} AUTO CACHE STRING "${_desc}")
	SET_PROPERTY(CACHE BUILD_${_pkg} PROPERTY STRINGS AUTO ON OFF)

	IF(BUILD_${_pkg} STREQUAL "AUTO")
		SET(REQUIRE_${_pkg} "")
	ELSEIF(BUILD_${_pkg})
		SET(REQUIRE_${_pkg} "REQUIRED")
	ELSE()
		SET(REQUIRE_${_pkg} "")
	ENDIF()
ENDMACRO(OPTION_UI)

IF(UNIX AND NOT APPLE)
	# NOTE: OPTION() only supports BOOL values.
	# Reference: https://cmake.org/pipermail/cmake/2016-October/064342.html
	OPTION_UI(KDE4 "Build the KDE4 plugin.")
	OPTION_UI(KF5 "Build the KDE Frameworks 5 plugin.")
	OPTION_UI(XFCE "Build the XFCE (GTK+ 2.x) plugin. (Thunar 1.7 and earlier)")
	OPTION_UI(XFCE3 "Build the XFCE (GTK+ 3.x) plugin. (Thunar 1.8 and later)")
	OPTION_UI(GNOME "Build the GNOME (GTK+ 3.x) plugin. (Also MATE and Cinnamon)")

	# Set BUILD_GTK2 and/or BUILD_GTK3 depending on frontends.
	SET(BUILD_GTK2 ${BUILD_XFCE} CACHE INTERNAL "Check for GTK+ 2.x." FORCE)
	IF(BUILD_GNOME OR BUILD_XFCE3)
		SET(BUILD_GTK3 ON CACHE INTERNAL "Check for GTK+ 3.x." FORCE)
	ELSE()
		SET(BUILD_GTK3 OFF CACHE INTERNAL "Check for GTK+ 3.x." FORCE)
	ENDIF()

	# QT_SELECT must be unset before compiling.
	UNSET(ENV{QT_SELECT})
ELSEIF(WIN32)
	SET(BUILD_WIN32 ON)
ENDIF()

OPTION(BUILD_CLI "Build the `rpcli` command line program." ON)

# ZLIB, libpng
# Internal versions are always used on Windows.
IF(WIN32)
	SET(USE_INTERNAL_ZLIB ON)
	SET(USE_INTERNAL_PNG ON)
	OPTION(ENABLE_XML "Enable XML parsing for e.g. Windows manifests." ON)
	SET(USE_INTERNAL_XML ON)
ELSE(WIN32)
	OPTION(USE_INTERNAL_ZLIB "Use the internal copy of zlib." OFF)
	OPTION(USE_INTERNAL_PNG "Use the internal copy of libpng." OFF)
	OPTION(ENABLE_XML "Enable XML parsing for e.g. Windows manifests." ON)
	OPTION(USE_INTERNAL_XML "Use the internal copy of TinyXML2." OFF)
ENDIF()

# TODO: If APNG export is added, verify that system libpng
# supports APNG.

# JPEG (gdiplus on Windows, libjpeg[-turbo] on other platforms)
IF(WIN32)
	SET(ENABLE_JPEG ON)
ELSE(WIN32)
	OPTION(ENABLE_JPEG "Enable JPEG decoding using libjpeg." ON)
ENDIF(WIN32)

# Enable decryption for newer ROM and disc images.
# TODO: Tri-state like UI frontends.
OPTION(ENABLE_DECRYPTION "Enable decryption for newer ROM and disc images." ON)

# Enable UnICE68 for Atari ST SNDH files. (GPLv3)
OPTION(ENABLE_UNICE68 "Enable UnICE68 for Atari ST SNDH files. (GPLv3)" ON)

# Enable libmspack-xenia for Xbox 360 executables.
OPTION(ENABLE_LIBMSPACK "Enable libmspack-xenia for Xbox 360 executables." ON)

# Enable the PowerVR Native SDK subset for PVRTC decompression.
OPTION(ENABLE_PVRTC "Enable the PowerVR Native SDK subset for PVRTC decompression." ON)

# Enable precompiled headers.
# FIXME: Not working properly on older gcc. Use cmake-3.16.0's built-in PCH?
IF(MSVC)
	SET(PCH_DEFAULT ON)
ELSE()
	SET(PCH_DEFAULT OFF)
ENDIF()
OPTION(ENABLE_PCH "Enable precompiled headers for faster builds." ${PCH_DEFAULT})

# Link-time optimization.
# FIXME: Not working in clang builds and Ubuntu's gcc...
IF(MSVC)
	SET(LTO_DEFAULT ON)
ELSE()
	SET(LTO_DEFAULT OFF)
ENDIF()
OPTION(ENABLE_LTO "Enable link-time optimization in release builds." ${LTO_DEFAULT})

# Split debug information into a separate file.
OPTION(SPLIT_DEBUG "Split debug information into a separate file." ON)

# Install the split debug file.
OPTION(INSTALL_DEBUG "Install the split debug files." ON)
IF(INSTALL_DEBUG AND NOT SPLIT_DEBUG)
	# Cannot install debug files if we're not splitting them.
	SET(INSTALL_DEBUG OFF CACHE INTERNAL "Install the split debug files." FORCE)
ENDIF(INSTALL_DEBUG AND NOT SPLIT_DEBUG)

# Enable coverage checking. (gcc/clang only)
OPTION(ENABLE_COVERAGE "Enable code coverage checking. (gcc/clang only)" OFF)

# Enable NLS. (internationalization)
OPTION(ENABLE_NLS "Enable NLS using gettext for localized messages." ON)

IF(WIN32 AND MSVC)
	# Enable compatibility with older Windows.
	OPTION(ENABLE_OLDWINCOMPAT "Enable compatibility with Windows 2000 with MSVC 2010+. (forces static CRT)" OFF)
ELSE(WIN32 AND MSVC)
	SET(ENABLE_OLDWINCOMPAT OFF)
ENDIF(WIN32 AND MSVC)

# Linux security options.
IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	OPTION(INSTALL_APPARMOR "Install AppArmor profiles." ON)
ELSE(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	SET(INSTALL_APPARMOR OFF)
ENDIF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
