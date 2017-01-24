# Find GLIB2 libraries and headers.
# If found, the following variables will be defined:
# - GObject2_FOUND: System has GObject2.
# - GObject2_INCLUDE_DIRS: GObject2 include directories.
# - GObject2_LIBRARIES: GObject2 libraries.
# - GObject2_DEFINITIONS: Compiler switches required for using GObject2.
#
# In addition, a target GObject2::glib2 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(GObject2
	gobject-2.0	# pkgconfig
	glib-object.h	# header
	gobject-2.0	# library
	GLib2::gobject	# imported target
	)
