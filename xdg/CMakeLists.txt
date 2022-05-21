PROJECT(doc)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# XDG files are only installed on Linux systems.
IF(UNIX AND NOT APPLE)
	# Desktop file
	CONFIGURE_FILE(rp-config.desktop.in rp-config.desktop)
	INSTALL(FILES rp-config.desktop
		DESTINATION "${DIR_INSTALL_XDG_DESKTOP}"
		COMPONENT "xdg"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		)

	# Install the XDG MIME type package.
	INSTALL(FILES rom-properties.xml
		DESTINATION "${DIR_INSTALL_MIME}/packages"
		COMPONENT "xdg"
		)
ENDIF(UNIX AND NOT APPLE)
