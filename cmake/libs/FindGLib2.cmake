# Find GLIB2 libraries and headers.
# If found, the following variables will be defined:
# - GLIB2_FOUND: System has GLib2.
# - GLIB2_INCLUDE_DIRS: GLib2 include directories.
# - GLIB2_LIBRARIES: GLib2 libraries.
# - GLIB2_DEFINITIONS: Compiler switches required for using GLib2.
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
	PKG_CHECK_MODULES(PC_GLIB2 QUIET glib-2.0)
	SET(GLIB2_DEFINITIONS ${PC_GLIB2_CFLAGS_OTHER})

	FIND_PATH(GLIB2_INCLUDE_DIR glib.h
		HINTS ${PC_GLIB2_INCLUDEDIR} ${PC_GLIB2_INCLUDE_DIRS})
	FIND_LIBRARY(GLIB2_LIBRARY NAMES glib-2.0 libglib-2.0
		HINTS ${PC_GLIB2_LIBDIR} ${PC_GLIB2_LIBRARY_DIRS})

	# Handle the QUIETLY and REQUIRED arguments and set GLIB2_FOUND to TRUE
	# if all listed variables are TRUE.
	INCLUDE(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLIB2 DEFAULT_MSG GLIB2_LIBRARY GLIB2_INCLUDE_DIR)
	MARK_AS_ADVANCED(GLIB2_INCLUDE_DIR GLIB2_LIBRARY)

	SET(GLIB2_LIBRARIES ${GLIB2_LIBRARY})
	SET(GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIR})

	# Create the library target.
	IF(GLIB2_FOUND)
		IF(NOT TARGET GLib2::glib2)
			ADD_LIBRARY(GLib2::glib2 UNKNOWN IMPORTED)
		ENDIF(NOT TARGET GLib2::glib2)
		SET_TARGET_PROPERTIES(GLib2::glib2 PROPERTIES
			IMPORTED_LOCATION "${GLIB2_LIBRARY}")
		SET_TARGET_PROPERTIES(GLib2::glib2 PROPERTIES
			COMPILE_DEFINITIONS "${GLIB2_DEFINITIONS}")

		# Add include directories from the pkgconfig's Cflags section.
		FOREACH(CFLAG ${PC_GLIB2_CFLAGS})
			IF(CFLAG MATCHES "^-I")
				STRING(SUBSTRING "${CFLAG}" 2 -1 INCDIR)
				LIST(APPEND GLIB2_INCLUDE_DIRS "${INCDIR}")
				UNSET(INCDIR)
			ENDIF(CFLAG MATCHES "^-I")
		ENDFOREACH()
		SET_TARGET_PROPERTIES(GLib2::glib2 PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${GLIB2_INCLUDE_DIRS}")
	ENDIF(GLIB2_FOUND)
ENDIF(PkgConfig_FOUND)
