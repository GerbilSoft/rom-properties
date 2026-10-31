# glib-mkenums CMake macro
# - target: Target to add these files as dependencies.
# - h_template_file: H template file
# - c_template_file: C template file
# - out_h_file: Output H filename
# - out_c_file: Output C filename
# - ARGN: Header files to parse
FUNCTION(DO_GLIB_MKENUMS target h_template_file c_template_file out_h_file out_c_file)
	# Find glib-mkenums.
	FIND_PROGRAM(GLIB_MKENUMS NAMES glib-mkenums REQUIRED)

	# Generate the H file.
	ADD_CUSTOM_COMMAND(
		OUTPUT "${out_h_file}"
		COMMAND "${GLIB_MKENUMS}" --template "${h_template_file}" ${ARGN} >"${out_h_file}"
		MAIN_DEPENDENCY "${h_template_file}"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		DEPENDS "${h_template_file}" ${ARGN}
		)

	# Generate the C file.
	ADD_CUSTOM_COMMAND(
		OUTPUT "${out_c_file}"
		COMMAND "${GLIB_MKENUMS}" --template "${c_template_file}" ${ARGN} >"${out_c_file}"
		MAIN_DEPENDENCY "${c_template_file}"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		DEPENDS "${c_template_file}" ${ARGN}
		)

	# Create the custom target.
	ADD_CUSTOM_TARGET(${target}_mkenums DEPENDS "${out_h_file}" "${out_c_file}")
	ADD_DEPENDENCIES(${target} ${target}_mkenums)
ENDFUNCTION(DO_GLIB_MKENUMS)
