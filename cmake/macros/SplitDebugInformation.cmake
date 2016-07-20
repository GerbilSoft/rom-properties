# Split debug information from an executable into a separate file.
# SPLIT_DEBUG_INFORMATION(EXE_TARGET)
#
# References:
# - http://cmake.3232098.n2.nabble.com/Save-stripped-debugging-information-td6819195.html
# - http://sourceware.org/bugzilla/show_bug.cgi?id=14527
#   - If debug symbols are stripped before .gnu_debuglink is added,
#     the section will be truncated to .gnu_deb, and hence won't
#     be recognized by gdb.
# - FIXME: If the above .gnu_debuglink workaround is used, Windows XP
#   and Windows 7 will claim that the executable isn't a valid Win32
#   executable. (Wine ignores it and works fine!)
#
IF(NOT MSVC)
	# CMake automatically finds objcopy and strip as
	# part of its toolchain initialization.
	IF(NOT CMAKE_OBJCOPY)
		MESSAGE(WARNING "'objcopy' was not found; debug information will not be split.")
	ELSEIF(NOT CMAKE_STRIP)
		MESSAGE(WARNING "'strip' was not found; debug information will not be split.")
	ENDIF()
ENDIF(NOT MSVC)

MACRO(SPLIT_DEBUG_INFORMATION _target)
SET(SPLIT_OK 1)
IF(MSVC)
	# MSVC splits debug information by itself.
	SET(SPLIT_OK 0)
ELSEIF(NOT CMAKE_OBJCOPY)
	# 'objcopy' is missing.
	SET(SPLIT_OK 0)
ELSEIF(NOT CMAKE_STRIP)
	# 'strip' is missing.
	SET(SPLIT_OK 0)
ENDIF()

IF(SPLIT_OK)
	# NOTE: $<TARGET_FILE:gcbanner> is preferred,
	# but this doesn't seem to work on Ubuntu 10.04.
	# (cmake_2.8.0-5ubuntu1_i386)
	GET_TARGET_PROPERTY(SPLITDEBUG_LOCATION ${_target} LOCATION)
	GET_FILENAME_COMPONENT(SPLITDEBUG_DIRECTORY ${SPLITDEBUG_LOCATION} DIRECTORY)
	SET(SPLITDEBUG_FILENAME "${SPLITDEBUG_DIRECTORY}/${_target}.debug")

	# NOTE: objcopy --strip-debug does NOT fully
	# strip the binary; two sections are left:
	# - .symtab: Symbol table.
	# - .strtab: String table.
	# These sections are split into the .debug file, so there's
	# no reason to keep them in the executable.
	ADD_CUSTOM_COMMAND(TARGET ${_target} POST_BUILD
		COMMAND ${CMAKE_OBJCOPY} --only-keep-debug
			${SPLITDEBUG_LOCATION} ${SPLITDEBUG_FILENAME}
		COMMAND ${CMAKE_STRIP}
			${SPLITDEBUG_LOCATION}
		COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink="${_target}.debug"
			${SPLITDEBUG_LOCATION}
		)

	# Set the target property to allow installation.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES PDB ${SPLITDEBUG_FILENAME})

	UNSET(SPLITDEBUG_LOCATION)
	UNSET(SPLITDEBUG_DIRECTORY)
	UNSET(SPLITDEBUG_FILENAME)
ENDIF(SPLIT_OK)
ENDMACRO(SPLIT_DEBUG_INFORMATION)
