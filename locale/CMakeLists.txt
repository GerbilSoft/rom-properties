# Localization files.
PROJECT(locale)

# Find gettext.
INCLUDE(CheckGettext)
CHECK_GETTEXT()

# Find standard Unix utilities.
# On Windows, these are included with git.
FIND_PROGRAM(FIND find)
FIND_PROGRAM(SORT sort)
FIND_PROGRAM(SED sed)

# Variables for xgettext.
SET(DOMAIN "rom-properties")
SET(COPYRIGHT_HOLDER "David Korth")
SET(MSGID_BUGS_ADDRESS "gerbilsoft@gerbilsoft.com")
SET(LINGUAS de es fr pt ro ru uk)

# Installation.
INCLUDE(DirInstallPaths)

SET(_gmoFiles)
FOREACH(_lingua ${LINGUAS})
	# Convert the .po files to .mo files.
	# TODO: ${_lingua}@quot
	SET(_gmoFile ${CMAKE_CURRENT_BINARY_DIR}/${_lingua}.gmo)
	SET(_poFile ${CMAKE_CURRENT_SOURCE_DIR}/${_lingua}.po)

	ADD_CUSTOM_COMMAND(OUTPUT ${_gmoFile}
		COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -c -o ${_gmoFile} ${_poFile}
		DEPENDS ${_poFile}
		)
	SET(_gmoFiles ${_gmoFiles} ${_gmoFile})

	# Custom target to initialize a new .po file.
	# Use this after adding the language code to LINGUAS.
	# TODO: ${_lingua}@quot
	ADD_CUSTOM_TARGET(init-${_lingua}.po
		COMMAND ${GETTEXT_MSGINIT_EXECUTABLE}
			--no-translator
			--input=${DOMAIN}.pot
			--locale=${_lingua}
			--output=${_lingua}.po
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		VERBATIM
		)
	SET(INIT_ALL_PO_TARGETS ${INIT_ALL_PO_TARGETS} init-${_lingua}.po)

	# Custom target to update a .po file.
	# Use this after adding new strings to the source code.
	# TODO: ${_lingua}@quot
	IF(WIN32)
		SET(MV move)
	ELSE(WIN32)
		SET(MV mv)
	ENDIF(WIN32)
	ADD_CUSTOM_TARGET(update-${_lingua}.po
		COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE}
			--lang=${_lingua}
			${_lingua}.po ${DOMAIN}.pot
			-o ${_lingua}.new.po
		COMMAND ${MV} ${_lingua}.new.po ${_lingua}.po
		DEPENDS ${_lingua}.po
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		VERBATIM
		)
	SET(UPDATE_ALL_PO_TARGETS ${UPDATE_ALL_PO_TARGETS} update-${_lingua}.po)

	# Install the .mo file.
	INSTALL(FILES ${_gmoFile}
		DESTINATION ${DIR_INSTALL_LOCALE}/${_lingua}/LC_MESSAGES
		RENAME ${DOMAIN}.mo
		COMPONENT "i18n"
		)
ENDFOREACH()

# "ALL" targets.
# NOTE: Don't run init-all.po unless you know what you're doing!
#ADD_CUSTOM_TARGET(init-all.po DEPENDS ${INIT_ALL_PO_TARGETS})
ADD_CUSTOM_TARGET(update-all.po DEPENDS ${UPDATE_ALL_PO_TARGETS})
ADD_CUSTOM_TARGET(translations ALL DEPENDS ${_gmoFiles})

# Determine the Qt UI target.
IF(TARGET kf6_generate_ui_h)
	SET(QT_UI_TARGET kf6_generate_ui_h)
ELSEIF(TARGET kf5_generate_ui_h)
	SET(QT_UI_TARGET kf5_generate_ui_h)
ELSEIF(TARGET kde5_generate_ui_h)
	SET(QT_UI_TARGET kde5_generate_ui_h)
ELSEIF(TARGET kde4_generate_ui_h)
	SET(QT_UI_TARGET kde4_generate_ui_h)
ENDIF()

# Create the POTFILES.unsort file. (Requires a Unix-like `find` command.)
ADD_CUSTOM_COMMAND(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/POTFILES.unsort"
	COMMAND ${FIND} "../src/" -type f
		( -name *.c -or -name *.h -or -name *.cpp -or -name *.hpp )
		> "${CMAKE_CURRENT_BINARY_DIR}/POTFILES.unsort"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	VERBATIM
	)

# Create the UIHFILES.unsort file. (Requires a Unix-like `find` command.)
FILE(RELATIVE_PATH RELBINDIR ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR})
ADD_CUSTOM_COMMAND(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/UIHFILES.unsort"
	COMMAND ${FIND} "${RELBINDIR}/src/" -type f -name ui_*.h
		> "${CMAKE_CURRENT_BINARY_DIR}/UIHFILES.unsort"
	DEPENDS ${QT_UI_TARGET}
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	VERBATIM
	)

# Create the POTFILES file.
ADD_CUSTOM_COMMAND(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/POTFILES"
	COMMAND ${SORT} -u POTFILES.unsort UIHFILES.unsort > POTFILES
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/POTFILES.unsort"
		"${CMAKE_CURRENT_BINARY_DIR}/UIHFILES.unsort"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	VERBATIM
	)

# Regenerate the rom-properties.pot file.
ADD_CUSTOM_TARGET(rom-properties.pot
	COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE}
		-o ${DOMAIN}.pot
		--default-domain=${DOMAIN}
		--from-code=UTF-8
		--add-comments=tr:
		--keyword=_ --keyword=C_:1c,2 --keyword=N_:1,2 --keyword=NC_:1c,2,3
		--keyword=NOP_ --keyword=NOP_C_:1c,2 --keyword=NOP_N_:1,2 --keyword=NOP_NC_:1c,2,3
		--keyword=translate:1c,2
		--language=C++
		--exclude-file=exclusions.po
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
