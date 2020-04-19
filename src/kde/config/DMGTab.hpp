/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * DMG.hpp: Game Boy tab for rp-config.                                    *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_CONFIG_DMGTAB_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_DMGTAB_HPP__

#include "ITab.hpp"

class DMGTabPrivate;
class DMGTab : public ITab
{
	Q_OBJECT

	public:
		explicit DMGTab(QWidget *parent = nullptr);
		virtual ~DMGTab();

	private:
		typedef ITab super;
		DMGTabPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(DMGTab);
		Q_DISABLE_COPY(DMGTab)

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
		 * A combobox was changed.
		 */
		void comboBox_changed(void);
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_DMGTAB_HPP__ */
