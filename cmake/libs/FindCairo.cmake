# Find Cairo libraries and headers.
# If found, the following variables will be defined:
# - Cairo_FOUND: System has Cairo.
# - Cairo_INCLUDE_DIRS: Cairo include directories.
# - Cairo_LIBRARIES: Cairo libraries.
# - Cairo_DEFINITIONS: Compiler switches required for using Cairo.
#
# In addition, a target Cairo::cairo will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(Cairo
	cairo		# pkgconfig
	cairo.h		# header
	cairo		# library
	Cairo::cairo	# imported target
	)
