/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * OptionsTab.hpp: Options tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ITab.hpp"

class OptionsTabPrivate;
class OptionsTab final : public ITab
{
public:
	OptionsTab();
	~OptionsTab() final;

private:
	typedef ITab super;
	RP_DISABLE_COPY(OptionsTab)
private:
	friend class OptionsTabPrivate;
	OptionsTabPrivate *const d_ptr;

public:
	/**
	 * Create the HPROPSHEETPAGE for this tab.
	 *
	 * NOTE: This function can only be called once.
	 * Subsequent invocations will return nullptr.
	 *
	 * @return HPROPSHEETPAGE.
	 */
	HPROPSHEETPAGE getHPropSheetPage(void) final;

	/**
	 * Reset the contents of this tab.
	 */
	void reset(void) final;

	/**
	 * Load the default configuration.
	 * This does NOT save, and will only emit modified()
	 * if it's different from the current configuration.
	 */
	void loadDefaults(void) final;

	/**
	 * Save the contents of this tab.
	 */
	void save(void) final;
};
