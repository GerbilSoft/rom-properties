#!/bin/sh
# Generate the rom-properties.pot file.
# Reference: http://www.labri.fr/perso/fleury/posts/programming/a-quick-gettext-tutorial.html

# TODO: Get version number from CMake.
PACKAGE=rom-properties
VERSION=1.2+
COPYRIGHT_HOLDER="David Korth"

find ../src -type f -and \( -name "*.c" -or -name "*.h" -or -name "*.cpp" -or -name "*.hpp" \) | sort >files.lst
xgettext --keyword=_ --keyword=C_:1c,2 --keyword=N_:1,2 --keyword=NC_:1c,2,3 \
	--keyword=NOP_ --keyword=NOP_C_:1c,2 \
	--language=C++ --add-comments \
	--package-name="${PACKAGE}" \
	--package-version="${VERSION}" \
	--copyright-holder="${COPYRIGHT_HOLDER}" \
	-o rom-properties.pot --files-from=files.lst

# Set the character set to UTF-8.
# Reference: http://mingw-users.1079350.n2.nabble.com/Getting-rid-of-xgettext-s-quot-CHARSET-quot-warning-td5620533.html
sed -i -e 's/CHARSET/UTF-8/' rom-properties.pot
