# Find Blkid libraries and headers.
# If found, the following variables will be defined:
# - Blkid_FOUND: System has Blkid.
# - Blkid_INCLUDE_DIRS: Blkid include directories.
# - Blkid_LIBRARIES: Blkid libraries.
# - Blkid_DEFINITIONS: Compiler switches required for using Blkid.
#
# In addition, a target Blkid::gio will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(Blkid
	blkid		# pkgconfig
	blkid/blkid.h	# header
	blkid		# library
	Blkid::blkid	# imported target
	)
