# Find Gnome's libnautilus-extension libraries and headers.
# If found, the following variables will be defined:
# - LibNautilusExtension_FOUND: System has ThunarX.
# - LibNautilusExtension_INCLUDE_DIRS: ThunarX include directories.
# - LibNautilusExtension_LIBRARIES: ThunarX libraries.
# - LibNautilusExtension_DEFINITIONS: Compiler switches required for using ThunarX.
# - LibNautilusExtension_EXTENSION_DIR: Extensions directory. (for installation)
#
# In addition, a target Gnome::libnautilus-extension will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

FIND_PACKAGE(PkgConfig)
IF(PkgConfig_FOUND)
	PKG_CHECK_MODULES(PC_LibNautilusExtension QUIET libnautilus-extension)
	SET(LibNautilusExtension_DEFINITIONS ${PC_LibNautilusExtension_CFLAGS_OTHER})

	FIND_PATH(LibNautilusExtension_INCLUDE_DIR libnautilus-extension/nautilus-extension-types.h
		HINTS ${PC_LibNautilusExtension_INCLUDEDIR} ${PC_LibNautilusExtension_INCLUDE_DIRS})
	FIND_LIBRARY(LibNautilusExtension_LIBRARY NAMES nautilus-extension libnautilus-extension
		HINTS ${PC_LibNautilusExtension_LIBDIR} ${PC_LibNautilusExtension_LIBRARY_DIRS})

	# Version must be set before calling FPHSA.
	SET(LibNautilusExtension_VERSION "${PC_LibNautilusExtension_VERSION}")
	MARK_AS_ADVANCED(LibNautilusExtension_VERSION)

	# Handle the QUIETLY and REQUIRED arguments and set LibNautilusExtension_FOUND to TRUE
	# if all listed variables are TRUE.
	INCLUDE(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibNautilusExtension
		FOUND_VAR LibNautilusExtension_FOUND
		REQUIRED_VARS LibNautilusExtension_LIBRARY LibNautilusExtension_INCLUDE_DIR
		VERSION_VAR LibNautilusExtension_VERSION
		)
	MARK_AS_ADVANCED(LibNautilusExtension_INCLUDE_DIR LibNautilusExtension_LIBRARY)

	SET(LibNautilusExtension_LIBRARIES ${LibNautilusExtension_LIBRARY})
	SET(LibNautilusExtension_INCLUDE_DIRS ${LibNautilusExtension_INCLUDE_DIR})

	# Create the library target.
	IF(LibNautilusExtension_FOUND)
		IF(NOT TARGET Gnome::libnautilus-extension)
			ADD_LIBRARY(Gnome::libnautilus-extension UNKNOWN IMPORTED)
		ENDIF(NOT TARGET Gnome::libnautilus-extension)
		SET_TARGET_PROPERTIES(Gnome::libnautilus-extension PROPERTIES
			IMPORTED_LOCATION "${LibNautilusExtension_LIBRARY}")
		SET_TARGET_PROPERTIES(Gnome::libnautilus-extension PROPERTIES
			COMPILE_DEFINITIONS "${LibNautilusExtension_DEFINITIONS}")

		# Add include directories from the pkgconfig's Cflags section.
		FOREACH(CFLAG ${PC_LibNautilusExtension_CFLAGS})
			IF(CFLAG MATCHES "^-I")
				STRING(SUBSTRING "${CFLAG}" 2 -1 INCDIR)
				LIST(APPEND LibNautilusExtension_INCLUDE_DIRS "${INCDIR}")
				UNSET(INCDIR)
			ENDIF(CFLAG MATCHES "^-I")
		ENDFOREACH()
		SET_TARGET_PROPERTIES(Gnome::libnautilus-extension PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${LibNautilusExtension_INCLUDE_DIRS}")

		# Extensions directory.
		PKG_GET_VARIABLE(LibNautilusExtension_EXTENSION_DIR libnautilus-extension extensiondir)
		IF(NOT LibNautilusExtension_EXTENSION_DIR)
			MESSAGE(FATAL_ERROR "LibNautilusExtension_EXTENSION_DIR isn't set.")
		ENDIF(NOT LibNautilusExtension_EXTENSION_DIR)
	ENDIF(LibNautilusExtension_FOUND)
ENDIF(PkgConfig_FOUND)
