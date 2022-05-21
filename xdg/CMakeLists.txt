PROJECT(doc)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# XDG files are only installed on Linux systems.
IF(UNIX AND NOT APPLE)
	# Desktop file
	CONFIGURE_FILE(rp-config.desktop.in rp-config.desktop)
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/rp-config.desktop"
		DESTINATION "${DIR_INSTALL_XDG_DESKTOP}"
		COMPONENT "xdg"
		)

	# Install the XDG MIME type package.
	INSTALL(FILES rom-properties.xml
		DESTINATION "${DIR_INSTALL_MIME}/packages"
		COMPONENT "xdg"
		)
ENDIF(UNIX AND NOT APPLE)
