# Find GSound libraries and headers.
# If found, the following variables will be defined:
# - GSound_FOUND: System has GSound.
# - GSound_INCLUDE_DIRS: GSound include directories.
# - GSound_LIBRARIES: GSound libraries.
# - GSound_DEFINITIONS: Compiler switches required for using GSound.
#
# In addition, a target GSound::gsound will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(GSound
	gsound			# pkgconfig
	gsound.h		# header
	gsound			# library
	GSound::gsound		# imported target
	)
