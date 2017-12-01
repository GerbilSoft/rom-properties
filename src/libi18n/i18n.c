/***************************************************************************
 * ROM Properties Page shell extension. (libi18n)                          *
 * i18n.c: Internationalization support code.                              *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

#include "i18n.h"

/**
 * Initialize the internationalization subsystem.
 * @return 0 on success; non-zero on error.
 */
int rp_i18n_init(void)
{
#ifdef DIR_INSTALL_LOCALE
	static const char dirname[] = DIR_INSTALL_LOCALE;
#else
	// Use the current directory.
	// TODO: On Windows, use the DLL directory.
	static const char dirname[] = "locale";
#endif

	const char *base = bindtextdomain(RP_I18N_DOMAIN, dirname);
	if (!base) {
		// bindtextdomain() failed.
		return -1;
	}

	// Initialized.
	return 0;
}
