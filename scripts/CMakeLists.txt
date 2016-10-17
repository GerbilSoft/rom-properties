PROJECT(scripts)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

IF(WIN32)
	# Install the Windows batch files for
	# registering and unregistering the DLL(s).
	INSTALL(FILES
		install.bat
		uninstall.bat
		DESTINATION "."
		COMPONENT "dll"
		)
ENDIF(WIN32)
