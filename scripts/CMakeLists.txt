PROJECT(scripts)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

IF(WIN32)
	# Install the Windows batch files for
	# registering and unregistering the DLL(s).
	INSTALL(FILES
		install.cmd
		uninstall.cmd
		DESTINATION "."
		COMPONENT "dll"
		)
ENDIF(WIN32)
