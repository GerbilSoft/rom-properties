/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ConfigDialog.hpp: Configuration dialog.                                 *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "libwin32common/RpWin32_sdk.h"

class ConfigDialogPrivate;
class ConfigDialog
{
public:
	ConfigDialog();
	~ConfigDialog();

private:
	friend class ConfigDialogPrivate;
	ConfigDialogPrivate *const d_ptr;
public:
	RP_DISABLE_COPY(ConfigDialog)

public:
	/**
	 * Run the property sheet.
	 * @return PropertySheet() return value.
	 */
	INT_PTR exec(void);
};
