/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * KeyManagerTab.hpp: Key Manager tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ITab.hpp"

class KeyManagerTabPrivate;
class KeyManagerTab final : public ITab
{
public:
	KeyManagerTab();
	~KeyManagerTab() final;

private:
	typedef ITab super;
	friend class KeyManagerTabPrivate;
	KeyManagerTabPrivate *const d_ptr;
public:
	RP_DISABLE_COPY(KeyManagerTab)

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
	void save(void) final;
};
