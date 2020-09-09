# Find LibCanberra GTK+ 2.x libraries and headers.
# If found, the following variables will be defined:
# - LibCanberraGtk2_FOUND: System has LibCanberra for GTK+ 2.x.
# - LibCanberraGtk2_INCLUDE_DIRS: LibCanberra GTK2 include directories.
# - LibCanberraGtk2_LIBRARIES: LibCanberra GTK2 libraries.
# - LibCanberraGtk2_DEFINITIONS: Compiler switches required for using LibCanberra GTK2.
#
# In addition, a target LibCanberra::gtk2 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(LibCanberraGtk2
	libcanberra-gtk		# pkgconfig
	canberra-gtk.h		# header
	canberra-gtk		# library
	LibCanberra::gtk2	# imported target
	)
