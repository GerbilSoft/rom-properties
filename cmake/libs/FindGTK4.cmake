# Find GTK4 libraries and headers.
# If found, the following variables will be defined:
# - GTK4_FOUND: System has GTK4.
# - GTK4_INCLUDE_DIRS: GTK4 include directories.
# - GTK4_LIBRARIES: GTK4 libraries.
# - GTK4_DEFINITIONS: Compiler switches required for using GTK4.
#
# In addition, a target Gtk3::gtk3 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(GTK4
	gtk4		# pkgconfig
	gtk/gtk.h	# header
	gtk-4		# library
	Gtk4::gtk4	# imported target
	)
