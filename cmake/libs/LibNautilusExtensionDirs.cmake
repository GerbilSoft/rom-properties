# libnautilus-extension directories.

# We're not actually using it directly, but the directories are needed
# for the installation paths. It's normally not possible to have multiple
# versions of libnautilus-extension installed, but for debugging purposes,
# it's useful to have directories for both the GTK3 and GTK4 versions
# available.

INCLUDE(DirInstallPaths)

IF(NOT DEFINED LibNautilusExtension3_EXTENSION_DIR)
	SET(LibNautilusExtension3_EXTENSION_DIR
		"${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_LIB}/nautilus/extensions-3.0"
		CACHE INTERNAL "LibNautilusExtension3_EXTENSION_DIR"
		)
ENDIF(NOT DEFINED LibNautilusExtension3_EXTENSION_DIR)

IF(NOT DEFINED LibNautilusExtension4_EXTENSION_DIR)
	SET(LibNautilusExtension4_EXTENSION_DIR
		"${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_LIB}/nautilus/extensions-4.0"
		CACHE INTERNAL "LibNautilusExtension4_EXTENSION_DIR"
		)
ENDIF(NOT DEFINED LibNautilusExtension4_EXTENSION_DIR)
