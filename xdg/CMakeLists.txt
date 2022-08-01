PROJECT(doc)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# XDG files are only installed on XDG systems,
# so check that the XDG paths are valid before
# installing the files.

# Desktop file
IF(DIR_INSTALL_XDG_DESKTOP)
	INSTALL(FILES rp-config.desktop
		DESTINATION "${DIR_INSTALL_XDG_DESKTOP}/com.gerbilsoft.rom-properties.rp-config.desktop"
		COMPONENT "xdg")
ENDIF(DIR_INSTALL_XDG_DESKTOP)

# XDG MIME type package
IF(DIR_INSTALL_XDG_MIME)
	INSTALL(FILES rom-properties.xml
		DESTINATION "${DIR_INSTALL_XDG_MIME}/packages"
		COMPONENT "xdg")
ENDIF(DIR_INSTALL_XDG_MIME)

# AppStream metainfo XML
IF(DIR_INSTALL_XDG_APPSTREAM)
	INSTALL(FILES com.gerbilsoft.rom-properties.metainfo.xml
		DESTINATION "${DIR_INSTALL_XDG_APPSTREAM}"
		COMPONENT "xdg")
ENDIF(DIR_INSTALL_XDG_APPSTREAM)
