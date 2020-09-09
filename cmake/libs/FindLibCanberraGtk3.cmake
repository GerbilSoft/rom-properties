# Find LibCanberra GTK+ 3.x libraries and headers.
# If found, the following variables will be defined:
# - LibCanberraGtk3_FOUND: System has LibCanberra for GTK+ 3.x.
# - LibCanberraGtk3_INCLUDE_DIRS: LibCanberra GTK3 include directories.
# - LibCanberraGtk3_LIBRARIES: LibCanberra GTK3 libraries.
# - LibCanberraGtk3_DEFINITIONS: Compiler switches required for using LibCanberra GTK3.
#
# In addition, a target LibCanberra::gtk3 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(LibCanberraGtk3
	libcanberra-gtk3	# pkgconfig
	canberra-gtk.h		# header
	canberra-gtk3		# library
	LibCanberra::gtk3	# imported target
	)
