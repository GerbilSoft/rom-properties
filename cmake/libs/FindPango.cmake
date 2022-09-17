# Find Pango libraries and headers.
# If found, the following variables will be defined:
# - Pango_FOUND: System has Pango.
# - Pango_INCLUDE_DIRS: Pango include directories.
# - Pango_LIBRARIES: Pango libraries.
# - Pango_DEFINITIONS: Compiler switches required for using Pango.
#
# In addition, a target Gtk3::gtk3 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(Pango
	pango		# pkgconfig
	pango/pango.h	# header
	pango-1.0	# library
	Pango::pango	# imported target
	)
