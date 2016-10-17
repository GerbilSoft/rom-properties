PROJECT(doc)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# TODO: Convert from Unix line endings to Windows when
# compiling for Windows?
INSTALL(FILES
	keys.conf.example
	../README.md
	../LICENSE
	DESTINATION "${DIR_INSTALL_DOC}"
	COMPONENT "doc"
	)
