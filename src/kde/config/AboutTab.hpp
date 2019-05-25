/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * AboutTab.hpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_CONFIG_ABOUTTAB_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_ABOUTTAB_HPP__

#include "ITab.hpp"

class AboutTabPrivate;
class AboutTab : public ITab
{
	Q_OBJECT

	public:
		explicit AboutTab(QWidget *parent = nullptr);
		virtual ~AboutTab();

	private:
		typedef ITab super;
		AboutTabPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(AboutTab);
		Q_DISABLE_COPY(AboutTab)

	public:
		/**
		 * Does this tab have defaults available?
		 * If so, the "Defaults" button will be enabled.
		 * Otherwise, it will be disabled.
		 *
		 * AboutTab sets this to false.
		 *
		 * @return True to enable; false to disable.
		 */
		bool hasDefaults(void) const final { return false; }

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
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_ABOUTTAB_HPP__ */
