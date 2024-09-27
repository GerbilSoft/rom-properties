# Find MATE's libcaja-extension libraries and headers.
# If found, the following variables will be defined:
# - LibCajaExtension_FOUND: System has libcaja-extension.
# - LibCajaExtension_INCLUDE_DIRS: libcaja-extension include directories.
# - LibCajaExtension_LIBRARIES: libcaja-extension libraries.
# - LibCajaExtension_DEFINITIONS: Compiler switches required for using libcaja-extension.
# - LibCajaExtension_EXTENSION_DIR: Extensions directory. (for installation)
# - LibCajaExtension_EXTENSION_DESC_DIR: Extensions description directory. (for installation)
#
# In addition, a target Gnome::libcaja-extension will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(LibCajaExtension
	libcaja-extension				# pkgconfig
	libcaja-extension/caja-extension-types.h	# header
	caja-extension					# library
	Mate::libcaja-extension				# imported target
	)

# Extensions library directory
IF(LibCajaExtension_FOUND AND NOT LibCajaExtension_EXTENSION_DIR)
	MESSAGE(WARNING "LibCajaExtension_EXTENSION_DIR is not set; using defaults.")
	INCLUDE(DirInstallPaths)
	SET(LibCajaExtension_EXTENSION_DIR "${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_LIB}/caja/extensions-2.0" CACHE INTERNAL "LibCajaExtension_EXTENSION_DIR")
ENDIF(LibCajaExtension_FOUND AND NOT LibCajaExtension_EXTENSION_DIR)

# Extensions descriptions directory
# NOTE: Not in the pkgconfig file.
SET(LibCajaExtension_EXTENSION_DESC_DIR "${CMAKE_INSTALL_PREFIX}/share/caja/extensions" CACHE INTERNAL "LibCajaExtension_EXTENSION_DESC_DIR")
