# Find Minizip libraries and headers.
# If found, the following variables will be defined:
# - MinizipNG_FOUND: System has Minizip.
# - MinizipNG_INCLUDE_DIRS: MinizipNG include directories.
# - MinizipNG_LIBRARIES: MinizipNG libraries.
# - MinizipNG_DEFINITIONS: Compiler switches required for using MinizipNG.
#
# In addition, a target MINIZIP::minizip-ng will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

# NOTE: We're actually searching for minizip-ng, *not* original minizip.
INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(MinizipNG
	minizip-ng		# pkgconfig
	minizip-ng/mz.h		# header
	minizip-ng		# library
	MINIZIP::minizip-ng	# imported target
	)
