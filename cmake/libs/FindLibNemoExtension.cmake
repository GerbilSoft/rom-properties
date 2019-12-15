# Find Cinnamon's libnemo-extension libraries and headers.
# If found, the following variables will be defined:
# - LibNemoExtension_FOUND: System has libnemo-extension.
# - LibNemoExtension_INCLUDE_DIRS: libnemo-extension include directories.
# - LibNemoExtension_LIBRARIES: libnemo-extension libraries.
# - LibNemoExtension_DEFINITIONS: Compiler switches required for using libnemo-extension.
# - LibNemoExtension_EXTENSION_DIR: Extensions directory. (for installation)
#
# In addition, a target Cinnamon::libnemo-extension will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(LibNemoExtension
	libnemo-extension				# pkgconfig
	libnemo-extension/nemo-extension-types.h	# header
	nemo-extension					# library
	Cinnamon::libnemo-extension			# imported target
	)

# Extensions directory.
IF(LibNemoExtension_FOUND AND NOT LibNemoExtension_EXTENSION_DIR)
	MESSAGE(WARNING "LibNemoExtension_EXTENSION_DIR is not set; using defaults.")
	INCLUDE(DirInstallPaths)
	SET(LibNemoExtension_EXTENSION_DIR "${DIR_INSTALL_LIB}/nemo/extensions-3.0" CACHE INTERNAL "LibNemoExtension_EXTENSION_DIR")
ENDIF(LibNemoExtension_FOUND AND NOT LibNemoExtension_EXTENSION_DIR)
