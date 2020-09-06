# Find LZO libraries and headers.
# If found, the following variables will be defined:
# - LZO_FOUND: System has LZO.
# - LZO_INCLUDE_DIRS: LZO include directories.
# - LZO_LIBRARIES: LZO libraries.
# - LZO_DEFINITIONS: Compiler switches required for using LZO.
#
# In addition, a target LZO::lzo will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(LZO
	lzo2		# pkgconfig
	lzo/lzo1x.h	# header
	lzo2		# library
	LZO::lzo	# imported target
	)
