# Find GLIB2 libraries and headers.
# If found, the following variables will be defined:
# - DBusGLib_FOUND: System has DBusGLib.
# - DBusGLib_INCLUDE_DIRS: DBusGLib include directories.
# - DBusGLib_LIBRARIES: DBusGLib libraries.
# - DBusGLib_DEFINITIONS: Compiler switches required for using DBusGLib.
#
# In addition, a target GLib2::dbus will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(DBusGLib
	dbus-glib-1	# pkgconfig
	dbus/dbus-glib-bindings.h	# header
	dbus-glib-1	# library
	GLib2::dbus	# imported target
	)

IF(DBusGLib_FOUND)
	# Find D-Bus binding tool.
	FIND_PROGRAM(GLIB_DBUS_BINDING_TOOL dbus-binding-tool)
	IF(NOT GLIB_DBUS_BINDING_TOOL)
		MESSAGE(FATAL_ERROR "dbus-binding-tool not found; dbus-glib isn't set up correctly.")
	ENDIF(NOT GLIB_DBUS_BINDING_TOOL)
ENDIF(DBusGLib_FOUND)
