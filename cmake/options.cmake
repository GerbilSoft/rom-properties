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
	OPTION_UI(KF6 "Build the KDE Frameworks 6 plugin. (EXPERIMENTAL)")
	OPTION_UI(XFCE "Build the XFCE (GTK+ 2.x) plugin. (Thunar 1.7 and earlier)")
	OPTION_UI(GTK3 "Build the GTK+ 3.x plugin.")
	OPTION_UI(GTK4 "Build the GTK 4.x plugin. (EXPERIMENTAL)")

	# QT_SELECT must be unset before compiling.
	UNSET(ENV{QT_SELECT})
ELSEIF(WIN32)
	SET(BUILD_WIN32 ON)
ENDIF()

OPTION(BUILD_CLI "Build the `rpcli` command line program." ON)

# ZLIB, libpng, XML, zstd
# Internal versions are always used on Windows.
OPTION(ENABLE_XML "Enable XML parsing for e.g. Windows manifests." ON)
OPTION(ENABLE_ZSTD "Enable ZSTD decompression. (Required for some unit tests.)" ON)
OPTION(ENABLE_LZ4 "Enable LZ4 decompression. (Required for some PSP disc formats.)" ON)
OPTION(ENABLE_LZO "Enable LZO decompression. (Required for some PSP disc formats.)" ON)

IF(WIN32)
	SET(USE_INTERNAL_ZLIB ON)
	SET(USE_INTERNAL_PNG ON)
	SET(USE_INTERNAL_XML ${ENABLE_XML})
	SET(USE_INTERNAL_ZSTD ${ENABLE_ZSTD})
	SET(USE_INTERNAL_LZ4 ${ENABLE_LZ4})
	SET(USE_INTERNAL_LZO ${ENABLE_LZO})
ELSE(WIN32)
	OPTION(USE_INTERNAL_ZLIB "Use the internal copy of zlib." OFF)
	OPTION(USE_INTERNAL_PNG "Use the internal copy of libpng." OFF)
	OPTION(USE_INTERNAL_XML "Use the internal copy of TinyXML2." OFF)
	OPTION(USE_INTERNAL_ZSTD "Use the internal copy of zstd." OFF)
	OPTION(USE_INTERNAL_LZ4 "Use the internal copy of LZ4." OFF)
	OPTION(USE_INTERNAL_LZO "Use the internal copy of LZO." OFF)
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

# Enable the ASTC decoder from Basis Universal.
OPTION(ENABLE_ASTC "Enable ASTC decoding using the Basis Universal decoder." ON)

# Enable precompiled headers.
# NOTE: Requires CMake 3.16.0.
IF(NOT (CMAKE_VERSION VERSION_LESS 3.16.0))
	IF(MSVC)
		SET(PCH_DEFAULT ON)
	ELSE()
		SET(PCH_DEFAULT OFF)
	ENDIF()
	OPTION(ENABLE_PCH "Enable precompiled headers for faster builds." ${PCH_DEFAULT})
ELSE()
	SET(ENABLE_PCH OFF CACHE INTERNAL "Enable precompiled headers for faster builds." FORCE)
ENDIF()

# Link-time optimization.
# FIXME: Not working in clang builds and Ubuntu's gcc...
IF(MSVC)
	SET(LTO_DEFAULT ON)
ELSE()
	SET(LTO_DEFAULT OFF)
ENDIF()
OPTION(ENABLE_LTO "Enable link-time optimization in release builds." ${LTO_DEFAULT})

# Split debug information into a separate file.
# FIXME: macOS `strip` shows an error:
# error: symbols referenced by indirect symbol table entries that can't be stripped in: [library]
# NOTE: Disabled on Emscripten because it's JavaScript/WebAssembly.
IF(NOT EMSCRIPTEN)
IF(APPLE)
	OPTION(SPLIT_DEBUG "Split debug information into a separate file." OFF)
ELSE(APPLE)
	OPTION(SPLIT_DEBUG "Split debug information into a separate file." ON)
ENDIF(APPLE)

# Install the split debug file.
OPTION(INSTALL_DEBUG "Install the split debug files." ON)
IF(INSTALL_DEBUG AND NOT SPLIT_DEBUG)
	# Cannot install debug files if we're not splitting them.
	SET(INSTALL_DEBUG OFF CACHE INTERNAL "Install the split debug files." FORCE)
ENDIF(INSTALL_DEBUG AND NOT SPLIT_DEBUG)
ENDIF(NOT EMSCRIPTEN)

# Enable coverage checking. (gcc/clang only)
OPTION(ENABLE_COVERAGE "Enable code coverage checking. (gcc/clang only)" OFF)
IF(ENABLE_COVERAGE)
	ADD_DEFINITIONS(-DGCOV)
ENDIF(ENABLE_COVERAGE)

# Enable NLS. (internationalization)
IF(NOT WIN32 OR NOT MSVC)
	OPTION(ENABLE_NLS "Enable NLS using gettext for localized messages." ON)
ELSEIF(MSVC AND _MSVC_C_ARCHITECTURE_FAMILY MATCHES "^([iI]?[xX3]86)|([xX]64)$")
	OPTION(ENABLE_NLS "Enable NLS using gettext for localized messages." ON)
ELSE()
	SET(ENABLE_NLS OFF CACHE INTERNAL "Enable NLS using gettext for localized messages." FORCE)
ENDIF()

# Linux security options.
IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	OPTION(INSTALL_APPARMOR "Install AppArmor profiles." ON)
ELSE(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	SET(INSTALL_APPARMOR OFF)
ENDIF(CMAKE_SYSTEM_NAME STREQUAL "Linux")

# Achievements. (TODO: "AUTO" option?)
OPTION(ENABLE_ACHIEVEMENTS "Enable achievement pop-ups." ON)

# Install documentation
OPTION(INSTALL_DOC "Install documentation." ON)
