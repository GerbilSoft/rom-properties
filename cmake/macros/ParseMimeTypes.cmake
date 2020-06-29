# Parse MIME types from MIME type files.
# PARSE_MIME_TYPES(_result files...)
#
#  _result  - variable to store the result in
#  files... - files to parse
FUNCTION(PARSE_MIME_TYPES _result)
	# Read all files and parse the lines.
	# TODO: Eliminate duplicates?
	UNSET(${_result} PARENT_SCOPE)

	SET(_tmp "")
	FOREACH(_file ${ARGN})
		FILE(STRINGS "${_file}" _filedata)
		FOREACH(_line ${_filedata})
			STRING(REGEX REPLACE "\#.*$" "" _line "${_line}")
			STRING(REGEX REPLACE "[ \t]+$" "" _line "${_line}")
			IF(_line)
				SET(_tmp "${_tmp}${_line};")
			ENDIF(_line)
		ENDFOREACH(_line)
	ENDFOREACH(_file)

	SET(${_result} "${_tmp}" PARENT_SCOPE)
ENDFUNCTION(PARSE_MIME_TYPES)
