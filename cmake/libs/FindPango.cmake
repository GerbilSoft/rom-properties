# Find Pango libraries and headers.
# If found, the following variables will be defined:
# - Pango_FOUND: System has Pango.
# - Pango_INCLUDE_DIRS: Pango include directories.
# - Pango_LIBRARIES: Pango libraries.
# - Pango_DEFINITIONS: Compiler switches required for using Pango.
#
# In addition, a target Pango::glib2 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

# NOTE: The FindGTK2.cmake, FindGTK3.cmake, and FindGTK4.cmake files
# also find Pango, but the target is in the GTK2/GTK3/GTK4 namespaces.
# If both Pango::pango and e.g. GTK2::pango are linked to a target,
# this does *not* cause a conflict, since they're both pointing to
# the same thing.

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(Pango
	pango		# pkgconfig
	pango/pango.h	# header
	pango-1.0	# library
	Pango::pango	# imported target
	)
