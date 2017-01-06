# Find GTK3 libraries and headers.
# If found, the following variables will be defined:
# - GTK3_FOUND: System has GTK3.
# - GTK3_INCLUDE_DIRS: GTK3 include directories.
# - GTK3_LIBRARIES: GTK3 libraries.
# - GTK3_DEFINITIONS: Compiler switches required for using GTK3.
#
# In addition, a target Gtk3::gtk3 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

FIND_PACKAGE(PkgConfig)
IF(PkgConfig_FOUND)
	PKG_CHECK_MODULES(PC_GTK3 QUIET gtk+-3.0)
	SET(GTK3_DEFINITIONS ${PC_GTK3_CFLAGS_OTHER})

	FIND_PATH(GTK3_INCLUDE_DIR gtk/gtk.h
		HINTS ${PC_GTK3_INCLUDEDIR} ${PC_GTK3_INCLUDE_DIRS})
	FIND_LIBRARY(GTK3_LIBRARY NAMES gtk-3 libgtk-3
		HINTS ${PC_GTK3_LIBDIR} ${PC_GTK3_LIBRARY_DIRS})

	# Version must be set before calling FPHSA.
	SET(GTK3_VERSION "${PC_GTK3_VERSION}")
	MARK_AS_ADVANCED(GTK3_VERSION)

	# Handle the QUIETLY and REQUIRED arguments and set GTK3_FOUND to TRUE
	# if all listed variables are TRUE.
	INCLUDE(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(GTK3
		FOUND_VAR GTK3_FOUND
		REQUIRED_VARS GTK3_LIBRARY GTK3_INCLUDE_DIR
		VERSION_VAR GTK3_VERSION
		)
	MARK_AS_ADVANCED(GTK3_INCLUDE_DIR GTK3_LIBRARY)

	SET(GTK3_LIBRARIES ${GTK3_LIBRARY})
	SET(GTK3_INCLUDE_DIRS ${GTK3_INCLUDE_DIR})

	# Create the library target.
	IF(GTK3_FOUND)
		IF(NOT TARGET Gtk3::gtk3)
			ADD_LIBRARY(Gtk3::gtk3 UNKNOWN IMPORTED)
		ENDIF(NOT TARGET Gtk3::gtk3)
		SET_TARGET_PROPERTIES(Gtk3::gtk3 PROPERTIES
			IMPORTED_LOCATION "${GTK3_LIBRARY}")
		SET_TARGET_PROPERTIES(Gtk3::gtk3 PROPERTIES
			COMPILE_DEFINITIONS "${GTK3_DEFINITIONS}")

		# Add include directories from the pkgconfig's Cflags section.
		FOREACH(CFLAG ${PC_GTK3_CFLAGS})
			IF(CFLAG MATCHES "^-I")
				STRING(SUBSTRING "${CFLAG}" 2 -1 INCDIR)
				LIST(APPEND GTK3_INCLUDE_DIRS "${INCDIR}")
				UNSET(INCDIR)
			ENDIF(CFLAG MATCHES "^-I")
		ENDFOREACH()
		SET_TARGET_PROPERTIES(Gtk3::gtk3 PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${GTK3_INCLUDE_DIRS}")
	ENDIF(GTK3_FOUND)
ENDIF(PkgConfig_FOUND)
