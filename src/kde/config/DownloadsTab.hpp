/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * DownloadsTab.hpp: Downloads tab for rp-config.                          *
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

#ifndef __ROMPROPERTIES_KDE_CONFIG_DOWNLOADSTAB_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_DOWNLOADSTAB_HPP__

#include "ITab.hpp"

class DownloadsTabPrivate;
class DownloadsTab : public ITab
{
	Q_OBJECT

	public:
		explicit DownloadsTab(QWidget *parent = nullptr);
		virtual ~DownloadsTab();

	private:
		typedef ITab super;
		DownloadsTabPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(DownloadsTab);
		Q_DISABLE_COPY(DownloadsTab)

	protected:
		// State change event. (Used for switching the UI language at runtime.)
		virtual void changeEvent(QEvent *event) override final;

	public slots:
		/**
		 * Reset the configuration.
		 */
		virtual void reset(void) override final;

		/**
		 * Load the default configuration.
		 * This does NOT save, and will only emit modified()
		 * if it's different from the current configuration.
		 */
		virtual void loadDefaults(void) override final;

		/**
		 * Save the configuration.
		 * @param pSettings QSettings object.
		 */
		virtual void save(QSettings *pSettings) override final;

	protected slots:
		/**
		 * A checkbox was clicked.
		 */
		void checkBox_clicked(void);
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_DOWNLOADSTAB_HPP__ */
