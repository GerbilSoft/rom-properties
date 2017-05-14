# Find TinyXML2 libraries and headers.
# If found, the following variables will be defined:
# - TinyXML2_FOUND: System has TinyXML2.
# - TinyXML2_INCLUDE_DIRS: GLib2 include directories.
# - TinyXML2_LIBRARIES: GLib2 libraries.
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
FIND_LIBRARY_PKG_CONFIG(TinyXML2
	tinyxml2	# pkgconfig
	tinyxml2.h	# header
	tinyxml2	# library
	tinyxml2	# imported target
	)
