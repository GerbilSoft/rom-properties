# Find XxHash libraries and headers.
# If found, the following variables will be defined:
# - XxHash_FOUND: System has XxHash.
# - XxHash_INCLUDE_DIRS: XxHash include directories.
# - XxHash_LIBRARIES: XxHash libraries.
# - XxHash_DEFINITIONS: Compiler switches required for using XxHash.
#
# In addition, a target XxHash::xxhash will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(XxHash
	libxxhash	# pkgconfig
	xxhash.h	# header
	xxhash		# library
	XxHash::xxhash	# imported target
	)
