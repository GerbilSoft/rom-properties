/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ConfigDialog.hpp: Configuration dialog.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
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
