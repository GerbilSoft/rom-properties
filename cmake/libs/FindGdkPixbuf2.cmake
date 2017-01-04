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

FIND_PACKAGE(PkgConfig)
IF(PkgConfig_FOUND)
	PKG_CHECK_MODULES(PC_GdkPixbuf2 QUIET gdk-pixbuf-2.0)
	SET(GdkPixbuf2_DEFINITIONS ${PC_GdkPixbuf2_CFLAGS_OTHER})

	FIND_PATH(GdkPixbuf2_INCLUDE_DIR gdk-pixbuf/gdk-pixbuf.h
		HINTS ${PC_GdkPixbuf2_INCLUDEDIR} ${PC_GdkPixbuf2_INCLUDE_DIRS})
	FIND_LIBRARY(GdkPixbuf2_LIBRARY NAMES gdk_pixbuf-2.0 libgdk_pixbuf-2.0
		HINTS ${PC_GdkPixbuf2_LIBDIR} ${PC_GdkPixbuf2_LIBRARY_DIRS})

	# Version must be set before calling FPHSA.
	SET(GdkPixbuf2_VERSION "${PC_GdkPixbuf2_VERSION}")
	MARK_AS_ADVANCED(GdkPixbuf2_VERSION)

	# Handle the QUIETLY and REQUIRED arguments and set GdkPixbuf2_FOUND to TRUE
	# if all listed variables are TRUE.
	INCLUDE(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(GdkPixbuf2
		FOUND_VAR GdkPixbuf2_FOUND
		REQUIRED_VARS GdkPixbuf2_LIBRARY GdkPixbuf2_INCLUDE_DIR
		VERSION_VAR GdkPixbuf2_VERSION
		)
	MARK_AS_ADVANCED(GdkPixbuf2_INCLUDE_DIR GdkPixbuf2_LIBRARY)

	SET(GdkPixbuf2_LIBRARIES ${GdkPixbuf2_LIBRARY})
	SET(GdkPixbuf2_INCLUDE_DIRS ${GdkPixbuf2_INCLUDE_DIR})

	# Create the library target.
	IF(GdkPixbuf2_FOUND)
		IF(NOT TARGET GdkPixbuf2::gdkpixbuf2)
			ADD_LIBRARY(GdkPixbuf2::gdkpixbuf2 UNKNOWN IMPORTED)
		ENDIF(NOT TARGET GdkPixbuf2::gdkpixbuf2)
		SET_TARGET_PROPERTIES(GdkPixbuf2::gdkpixbuf2 PROPERTIES
			IMPORTED_LOCATION "${GdkPixbuf2_LIBRARY}")
		SET_TARGET_PROPERTIES(GdkPixbuf2::gdkpixbuf2 PROPERTIES
			COMPILE_DEFINITIONS "${GdkPixbuf2_DEFINITIONS}")

		# Add include directories from the pkgconfig's Cflags section.
		FOREACH(CFLAG ${PC_GdkPixbuf2_CFLAGS})
			IF(CFLAG MATCHES "^-I")
				STRING(SUBSTRING "${CFLAG}" 2 -1 INCDIR)
				LIST(APPEND GdkPixbuf2_INCLUDE_DIRS "${INCDIR}")
				UNSET(INCDIR)
			ENDIF(CFLAG MATCHES "^-I")
		ENDFOREACH()
		SET_TARGET_PROPERTIES(GdkPixbuf2::gdkpixbuf2 PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${GdkPixbuf2_INCLUDE_DIRS}")
	ENDIF(GdkPixbuf2_FOUND)
ENDIF(PkgConfig_FOUND)
