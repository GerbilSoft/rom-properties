# Find GLIB2 libraries and headers.
# If found, the following variables will be defined:
# - GLib2_FOUND: System has GLib2.
# - GLib2_INCLUDE_DIRS: GLib2 include directories.
# - GLib2_LIBRARIES: GLib2 libraries.
# - GLib2_DEFINITIONS: Compiler switches required for using GLib2.
#
# In addition, a target GLib2::glib2 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(GLib2
	glib-2.0	# pkgconfig
	glib.h		# header
	glib-2.0	# library
	GLib2::glib	# imported target
	)
