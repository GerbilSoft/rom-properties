# Find GIO-UNIX libraries and headers.
# If found, the following variables will be defined:
# - GIO-UNIX_FOUND: System has GIO-UNIX.
# - GIO-UNIX_INCLUDE_DIRS: GIO-UNIX include directories.
# - GIO-UNIX_LIBRARIES: GIO-UNIX libraries.
# - GIO-UNIX_DEFINITIONS: Compiler switches required for using GIO-UNIX.
#
# In addition, a target GLib2::gio-unix will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(GIO-UNIX
	gio-unix-2.0		# pkgconfig
	gio/gunixfdlist.h	# header
	gio-2.0			# library
	GLib2::gio-unix		# imported target
	)

IF(GLib2_FOUND)
	# Find the GDBus code generator.
	FIND_PROGRAM(GDBUS_CODEGEN gdbus-codegen)
	IF(NOT GDBUS_CODEGEN)
		MESSAGE(FATAL_ERROR "gdbus-codegen not found; glib-2.x isn't set up correctly.")
	ENDIF(NOT GDBUS_CODEGEN)
ENDIF(GLib2_FOUND)
