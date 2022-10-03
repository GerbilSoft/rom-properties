# Find Gnome's libnautilus-extension.so.4 libraries and headers.
# If found, the following variables will be defined:
# - LibNautilusExtension4_FOUND: System has libnautilus-extension.
# - LibNautilusExtension4_INCLUDE_DIRS: libnautilus-extension include directories.
# - LibNautilusExtension4_LIBRARIES: libnautilus-extension libraries.
# - LibNautilusExtension4_DEFINITIONS: Compiler switches required for using libnautilus-extension.
# - LibNautilusExtension4_EXTENSION_DIR: Extensions directory. (for installation)
#
# In addition, a target Gnome::libnautilus-extension-4 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(LibNautilusExtension4
	libnautilus-extension-4					# pkgconfig
	libnautilus-extension/nautilus-properties-item.h	# header
	nautilus-extension					# library
	Gnome::libnautilus-extension-4				# imported target
	)

# Extensions directory.
IF(LibNautilusExtension4_FOUND AND NOT LibNautilusExtension4_EXTENSION_DIR)
	MESSAGE(WARNING "LibNautilusExtension4_EXTENSION_DIR is not set; using defaults.")
	INCLUDE(DirInstallPaths)
	SET(LibNautilusExtension4_EXTENSION_DIR "${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_LIB}/nautilus/extensions-4" CACHE INTERNAL "LibNautilusExtension4_EXTENSION_DIR")
ENDIF(LibNautilusExtension4_FOUND AND NOT LibNautilusExtension4_EXTENSION_DIR)
