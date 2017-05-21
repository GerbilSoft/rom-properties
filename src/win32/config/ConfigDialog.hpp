/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ConfigDialog.hpp: Configuration dialog.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_CONFIG_CONFIGDIALOG_HPP__
#define __ROMPROPERTIES_WIN32_CONFIG_CONFIGDIALOG_HPP__

#include "librpbase/common.h"
#include "libwin32common/RpWin32_sdk.h"

class ConfigDialogPrivate;
class ConfigDialog
{
	public:
		ConfigDialog();
		~ConfigDialog();

	private:
		RP_DISABLE_COPY(ConfigDialog)
	private:
		friend class ConfigDialogPrivate;
		ConfigDialogPrivate *const d_ptr;

	public:
		/**
		 * Run the property sheet.
		 * @return PropertySheet() return value.
		 */
		INT_PTR exec(void);
};

#endif /* __ROMPROPERTIES_WIN32_CONFIG_CONFIGDIALOGPRIVATE_HPP__ */
