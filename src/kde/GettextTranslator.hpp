/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * GettextTranslator.hpp: QTranslator class using GNU Gettext.             *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <QtCore/QTranslator>

class GettextTranslator : public QTranslator
{
Q_OBJECT

public:
	explicit GettextTranslator(QObject *parent = nullptr);

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
