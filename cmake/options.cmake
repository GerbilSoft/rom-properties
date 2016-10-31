# Build options.

# Platform options.
# NOTE: If a platform is specified but it isn't found,
# that plugin will not be built. There doesn't seem to
# be a way to have "yes, no, auto" like in autotools.

IF(UNIX AND NOT APPLE)
	OPTION(BUILD_KDE4 "Build the KDE4 plugin." ON)
	OPTION(BUILD_KDE5 "Build the KDE5 plugin." ON)
ELSEIF(WIN32)
	SET(BUILD_WIN32 ON)
ENDIF()

# Enable decryption for newer ROM and disc images.
OPTION(ENABLE_DECRYPTION "Enable decryption for newer ROM and disc images." ON)

# Split debug information into a separate file.
OPTION(SPLIT_DEBUG "Split debug information into a separate file." ON)

# Install the split debug file.
OPTION(INSTALL_DEBUG "Install the split debug files." ON)

# Enable coverage checking. (gcc/clang only)
OPTION(ENABLE_COVERAGE "Enable code coverage checking. (gcc/clang only)" OFF)
