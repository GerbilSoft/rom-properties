/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyManagerTab.hpp: Key Manager tab for rp-config.                       *
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

#ifndef __ROMPROPERTIES_KDE_CONFIG_KEYMANAGERTAB_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_KEYMANAGERTAB_HPP__

#include "ITab.hpp"

class KeyManagerTabPrivate;
class KeyManagerTab : public ITab
{
	Q_OBJECT
	Q_PROPERTY(bool defaults READ hasDefaults)

	public:
		explicit KeyManagerTab(QWidget *parent = nullptr);
		virtual ~KeyManagerTab();

	private:
		typedef ITab super;
		KeyManagerTabPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(KeyManagerTab);
		Q_DISABLE_COPY(KeyManagerTab)

	public:
		/**
		 * Does this tab have defaults available?
		 * If so, the "Defaults" button will be enabled.
		 * Otherwise, it will be disabled.
		 *
		 * KeyManagerTab sets this to false.
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

#endif /* __ROMPROPERTIES_KDE_CONFIG_KEYMANAGERTAB_HPP__ */
