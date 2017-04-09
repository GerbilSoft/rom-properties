# Find a library using PkgConfig.
#
# Required variables:
# - _prefix: Variable prefix.
# - _prefix: pkgconfig package name. (Used for variable prefixes.)
# - _include: Include file to test.
# - _library: Library name, without the leading "lib".
# - _target: Name for the imported target.
#
# If found, the following variables will be defined:
# - ${_prefix}_FOUND: System has the package.
# - ${_prefix}_INCLUDE_DIRS: Package include directories.
# - ${_prefix}_LIBRARIES: Package libraries.
# - ${_prefix}_DEFINITIONS: Compiler switches required for using the package.
# - ${_prefix}_VERSION: Package version.
#
# In addition, if a package has extensiondir or extensionsdir set,
# the appropriate ${_prefix}_* variable will also be set.
#
# A target with the name ${_target} will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#
FUNCTION(FIND_LIBRARY_PKG_CONFIG _prefix _pkgconf _include _library _target)
	FIND_PACKAGE(PkgConfig)
	IF(PKG_CONFIG_FOUND)
		PKG_CHECK_MODULES(PC_${_prefix} QUIET ${_pkgconf})
		SET(${_prefix}_DEFINITIONS ${PC_${_prefix}_CFLAGS_OTHER})

		FIND_PATH(${_prefix}_INCLUDE_DIR ${_include}
			HINTS ${PC_${_prefix}_INCLUDEDIR} ${PC_${_prefix}_INCLUDE_DIRS})
		FIND_LIBRARY(${_prefix}_LIBRARY NAMES ${_library} lib${_library}
			HINTS ${PC_${_prefix}_LIBDIR} ${PC_${_prefix}_LIBRARY_DIRS})

		# Version must be set before calling FPHSA.
		SET(${_prefix}_VERSION ${PC_${_prefix}_VERSION})
		MARK_AS_ADVANCED(${_prefix}_VERSION)

		# Handle the QUIETLY and REQUIRED arguments and set
		# ${_prefix}_FOUND to TRUE if all listed variables
		# are TRUE.
		SET(${_prefix}_PREV_FOUND ${_prefix}_FOUND)
		INCLUDE(FindPackageHandleStandardArgs)
		FIND_PACKAGE_HANDLE_STANDARD_ARGS(${_prefix}
			FOUND_VAR ${_prefix}_FOUND
			REQUIRED_VARS ${_prefix}_LIBRARY ${_prefix}_INCLUDE_DIR
			VERSION_VAR ${_prefix}_VERSION
			)
		MARK_AS_ADVANCED(${_prefix}_INCLUDE_DIR ${_prefix}_LIBRARY)

		SET(${_prefix}_LIBRARIES ${${_prefix}_LIBRARY})
		SET(${_prefix}_INCLUDE_DIRS ${${_prefix}_INCLUDE_DIR})

		# Create the library target.
		IF(${_prefix}_FOUND)
			IF(NOT TARGET ${_target})
				ADD_LIBRARY(${_target} UNKNOWN IMPORTED)
			ENDIF(NOT TARGET ${_target})
			SET_TARGET_PROPERTIES(${_target} PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${${_prefix}_INCLUDE_DIRS}")
			SET_TARGET_PROPERTIES(${_target} PROPERTIES
				IMPORTED_LOCATION "${${_prefix}_LIBRARY}")
			SET_TARGET_PROPERTIES(${_target} PROPERTIES
				COMPILE_DEFINITIONS "${${_prefix}_DEFINITIONS}")

			# Add include directories from the pkgconfig's Cflags section.
			FOREACH(CFLAG ${PC_${_prefix}_CFLAGS})
				IF(CFLAG MATCHES "^-I")
					STRING(SUBSTRING "${CFLAG}" 2 -1 INCDIR)
					LIST(APPEND ${_prefix}_INCLUDE_DIRS "${INCDIR}")
					UNSET(INCDIR)
				ENDIF(CFLAG MATCHES "^-I")
			ENDFOREACH()
			SET_TARGET_PROPERTIES(${_target} PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${${_prefix}_INCLUDE_DIRS}")

			IF(COMMAND PKG_GET_VARIABLE)
				IF(NOT ${_prefix}_PREV_FOUND)
					MESSAGE(STATUS "Found ${_prefix}: ${${_prefix}_LIBRARY} (version ${${_prefix}_VERSION})")
				ENDIF(NOT ${_prefix}_PREV_FOUND)
				# Check for extensiondir and/or extensionsdir.
				PKG_GET_VARIABLE(${_prefix}_EXTENSION_DIR ${_pkgconf} extensiondir)
				PKG_GET_VARIABLE(${_prefix}_EXTENSIONS_DIR ${_pkgconf} extensionsdir)
			ENDIF(COMMAND PKG_GET_VARIABLE)

			# Replace hard-coded prefixes.
			INCLUDE(ReplaceHardcodedPrefix)
			FOREACH(VAR EXTENSION_DIR EXTENSIONS_DIR)
				REPLACE_HARDCODED_PREFIX(${_prefix}_${VAR} "${PC_${_prefix}_PREFIX}")
			ENDFOREACH()

			# Export variables.
			FOREACH(VAR FOUND INCLUDE_DIRS LIBRARY LIBRARIES DEFINITIONS EXTENSION_DIR EXTENSIONS_DIR VERSION)
				SET(${_prefix}_${VAR} ${${_prefix}_${VAR}} CACHE INTERNAL "${_prefix}_${VAR}")
			ENDFOREACH()
		ELSE(${_prefix}_FOUND)
			MESSAGE(STATUS "NOT FOUND: ${_prefix}")
		ENDIF(${_prefix}_FOUND)
	ELSE(PKG_CONFIG_FOUND)
		MESSAGE(STATUS "NOT FOUND: PkgConfig - cannot search for ${_prefix}")
	ENDIF(PKG_CONFIG_FOUND)
ENDFUNCTION()
