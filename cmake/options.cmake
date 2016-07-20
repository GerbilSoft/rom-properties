# Build options.

# KDE4 plugin.
# NOTE: This is currently the only one available, so it's always enabled.
SET(BUILD_KDE4 ON)

# Split debug information into a separate file.
OPTION(SPLIT_DEBUG "Split debug information into a separate file." 1)

# Install the split debug file.
OPTION(INSTALL_DEBUG "Install the split debug file." 1)
