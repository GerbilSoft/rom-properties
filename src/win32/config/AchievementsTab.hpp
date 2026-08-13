/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * AchievementsTab.hpp: Achievements tab for rp-config.                    *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ITab.hpp"

class AchievementsTabPrivate;
class AchievementsTab final : public ITab
{
public:
	AchievementsTab();
	~AchievementsTab() final;

private:
	typedef ITab super;
	friend class AchievementsTabPrivate;
	AchievementsTabPrivate *const d_ptr;
public:
	RP_DISABLE_COPY(AchievementsTab)

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
	 * Save the contents of this tab.
	 */
	void save(void) final {}		// Nothing to do here.
};
