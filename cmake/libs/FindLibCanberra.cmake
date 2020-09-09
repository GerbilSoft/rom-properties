# Find LibCanberra libraries and headers.
# If found, the following variables will be defined:
# - LibCanberra_FOUND: System has LibCanberra.
# - LibCanberra_INCLUDE_DIRS: LibCanberra include directories.
# - LibCanberra_LIBRARIES: LibCanberra libraries.
# - LibCanberra_DEFINITIONS: Compiler switches required for using LibCanberra.
#
# In addition, a target LibCanberra::libcanberra will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(LibCanberra
	libcanberra			# pkgconfig
	canberra.h			# header
	canberra			# library
	LibCanberra::libcanberra	# imported target
	)
