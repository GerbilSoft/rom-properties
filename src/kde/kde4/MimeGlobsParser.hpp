/***************************************************************************
 * ROM Properties Page shell extension. (KDE4)                             *
 * MimeGlobsParser.hpp: shared-mime-info globs parser.                     *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef _ROMPROPERTIES_KDE_KDE4_MIMEGLOBSPARSER_HPP__
#define _ROMPROPERTIES_KDE_KDE4_MIMEGLOBSPARSER_HPP__

#include <string>
#include <QtCore/QString>

class MimeGlobsParser
{
	private:
		// Static class.
		MimeGlobsParser();
		~MimeGlobsParser();

	private:
		Q_DISABLE_COPY(MimeGlobsParser);

	public:
		/**
		 * Get the MIME type for the specified filename.
		 * @param filename Filename.
		 * @return MIME type, or empty string on error.
		 */
		static std::string getMimeTypeForFilename(const QString &filename);
};

#endif /* _ROMPROPERTIES_KDE_KDE4_MIMEGLOBSPARSER_HPP__ */
