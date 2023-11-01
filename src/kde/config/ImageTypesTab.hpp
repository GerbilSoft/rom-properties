/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ImageTypesTab.hpp: Image Types tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ITab.hpp"

class ImageTypesTabPrivate;
class ImageTypesTab : public ITab
{
Q_OBJECT

public:
	explicit ImageTypesTab(QWidget *parent = nullptr);
	~ImageTypesTab() override;

private:
	typedef ITab super;
	ImageTypesTabPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(ImageTypesTab);
	Q_DISABLE_COPY(ImageTypesTab)

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
	 * A QComboBox index has changed.
	 * Check the QComboBox's "rp-config.cbid" property for the cbid.
	 */
	void cboImageType_currentIndexChanged(void);
};
