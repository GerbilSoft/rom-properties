#!/bin/sh
# Generate the rom-properties.pot file.
# Reference: http://www.labri.fr/perso/fleury/posts/programming/a-quick-gettext-tutorial.html

# TODO: Get version number from CMake.
DOMAIN=rom-properties
PACKAGE=rom-properties
COPYRIGHT_HOLDER="David Korth"
MSGID_BUGS_ADDRESS="gerbilsoft@gerbilsoft.com"
VERSION=1.2+

find ../src -type f -and \( -name "*.c" -or -name "*.h" -or -name "*.cpp" -or -name "*.hpp" \) | sort >files.lst
xgettext -o rom-properties.pot \
	--default-domain="${DOMAIN}" \
	--add-comments=tr: \
	--keyword=_ --keyword=C_:1c,2 --keyword=N_:1,2 --keyword=NC_:1c,2,3 \
	--keyword=NOP_ --keyword=NOP_C_:1c,2 --keyword=NOP_N_:1,2 --keyword=NOP_NC_:1c,2,3 \
	--language=C++ \
	--copyright-holder="${COPYRIGHT_HOLDER}" \
	--msgid-bugs-address="${MSGID_BUGS_ADDRESS}" \
	--files-from=files.lst \
	--package-version="${VERSION}" \
	--package-name="${PACKAGE}" \

# Set the character set to UTF-8. Reference: http://mingw-users.1079350.n2.nabble.com/Getting-rid-of-xgettext-s-quot-CHARSET-quot-warning-td5620533.html
sed -i -e 's/CHARSET/UTF-8/' rom-properties.pot
