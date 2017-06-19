/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ITab.hpp: Configuration tab interface.                                  *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_CONFIG_ITAB_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_ITAB_HPP__

// Qt includes.
#include <QSettings>
#include <QWidget>

class ITab : public QWidget
{
	Q_OBJECT

	protected:
		explicit ITab(QWidget *parent = nullptr);
	public:
		virtual ~ITab();

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
		virtual void loadDefaults(void) = 0;

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

#endif /* __ROMPROPERTIES_KDE_CONFIG_ITAB_HPP__ */
