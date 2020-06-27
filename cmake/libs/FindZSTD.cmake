# Find ZSTD libraries and headers.
# If found, the following variables will be defined:
# - ZSTD_FOUND: System has ZSTD.
# - ZSTD_INCLUDE_DIRS: ZSTD include directories.
# - ZSTD_LIBRARIES: ZSTD libraries.
# - ZSTD_DEFINITIONS: Compiler switches required for using ZSTD.
#
# In addition, a target Gtk3::gtk3 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(ZSTD
	libzstd		# pkgconfig
	zstd.h		# header
	zstd		# library
	ZSTD::zstd	# imported target
	)
