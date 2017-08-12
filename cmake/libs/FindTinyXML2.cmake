# Find TinyXML2 libraries and headers.
# If found, the following variables will be defined:
# - TinyXML2_FOUND: System has TinyXML2.
# - TinyXML2_INCLUDE_DIRS: TinyXML2 include directories.
# - TinyXML2_LIBRARIES: TinyXML2 libraries.
# - TinyXML2_DEFINITIONS: Compiler switches required for using TinyXML2.
#
# In addition, a target TinyXML2::tinyxml2 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(TinyXML2
	tinyxml2		# pkgconfig
	tinyxml2.h		# header
	tinyxml2		# library
	TinyXML2::tinyxml2	# imported target
	)
