# Build options.

# Platform options.
# NOTE: If a platform is specified but it isn't found,
# that plugin will not be built. There doesn't seem to
# be a way to have "yes, no, auto" like in autotools.

IF(UNIX AND NOT APPLE)
	OPTION(BUILD_KDE4 "Build the KDE4 plugin." ON)
	OPTION(BUILD_KDE5 "Build the KDE5 plugin." ON)
	OPTION(BUILD_XFCE "Build the XFCE (GTK+ 2.x) plugin." ON)
	OPTION(BUILD_GNOME "Build the GNOME (GTK+ 3.x) plugin." ON)

	# Set BUILD_GTK2 and/or BUILD_GTK3 depending on frontends.
	IF(BUILD_XFCE)
		SET(BUILD_GTK2 ON CACHE "Check for GTK+ 2.x." INTERNAL FORCE)
	ELSE(BUILD_XFCE)
		SET(BUILD_GTK2 OFF CACHE "Check for GTK+ 2.x." INTERNAL FORCE)
	ENDIF(BUILD_XFCE)
	IF(BUILD_GNOME)
		SET(BUILD_GTK3 ON CACHE "Check for GTK+ 3.x." INTERNAL FORCE)
	ELSE(BUILD_GNOME)
		SET(BUILD_GTK3 OFF CACHE "Check for GTK+ 3.x." INTERNAL FORCE)
	ENDIF(BUILD_GNOME)
ELSEIF(WIN32)
	SET(BUILD_WIN32 ON)
ENDIF()

OPTION(BUILD_CLI "Build the `rpcli` command line program." ON)

# ZLIB, libpng, libjpeg-turbo
# Internal versions are always used on Windows.
IF(WIN32)
	SET(USE_INTERNAL_ZLIB ON)
	SET(USE_INTERNAL_PNG ON)
	OPTION(ENABLE_JPEG "Enable JPEG decoding using libjpeg." ON)
	SET(USE_INTERNAL_JPEG ON)
ELSE(WIN32)
	OPTION(USE_INTERNAL_ZLIB "Use the internal copy of zlib." OFF)
	OPTION(USE_INTERNAL_PNG "Use the internal copy of libpng." OFF)
	OPTION(ENABLE_JPEG "Enable JPEG decoding using libjpeg." ON)
	#OPTION(USE_INTERNAL_JPEG "Use the internal copy of libjpeg-turbo." OFF)
	SET(USE_INTERNAL_JPEG OFF)
ENDIF()

# TODO: If APNG export is added, verify that system libpng
# supports APNG.

# Enable decryption for newer ROM and disc images.
OPTION(ENABLE_DECRYPTION "Enable decryption for newer ROM and disc images." ON)

# Split debug information into a separate file.
OPTION(SPLIT_DEBUG "Split debug information into a separate file." ON)

# Install the split debug file.
OPTION(INSTALL_DEBUG "Install the split debug files." ON)
IF(INSTALL_DEBUG AND NOT SPLIT_DEBUG)
	# Cannot install debug files if we're not splitting them.
	SET(INSTALL_DEBUG OFF CACHE "Install the split debug files." INTERNAL FORCE)
ENDIF(INSTALL_DEBUG AND NOT SPLIT_DEBUG)

# Enable coverage checking. (gcc/clang only)
OPTION(ENABLE_COVERAGE "Enable code coverage checking. (gcc/clang only)" OFF)
