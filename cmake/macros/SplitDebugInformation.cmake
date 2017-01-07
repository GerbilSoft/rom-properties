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
	INCLUDE(CMakeFindBinUtils)
	IF(NOT CMAKE_OBJCOPY)
		MESSAGE(WARNING "'objcopy' was not found; debug information will not be split.")
		SET(INSTALL_DEBUG OFF CACHE "" INTERNAL FORCE)
	ELSEIF(NOT CMAKE_STRIP)
		MESSAGE(WARNING "'strip' was not found; debug information will not be split.")
		SET(INSTALL_DEBUG OFF CACHE "" INTERNAL FORCE)
	ENDIF()
ENDIF(NOT MSVC)

FUNCTION(SPLIT_DEBUG_INFORMATION _target)
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
	# Handle target prefixes if not overridden.
	# NOTE: Cannot easily use the TYPE property in a generator expression...
	GET_PROPERTY(TARGET_TYPE TARGET ${_target} PROPERTY TYPE)
	SET(PREFIX_EXPR_1 "$<$<STREQUAL:$<TARGET_PROPERTY:${_target},PREFIX>,>:${CMAKE_${TARGET_TYPE}_PREFIX}>")
	SET(PREFIX_EXPR_2 "$<$<NOT:$<STREQUAL:$<TARGET_PROPERTY:${_target},PREFIX>,>>:$<TARGET_PROPERTY:${_target},PREFIX>>")
	SET(PREFIX_EXPR_FULL "${PREFIX_EXPR_1}${PREFIX_EXPR_2}")

	# If a custom OUTPUT_NAME was specified, use it.
	SET(OUTPUT_NAME_EXPR_1 "$<$<STREQUAL:$<TARGET_PROPERTY:${_target},OUTPUT_NAME>,>:${_target}>")
	SET(OUTPUT_NAME_EXPR_2 "$<$<NOT:$<STREQUAL:$<TARGET_PROPERTY:${_target},OUTPUT_NAME>,>>:$<TARGET_PROPERTY:${_target},OUTPUT_NAME>>")
	SET(OUTPUT_NAME_EXPR "${OUTPUT_NAME_EXPR_1}${OUTPUT_NAME_EXPR_2}")

	SET(OUTPUT_NAME_FULL "${PREFIX_EXPR_FULL}${OUTPUT_NAME_EXPR}$<TARGET_PROPERTY:${_target},POSTFIX>")

	SET(SPLITDEBUG_SOURCE "$<TARGET_FILE:${_target}>")
	SET(SPLITDEBUG_TARGET "$<TARGET_FILE_DIR:${_target}>/${OUTPUT_NAME_FULL}.debug")

	# NOTE: objcopy --strip-debug does NOT fully
	# strip the binary; two sections are left:
	# - .symtab: Symbol table.
	# - .strtab: String table.
	# These sections are split into the .debug file, so there's
	# no reason to keep them in the executable.
	ADD_CUSTOM_COMMAND(TARGET ${_target} POST_BUILD
		COMMAND ${CMAKE_OBJCOPY} --only-keep-debug
			${SPLITDEBUG_SOURCE} ${SPLITDEBUG_TARGET}
		COMMAND ${CMAKE_STRIP}
			${SPLITDEBUG_SOURCE}
		COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink="${SPLITDEBUG_TARGET}"
			${SPLITDEBUG_SOURCE}
		)

	# Set the target property to allow installation.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES PDB ${SPLITDEBUG_TARGET})

	# Make sure the file is deleted on `make clean`.
	SET_PROPERTY(DIRECTORY APPEND
		PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${SPLITDEBUG_TARGET})
ENDIF(SPLIT_OK)
ENDFUNCTION(SPLIT_DEBUG_INFORMATION)
