/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * GettextTranslator.cpp: QTranslator class using GNU Gettext.             *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GettextTranslator.hpp"

GettextTranslator::GettextTranslator(QObject *parent)
	: super(parent)
{ }

QString GettextTranslator::translate(const char *context,
	const char *sourceText, const char *disambiguation, int n) const
{
	// FIXME: Make use of disambiguation.
	Q_UNUSED(disambiguation)

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	// Qt5: This function handles non-plurals as well as plurals.
	if (n >= 0)
#endif
	{
		// NOTE: gettext() requires two message IDs for plurals.
		// Qt only has one, since it does all plural processing in the translation.
		const char *const txt = dnpgettext_expr(RP_I18N_DOMAIN, context, sourceText, sourceText, n);
		if (txt == sourceText) {
			// No translation is available from gettext.
			return {};
		}
		return QString::fromUtf8(txt);
	}

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	// Qt5: No plurals here.
	const char *const txt = dpgettext_expr(RP_I18N_DOMAIN, context, sourceText);
	if (txt == sourceText) {
		// No translation is available from gettext.
		return {};
	}
	return QString::fromUtf8(txt);
#endif
}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
QString GettextTranslator::translate(const char *context,
	const char *sourceText, const char *disambiguation) const
{
	// FIXME: Make use of disambiguation.
	Q_UNUSED(disambiguation)

	const char *const txt = dpgettext_expr(RP_I18N_DOMAIN, context, sourceText);
	if (txt == sourceText) {
		// No translation is available from gettext.
		return {};
	}
	return QString::fromUtf8(txt);
}
#endif
