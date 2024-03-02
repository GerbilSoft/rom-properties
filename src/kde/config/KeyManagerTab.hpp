/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyManagerTab.hpp: Key Manager tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ITab.hpp"

class KeyManagerTabPrivate;
class KeyManagerTab : public ITab
{
Q_OBJECT

public:
	explicit KeyManagerTab(QWidget *parent = nullptr);
	~KeyManagerTab() override;

private:
	typedef ITab super;
	KeyManagerTabPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(KeyManagerTab);
	Q_DISABLE_COPY(KeyManagerTab)

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
	 * Import keys from Wii keys.bin. (BootMii format)
	 */
	void on_actionImportWiiKeysBin_triggered(void);

	/**
	 * Import keys from Wii U otp.bin.
	 */
	void on_actionImportWiiUOtpBin_triggered(void);

	/**
	 * Import keys from 3DS boot9.bin.
	 */
	void on_actionImport3DSboot9bin_triggered(void);

	/**
	 * Import keys from 3DS aeskeydb.bin.
	 */
	void on_actionImport3DSaeskeydb_triggered(void);
};
