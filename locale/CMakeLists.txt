# Localization files.
PROJECT(locale)

# Find Gettext.
FIND_PACKAGE(Gettext REQUIRED)
FIND_PROGRAM(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
FIND_PROGRAM(FIND find)
FIND_PROGRAM(SORT sort)
FIND_PROGRAM(SED sed)

# Variables for xgettext.
SET(DOMAIN "rom-properties")
SET(COPYRIGHT_HOLDER "David Korth")
SET(MSGID_BUGS_ADDRESS "gerbilsoft@gerbilsoft.com")
SET(LINGUAS it)

# Convert the .po files to .mo files.
SET(LINGUAS it)
FOREACH(_lingua ${LINGUAS})
	GETTEXT_CREATE_TRANSLATIONS(${DOMAIN}.pot ALL ${_lingua}.po)
ENDFOREACH()

# Create the POTFILES file. (Requires a Unix-like `find` command.)
ADD_CUSTOM_COMMAND(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/POTFILES"
	COMMAND ${FIND} "../src/" -type f
		( -name *.c -or -name *.h -or -name *.cpp -or -name *.hpp ) | ${SORT}
		> "${CMAKE_CURRENT_BINARY_DIR}/POTFILES"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	VERBATIM
	)

# Regenerate the rom-properties.pot file.
# TODO: --add-comments=TRANSLATORS:
ADD_CUSTOM_TARGET(rom-properties.pot
	COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE}
		-o ${DOMAIN}.pot
		--default-domain=${DOMAIN}
		--add-comments
		--keyword=_ --keyword=C_:1c,2 --keyword=N_:1,2 --keyword=NC_:1c,2,3
		--keyword=NOP_ --keyword=NOP_C_:1c,2 --keyword=NOP_N_:1,2 --keyword=NOP_NC_:1c,2,3
		--language=C++
		--copyright-holder=${COPYRIGHT_HOLDER}
		--msgid-bugs-address=${MSGID_BUGS_ADDRESS}
		--files-from=${CMAKE_CURRENT_BINARY_DIR}/POTFILES
		--package-version=${VERSION_STRING}
		--package-name=${CMAKE_PROJECT_NAME}
	# Set the character set to UTF-8.
	# Reference: Reference: http://mingw-users.1079350.n2.nabble.com/Getting-rid-of-xgettext-s-quot-CHARSET-quot-warning-td5620533.html
	COMMAND ${SED} -i -e "s/CHARSET/UTF-8/" ${DOMAIN}.pot
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/POTFILES"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	VERBATIM
	)
