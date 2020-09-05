# Find LZ4 libraries and headers.
# If found, the following variables will be defined:
# - LZ4_FOUND: System has LZ4.
# - LZ4_INCLUDE_DIRS: LZ4 include directories.
# - LZ4_LIBRARIES: LZ4 libraries.
# - LZ4_DEFINITIONS: Compiler switches required for using LZ4.
#
# In addition, a target Gtk3::gtk3 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(LZ4
	liblz4		# pkgconfig
	lz4.h		# header
	lz4		# library
	LZ4::lz4	# imported target
	)
