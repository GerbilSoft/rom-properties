# Find ThunarX libraries and headers. (GTK+ 2.x version)
# If found, the following variables will be defined:
# - ThunarX2_FOUND: System has ThunarX.
# - ThunarX2_INCLUDE_DIRS: ThunarX include directories.
# - ThunarX2_LIBRARIES: ThunarX libraries.
# - ThunarX2_DEFINITIONS: Compiler switches required for using ThunarX.
# - ThunarX2_EXTENSIONS_DIR: Extensions directory. (for installation)
#
# In addition, a target Xfce::thunarx-2 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

# TODO: Replace package prefix with CMAKE_INSTALL_PREFIX.
INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(ThunarX2
	thunarx-2		# pkgconfig
	thunarx/thunarx.h	# header
	thunarx-2		# library
	Xfce::thunarx-2		# imported target
	)

# Extensions directory.
IF(ThunarX2_FOUND AND NOT ThunarX2_EXTENSIONS_DIR)
	MESSAGE(WARNING "ThunarX2_EXTENSIONS_DIR is not set; using defaults.")
	INCLUDE(DirInstallPaths)
	SET(ThunarX2_EXTENSIONS_DIR "${DIR_INSTALL_LIB}/thunarx-2" CACHE INTERNAL "ThunarX2_EXTENSIONS_DIR")
ENDIF(ThunarX2_FOUND AND NOT ThunarX2_EXTENSIONS_DIR)
