/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * Cache.hpp: Cache tab for rp-config.                                     *
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

#ifndef __ROMPROPERTIES_KDE_CONFIG_CACHETAB_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_CACHETAB_HPP__

#include "ITab.hpp"

class CacheTabPrivate;
class CacheTab : public ITab
{
	Q_OBJECT

	public:
		explicit CacheTab(QWidget *parent = nullptr);
		virtual ~CacheTab();

	private:
		typedef ITab super;
		CacheTabPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(CacheTab);
		Q_DISABLE_COPY(CacheTab)

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

	protected slots:
		/**
		 * "Clear System Thumbnail Cache" was clicked.
		 */
		void on_btnClearSysThumbnailCache_clicked(void);

		/**
		 * "Clear ROM Properties Cache" was clicked.
		 */
		void on_btnClearRomPropertiesCache_clicked(void);
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_CACHETAB_HPP__ */
