/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ITab.hpp: Configuration tab interface.                                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Qt includes.
#include <QSettings>
#include <QWidget>

class ITab : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(bool defaults READ hasDefaults)

	protected:
		explicit ITab(QWidget *parent = nullptr);
	public:
		~ITab() override = default;

	private:
		typedef QWidget super;
		Q_DISABLE_COPY(ITab)

	public:
		/**
		 * Does this tab have defaults available?
		 * If so, the "Defaults" button will be enabled.
		 * Otherwise, it will be disabled.
		 *
		 * Default is true.
		 *
		 * @return True to enable; false to disable.
		 */
		virtual bool hasDefaults(void) const { return true; }

	public slots:
		/**
		 * Reset the configuration.
		 */
		virtual void reset(void) = 0;

		/**
		 * Load the default configuration.
		 * This does NOT save, and will only emit modified()
		 * if it's different from the current configuration.
		 */
		virtual void loadDefaults(void) { }

		/**
		 * Save the configuration.
		 * @param pSettings QSettings object.
		 */
		virtual void save(QSettings *pSettings) = 0;

	signals:
		/**
		 * Configuration has been modified.
		 */
		void modified(void);
};
