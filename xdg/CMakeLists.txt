PROJECT(doc)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# XDG files are only installed on Linux systems.
IF(UNIX AND NOT APPLE)
	# Install the XDG MIME type package.
	INSTALL(FILES rom-properties.xml
		DESTINATION "${DIR_INSTALL_MIME}/packages"
		COMPONENT "xdg"
		)
ENDIF(UNIX AND NOT APPLE)
