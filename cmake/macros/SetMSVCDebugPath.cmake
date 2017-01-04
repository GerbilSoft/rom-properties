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
MACRO(SET_MSVC_DEBUG_PATH _target)
IF(MSVC)
	# CMake seems to use weird settings for the PDB file.
	# (at least version 2.8.12.2; 3.0.0 might be different)
	STRING(REGEX REPLACE
		"/Fd<OBJECT_DIR>/"
		"/Fd<TARGET_PDB>"
		CMAKE_C_COMPILE_OBJECT "${CMAKE_C_COMPILE_OBJECT}")
	STRING(REGEX REPLACE
		"/Fd<OBJECT_DIR>/"
		"/Fd<TARGET_PDB>"
		CMAKE_CXX_COMPILE_OBJECT "${CMAKE_CXX_COMPILE_OBJECT}")

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

	SET(OUTPUT_NAME_EXPR_1 "$<TARGET_PROPERTY:${_target},PREFIX>$<TARGET_PROPERTY:${_target},OUTPUT_NAME>$<TARGET_PROPERTY:${_target},POSTFIX>")
	SET(OUTPUT_NAME_EXPR_2 "$<$<STREQUAL:$<TARGET_PROPERTY:${_target},OUTPUT_NAME>,>:$<TARGET_PROPERTY:${_target},PREFIX>${_target}$<TARGET_PROPERTY:${_target},POSTFIX>>")

	# Set the target PDB filename.
	SET_TARGET_PROPERTIES(${_target}
		PROPERTIES PDB "$<TARGET_FILE_DIR:${_target}>/${OUTPUT_NAME_EXPR_1}${OUTPUT_NAME_EXPR_2}.pdb"
		)

	UNSET(OUTPUT_NAME_EXPR_1)
	UNSET(OUTPUT_NAME_EXPR_2)
ENDIF(MSVC)
ENDMACRO(SET_MSVC_DEBUG_PATH)
