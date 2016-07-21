# Build options.

# Platform options.
# NOTE: If a platform is specified but it isn't found,
# that plugin will not be built. There doesn't seem to
# be a way to have "yes, no, auto" like in autotools.
OPTION(BUILD_KDE4 "Build the KDE4 plugin." ON)
OPTION(BUILD_KDE5 "Build the KDE5 plugin." ON)

# Split debug information into a separate file.
OPTION(SPLIT_DEBUG "Split debug information into a separate file." 1)

# Install the split debug file.
OPTION(INSTALL_DEBUG "Install the split debug file." 1)
