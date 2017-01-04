# Find GdkPixbuf2 libraries and headers.
# If found, the following variables will be defined:
# - GDKPIXBUF2_FOUND: System has GdkPixbuf2.
# - GDKPIXBUF2_INCLUDE_DIRS: GdkPixbuf2 include directories.
# - GDKPIXBUF2_LIBRARIES: GdkPixbuf2 libraries.
# - GDKPIXBUF2_DEFINITIONS: Compiler switches required for using GdkPixbuf2.
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
	PKG_CHECK_MODULES(PC_GDKPIXBUF2 QUIET gdk-pixbuf-2.0)
	SET(GDKPIXBUF2_DEFINITIONS ${PC_GDKPIXBUF2_CFLAGS_OTHER})

	FIND_PATH(GDKPIXBUF2_INCLUDE_DIR gdk-pixbuf/gdk-pixbuf.h
		HINTS ${PC_GDKPIXBUF2_INCLUDEDIR} ${PC_GDKPIXBUF2_INCLUDE_DIRS})
	FIND_LIBRARY(GDKPIXBUF2_LIBRARY NAMES gdk_pixbuf-2.0 libgdk_pixbuf-2.0
		HINTS ${PC_GDKPIXBUF2_LIBDIR} ${PC_GDKPIXBUF2_LIBRARY_DIRS})

	# Handle the QUIETLY and REQUIRED arguments and set GDKPIXBUF2_FOUND to TRUE
	# if all listed variables are TRUE.
	INCLUDE(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(GDKPIXBUF2 DEFAULT_MSG GDKPIXBUF2_LIBRARY GDKPIXBUF2_INCLUDE_DIR)
	MARK_AS_ADVANCED(GDKPIXBUF2_INCLUDE_DIR GDKPIXBUF2_LIBRARY)

	SET(GDKPIXBUF2_LIBRARIES ${GDKPIXBUF2_LIBRARY})
	SET(GDKPIXBUF2_INCLUDE_DIRS ${GDKPIXBUF2_INCLUDE_DIR})

	# Create the library target.
	IF(GDKPIXBUF2_FOUND)
		IF(NOT TARGET GdkPixbuf2::gdkpixbuf2)
			ADD_LIBRARY(GdkPixbuf2::gdkpixbuf2 UNKNOWN IMPORTED)
		ENDIF(NOT TARGET GdkPixbuf2::gdkpixbuf2)
		SET_TARGET_PROPERTIES(GdkPixbuf2::gdkpixbuf2 PROPERTIES
			IMPORTED_LOCATION "${GDKPIXBUF2_LIBRARY}")
		SET_TARGET_PROPERTIES(GdkPixbuf2::gdkpixbuf2 PROPERTIES
			COMPILE_DEFINITIONS "${GDKPIXBUF2_DEFINITIONS}")

		# Add include directories from the pkgconfig's Cflags section.
		FOREACH(CFLAG ${PC_GDKPIXBUF2_CFLAGS})
			IF(CFLAG MATCHES "^-I")
				STRING(SUBSTRING "${CFLAG}" 2 -1 INCDIR)
				LIST(APPEND GDKPIXBUF2_INCLUDE_DIRS "${INCDIR}")
				UNSET(INCDIR)
			ENDIF(CFLAG MATCHES "^-I")
		ENDFOREACH()
		SET_TARGET_PROPERTIES(GdkPixbuf2::gdkpixbuf2 PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${GDKPIXBUF2_INCLUDE_DIRS}")
	ENDIF(GDKPIXBUF2_FOUND)
ENDIF(PkgConfig_FOUND)
