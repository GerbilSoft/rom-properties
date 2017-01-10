# Find GdkPixbuf2 libraries and headers.
# If found, the following variables will be defined:
# - GdkPixbuf2_FOUND: System has GdkPixbuf2.
# - GdkPixbuf2_INCLUDE_DIRS: GdkPixbuf2 include directories.
# - GdkPixbuf2_LIBRARIES: GdkPixbuf2 libraries.
# - GdkPixbuf2_DEFINITIONS: Compiler switches required for using GdkPixbuf2.
#
# In addition, a target GdkPixbuf2::gdkpixbuf2 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(GdkPixbuf2
	gdk-pixbuf-2.0		# pkgconfig
	gdk-pixbuf/gdk-pixbuf.h		# header
	gdk_pixbuf-2.0			# library
	GdkPixbuf2::gdkpixbuf2		# imported target
	)
