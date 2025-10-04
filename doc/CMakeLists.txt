PROJECT(doc)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# TODO: Convert from Unix line endings to Windows when
# compiling for Windows?
INSTALL(FILES
	keys.conf.example
	rom-properties.conf.example
	COMPILING.md
	DESTINATION "${DIR_INSTALL_DOC}"
	COMPONENT "doc"
	)

# Root directory files should be installed in
# the root directory on Windows.
INSTALL(FILES
	../README.md
	../NEWS.md
	DESTINATION "${DIR_INSTALL_DOC_ROOT}"
	COMPONENT "doc"
	)

# Don't use a component for the LICENSE.
INSTALL(FILES
	../LICENSE
	DESTINATION "${DIR_INSTALL_DOC_ROOT}"
	)
