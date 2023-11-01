/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * OptionsTab.hpp: Options tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ITab.hpp"

class OptionsTabPrivate;
class OptionsTab : public ITab
{
Q_OBJECT

public:
	explicit OptionsTab(QWidget *parent = nullptr);
	~OptionsTab() override;

private:
	typedef ITab super;
	OptionsTabPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(OptionsTab);
	Q_DISABLE_COPY(OptionsTab)

protected:
	// State change event. (Used for switching the UI language at runtime.)
	void changeEvent(QEvent *event) final;

public slots:
	/**
	 * Reset the configuration.
	 */
	void reset(void) final;

	/**
	 * Load the default configuration.
	 * This does NOT save, and will only emit modified()
	 * if it's different from the current configuration.
	 */
	void loadDefaults(void) final;

	/**
	 * Save the configuration.
	 * @param pSettings QSettings object.
	 */
	void save(QSettings *pSettings) final;

protected slots:
	/**
	 * An option was changed.
	 */
	void optionChanged_slot(void);
};
