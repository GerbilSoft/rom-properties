# Find Gnome's libnautilus-extension libraries and headers.
# If found, the following variables will be defined:
# - LibNautilusExtension_FOUND: System has ThunarX.
# - LibNautilusExtension_INCLUDE_DIRS: ThunarX include directories.
# - LibNautilusExtension_LIBRARIES: ThunarX libraries.
# - LibNautilusExtension_DEFINITIONS: Compiler switches required for using ThunarX.
# - LibNautilusExtension_EXTENSION_DIR: Extensions directory. (for installation)
#
# In addition, a target Gnome::libnautilus-extension will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(LibNautilusExtension
	libnautilus-extension					# pkgconfig
	libnautilus-extension/nautilus-extension-types.h	# header
	nautilus-extension					# library
	Gnome::libnautilus-extension				# imported target
	)

# Extensions directory.
IF(LibNautilusExtension_FOUND AND NOT LibNautilusExtension_EXTENSION_DIR)
	MESSAGE(WARNING "LibNautilusExtension_EXTENSION_DIR is not set; using defaults.")
	INCLUDE(DirInstallPaths)
	SET(LibNautilusExtension_EXTENSION_DIR "${DIR_INSTALL_LIB}/nautilus/extensions-3.0" CACHE INTERNAL "LibNautilusExtension_EXTENSION_DIR")
ENDIF(LibNautilusExtension_FOUND AND NOT LibNautilusExtension_EXTENSION_DIR)
