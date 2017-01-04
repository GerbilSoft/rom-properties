# Find ThunarX libraries and headers. (GTK+ 2.x version)
# If found, the following variables will be defined:
# - ThunarX2_FOUND: System has ThunarX.
# - ThunarX2_INCLUDE_DIRS: ThunarX include directories.
# - ThunarX2_LIBRARIES: ThunarX libraries.
# - ThunarX2_DEFINITIONS: Compiler switches required for using ThunarX.
# - ThunarX2_EXTENSIONS_DIR: Extensions directory. (for installation)
#
# In addition, a target Xfce::thunarx-2 will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

FIND_PACKAGE(PkgConfig)
IF(PkgConfig_FOUND)
	PKG_CHECK_MODULES(PC_ThunarX2 QUIET thunarx-2)
	SET(ThunarX2_DEFINITIONS ${PC_THUNARX_CFLAGS_OTHER})

	FIND_PATH(ThunarX2_INCLUDE_DIR thunarx/thunarx.h
		HINTS ${PC_ThunarX2_INCLUDEDIR} ${PC_ThunarX2_INCLUDE_DIRS})
	FIND_LIBRARY(ThunarX2_LIBRARY NAMES thunarx-2 libthunarx-2
		HINTS ${PC_ThunarX2_LIBDIR} ${PC_ThunarX2_LIBRARY_DIRS})

	# Version must be set before calling FPHSA.
	SET(ThunarX2_VERSION "${PC_ThunarX2_VERSION}")
	MARK_AS_ADVANCED(ThunarX2_VERSION)

	# Handle the QUIETLY and REQUIRED arguments and set ThunarX2_FOUND to TRUE
	# if all listed variables are TRUE.
	INCLUDE(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(ThunarX2
		FOUND_VAR ThunarX2_FOUND
		REQUIRED_VARS ThunarX2_LIBRARY ThunarX2_INCLUDE_DIR
		VERSION_VAR ThunarX2_VERSION
		)
	MARK_AS_ADVANCED(ThunarX2_INCLUDE_DIR ThunarX2_LIBRARY)

	SET(ThunarX2_LIBRARIES ${ThunarX2_LIBRARY})
	SET(ThunarX2_INCLUDE_DIRS ${ThunarX2_INCLUDE_DIR})

	# Create the library target.
	IF(ThunarX2_FOUND)
		IF(NOT TARGET Xfce::thunarx-2)
			ADD_LIBRARY(Xfce::thunarx-2 UNKNOWN IMPORTED)
		ENDIF(NOT TARGET Xfce::thunarx-2)
		SET_TARGET_PROPERTIES(Xfce::thunarx-2 PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${ThunarX2_INCLUDE_DIRS}")
		SET_TARGET_PROPERTIES(Xfce::thunarx-2 PROPERTIES
			IMPORTED_LOCATION "${ThunarX2_LIBRARY}")
		SET_TARGET_PROPERTIES(Xfce::thunarx-2 PROPERTIES
			COMPILE_DEFINITIONS "${ThunarX2_DEFINITIONS}")

		# Add include directories from the pkgconfig's Cflags section.
		FOREACH(CFLAG ${PC_ThunarX2_CFLAGS})
			IF(CFLAG MATCHES "^-I")
				STRING(SUBSTRING "${CFLAG}" 2 -1 INCDIR)
				LIST(APPEND ThunarX2_INCLUDE_DIRS "${INCDIR}")
				UNSET(INCDIR)
			ENDIF(CFLAG MATCHES "^-I")
		ENDFOREACH()
		SET_TARGET_PROPERTIES(Xfce::thunarx-2 PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${ThunarX2_INCLUDE_DIRS}")

		# Extensions directory.
		PKG_GET_VARIABLE(ThunarX2_EXTENSIONS_DIR thunarx-2 extensionsdir)
		IF(NOT ThunarX2_EXTENSIONS_DIR)
			MESSAGE(FATAL_ERROR "ThunarX2_EXTENSIONS_DIR isn't set.")
		ENDIF(NOT ThunarX2_EXTENSIONS_DIR)
	ENDIF(ThunarX2_FOUND)
ENDIF(PkgConfig_FOUND)
