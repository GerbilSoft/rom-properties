# Find GLIB2 libraries and headers.
# If found, the following variables will be defined:
# - GLib2_FOUND: System has GLib2.
# - GLib2_INCLUDE_DIRS: GLib2 include directories.
# - GLib2_LIBRARIES: GLib2 libraries.
# - GLib2_DEFINITIONS: Compiler switches required for using GLib2.
#
# In addition, a target GLib2::glib2 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

FIND_PACKAGE(PkgConfig)
IF(PkgConfig_FOUND)
	PKG_CHECK_MODULES(PC_GLib2 QUIET glib-2.0)
	SET(GLib2_DEFINITIONS ${PC_GLib2_CFLAGS_OTHER})

	FIND_PATH(GLib2_INCLUDE_DIR glib.h
		HINTS ${PC_GLib2_INCLUDEDIR} ${PC_GLib2_INCLUDE_DIRS})
	FIND_LIBRARY(GLib2_LIBRARY NAMES glib-2.0 libglib-2.0
		HINTS ${PC_GLib2_LIBDIR} ${PC_GLib2_LIBRARY_DIRS})

	# Version must be set before calling FPHSA.
	SET(GLib2_VERSION "${PC_GLib2_VERSION}")
	MARK_AS_ADVANCED(GLib2_VERSION)

	# Handle the QUIETLY and REQUIRED arguments and set GLib2_FOUND to TRUE
	# if all listed variables are TRUE.
	INCLUDE(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLib2
		FOUND_VAR GLib2_FOUND
		REQUIRED_VARS GLib2_LIBRARY GLib2_INCLUDE_DIR
		VERSION_VAR GLib2_VERSION
		)
	MARK_AS_ADVANCED(GLib2_INCLUDE_DIR GLib2_LIBRARY)

	SET(GLib2_LIBRARIES ${GLib2_LIBRARY})
	SET(GLib2_INCLUDE_DIRS ${GLib2_INCLUDE_DIR})

	# Create the library target.
	IF(GLib2_FOUND)
		IF(NOT TARGET GLib2::glib2)
			ADD_LIBRARY(GLib2::glib2 UNKNOWN IMPORTED)
		ENDIF(NOT TARGET GLib2::glib2)
		SET_TARGET_PROPERTIES(GLib2::glib2 PROPERTIES
			IMPORTED_LOCATION "${GLib2_LIBRARY}")
		SET_TARGET_PROPERTIES(GLib2::glib2 PROPERTIES
			COMPILE_DEFINITIONS "${GLib2_DEFINITIONS}")

		# Add include directories from the pkgconfig's Cflags section.
		FOREACH(CFLAG ${PC_GLib2_CFLAGS})
			IF(CFLAG MATCHES "^-I")
				STRING(SUBSTRING "${CFLAG}" 2 -1 INCDIR)
				LIST(APPEND GLib2_INCLUDE_DIRS "${INCDIR}")
				UNSET(INCDIR)
			ENDIF(CFLAG MATCHES "^-I")
		ENDFOREACH()
		SET_TARGET_PROPERTIES(GLib2::glib2 PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${GLib2_INCLUDE_DIRS}")
	ENDIF(GLib2_FOUND)
ENDIF(PkgConfig_FOUND)
