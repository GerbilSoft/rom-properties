/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpProgressBar.hpp: QProgressBar subclass with error status support.     *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpProgressBar.hpp"

/**
 * Set the error state.
 * @param error True if error; false if not.
 */
void RpProgressBar::setError(bool error)
{
	if (error == m_error)
		return;
	m_error = error;

	// Set the palette.
	if (error) {
		// TODO: Color scheme matching?
		QPalette pal = this->palette();
		pal.setColor(QPalette::Highlight, QColor(Qt::red));
		this->setPalette(pal);
	} else {
		this->setPalette(this->style()->standardPalette());
	}
}
