/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * SystemsTab.hpp: Systems tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_CONFIG_SYSTEMSTAB_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_SYSTEMSTAB_HPP__

#include "ITab.hpp"

class SystemsTabPrivate;
class SystemsTab : public ITab
{
	Q_OBJECT

	public:
		explicit SystemsTab(QWidget *parent = nullptr);
		~SystemsTab() final;

	private:
		typedef ITab super;
		SystemsTabPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(SystemsTab);
		Q_DISABLE_COPY(SystemsTab)

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

#endif /* __ROMPROPERTIES_KDE_CONFIG_SYSTEMSTAB_HPP__ */
