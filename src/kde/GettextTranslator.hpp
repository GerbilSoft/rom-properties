/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * GettextTranslator.hpp: QTranslator class using GNU Gettext.             *
 *                                                                         *
 * Copyright (c) 2017-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_GETTEXTTRANSLATOR_HPP__
#define __ROMPROPERTIES_KDE_GETTEXTTRANSLATOR_HPP__

#include <QTranslator>

class GettextTranslator : public QTranslator
{
	Q_OBJECT

	public:
		GettextTranslator(QObject *parent = nullptr);
		virtual ~GettextTranslator();

	private:
		typedef QTranslator super;
		Q_DISABLE_COPY(GettextTranslator);

	public:
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		// Qt5: Single virtual function that takes a plural parameter.
		QString translate(const char *context, const char *sourceText, const char *disambiguation = nullptr, int n = -1) const final;
#else
		// Qt4: Virtual function with no plurals; non-virtual function with plurals.
		QString translate(const char *context, const char *sourceText, const char *disambiguation, int n) const;
		QString translate(const char *context, const char *sourceText, const char *disambiguation = nullptr) const final;
#endif
};

#endif /* __ROMPROPERTIES_KDE_GETTEXTTRANSLATOR_HPP__ */
