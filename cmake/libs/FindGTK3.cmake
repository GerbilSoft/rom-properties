# Find GTK3 libraries and headers. (for GTK and GDK)
# If found, the following variables will be defined:
#
# - GTK3_FOUND: System has GTK3.
# - GTK3_INCLUDE_DIRS: GTK3 include directories.
# - GTK3_LIBRARIES: GTK3 libraries.
# - GTK3_DEFINITIONS: Compiler switches required for using GTK3.
#
# - GDK3_FOUND: System has GDK3.
# - GDK3_INCLUDE_DIRS: GDK3 include directories.
# - GDK3_LIBRARIES: GDK3 libraries.
# - GDK3_DEFINITIONS: Compiler switches required for using GDK3.
#
# In addition, the targets Gtk3::gtk3 and Gtk3::gdk will be
# created with all of these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(GTK3
	gtk+-3.0	# pkgconfig
	gtk/gtk.h	# header
	gtk-3		# library
	Gtk3::gtk3	# imported target
	)
FIND_LIBRARY_PKG_CONFIG(GDK3
	gdk-3.0		# pkgconfig
	gdk/gdk.h	# header
	gdk-3		# library
	Gtk3::gdk	# imported target
	)
