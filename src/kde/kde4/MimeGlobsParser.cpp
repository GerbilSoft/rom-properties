/***************************************************************************
 * ROM Properties Page shell extension. (KDE4)                             *
 * MimeGlobsParser.cpp: shared-mime-info globs parser.                     *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.kde4.h"

#include "MimeGlobsParser.hpp"
#include "librpbase/ctypex.h"

// C++ STL classes.
using std::string;

// Qt includes.
#include <QtCore/QRegExp>

/**
 * Get the MIME type for the specified filename.
 * @param filename Filename.
 * @return MIME type, or empty string on error.
 */
string MimeGlobsParser::getMimeTypeForFilename(const QString &filename)
{
	// TODO: Cache the database instead of re-reading it every time?
	std::string mimeType;

	// Get the file extension.
	// TODO: suffix() or completeSuffix(), or both?
	// Using suffix() for now.
	// /media/sf_D_DRIVE/STUFF/mr_128k_huti.pvr
	QString fileExt = QChar(L'.') + QFileInfo(filename).suffix();

	// Open /usr/share/mime/globs.
	// TODO: Check ~/.local/share/mime/globs first.
	QFile f_globs(QString::fromUtf8(CMAKE_INSTALL_PREFIX "/share/mime/globs"));
	if (!f_globs.open(QIODevice::ReadOnly | QIODevice::Text)) {
		// Unable to open the globs file.
		return mimeType;
	}

	// Read each line.
	// NOTE: We're stopping at the first match.
	// TODO: Keep going for longest match, e.g. for *.tar.gz?
	// TODO: Use globs2 to get the weight values?
	// FIXME: QRegExp might be slow, even in wildcard mode.
	QRegExp re;
	re.setPatternSyntax(QRegExp::Wildcard);
	re.setCaseSensitivity(Qt::CaseInsensitive);

	while (!f_globs.atEnd()) {
		QByteArray line = f_globs.readLine();
		if (line.isEmpty() || line[0] == '#' || ISSPACE(line[0])) {
			// Empty line, or it starts with a comment.
			continue;
		}

		// Find the ':'.
		int idx = line.indexOf(':');
		if (idx < 0 || idx >= line.size()-2) {
			// No colon, or the colon is at the end of the line.
			continue;
		}

		// Get the glob for QRegExp.
		// NOTE: Line ends with '\n', so subtract *2*, not just 1.
		// (The initial 1 accounts for the colon.)
		QString glob = QString::fromUtf8(&line.constData()[idx+1], line.size()-idx-2);
		re.setPattern(glob);
		if (re.exactMatch(fileExt)) {
			mimeType.assign(line.constData(), idx);
			break;
		}
	}

	f_globs.close();
	return mimeType;
}
