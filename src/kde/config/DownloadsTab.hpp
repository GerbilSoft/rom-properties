/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * DownloadsTab.hpp: Downloads tab for rp-config.                          *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
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
		 * A checkbox was clicked.
		 */
		void checkBox_clicked(void);
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_DOWNLOADSTAB_HPP__ */
