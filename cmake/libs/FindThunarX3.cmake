# Find ThunarX libraries and headers. (GTK+ 3.x version)
# If found, the following variables will be defined:
# - ThunarX3_FOUND: System has ThunarX.
# - ThunarX3_INCLUDE_DIRS: ThunarX include directories.
# - ThunarX3_LIBRARIES: ThunarX libraries.
# - ThunarX3_DEFINITIONS: Compiler switches required for using ThunarX.
# - ThunarX3_EXTENSIONS_DIR: Extensions directory. (for installation)
#
# In addition, a target Xfce::thunarx-3 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

# TODO: Replace package prefix with CMAKE_INSTALL_PREFIX.
INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(ThunarX3
	thunarx-3		# pkgconfig
	thunarx/thunarx.h	# header
	thunarx-3		# library
	Xfce::thunarx-3		# imported target
	)

# Extensions directory.
IF(ThunarX3_FOUND AND NOT ThunarX3_EXTENSIONS_DIR)
	MESSAGE(WARNING "ThunarX3_EXTENSIONS_DIR is not set; using defaults.")
	INCLUDE(DirInstallPaths)
	SET(ThunarX3_EXTENSIONS_DIR "${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_LIB}/thunarx-3" CACHE INTERNAL "ThunarX3_EXTENSIONS_DIR")
ENDIF(ThunarX3_FOUND AND NOT ThunarX3_EXTENSIONS_DIR)
