# Find CairoGObject libraries and headers.
# If found, the following variables will be defined:
# - CairoGObject_FOUND: System has CairoGObject.
# - CairoGObject_INCLUDE_DIRS: CairoGObject include directories.
# - CairoGObject_LIBRARIES: CairoGObject libraries.
# - CairoGObject_DEFINITIONS: Compiler switches required for using CairoGObject.
#
# In addition, a target CairoGObject::CairoGObject will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(CairoGObject
	cairo-gobject	# pkgconfig
	cairo-gobject.h	# header
	cairo-gobject	# library
	Cairo::gobject	# imported target
	)
