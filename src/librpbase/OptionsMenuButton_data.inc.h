/********************************************************************************
 * ROM Properties Page shell extension. (Win32)                                 *
 * OptionsMenuButton_data.h: Common data for OptionsMenuButton implementations. *
 * NOTE: Should only be included by the OptionsMenuButton implementation.       *
 *                                                                              *
 * Copyright (c) 2017-2026 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                                    *
 ********************************************************************************/

#pragma once
#include "OptionsMenuButton_decl.h"

// libi18n
#include "libi18n/i18n.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/** Standard actions **/
struct option_menu_action_t {
	const char *desc;
	int id;
};
static const option_menu_action_t OptionsMenuButton_stdacts[] = {
	{NOP_C_("OptionsMenuButton|StdActs", "Export to Text..."),	OPTION_EXPORT_TEXT},
	{NOP_C_("OptionsMenuButton|StdActs", "Export to JSON..."),	OPTION_EXPORT_JSON},
	{NOP_C_("OptionsMenuButton|StdActs", "Copy as Text"),		OPTION_COPY_TEXT},
	{NOP_C_("OptionsMenuButton|StdActs", "Copy as JSON"),		OPTION_COPY_JSON},
};

#ifdef __cplusplus
}
#endif
