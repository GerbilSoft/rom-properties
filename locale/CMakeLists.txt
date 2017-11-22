# Localization files.
PROJECT(locale)

# Find Gettext.
FIND_PACKAGE(Gettext REQUIRED)

SET(POTFILES "rom-properties.pot")
SET(LINGUAS it)

# Convert the .po files to .mo files.
SET(LINGUAS it)
FOREACH(_lingua ${LINGUAS})
	GETTEXT_CREATE_TRANSLATIONS(rom-properties.pot ${_lingua}.po)
ENDFOREACH()
