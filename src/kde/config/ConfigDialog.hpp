/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ConfigDialog.hpp: Configuration dialog.                                 *
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

#ifndef __ROMPROPERTIES_KDE_CONFIG_CONFIGDIALOG_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_CONFIGDIALOG_HPP__

#include <QDialog>

class ConfigDialogPrivate;
class ConfigDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit ConfigDialog(QWidget *parent = nullptr);
		~ConfigDialog();

	private:
		typedef QDialog super;
		ConfigDialogPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(ConfigDialog)
		Q_DISABLE_COPY(ConfigDialog)

	protected:
		// State change event. (Used for switching the UI language at runtime.)
		void changeEvent(QEvent *event) final;

		// Event filter for tracking focus.
		bool eventFilter(QObject *watched, QEvent *event) final;

	protected slots:
		/**
		 * The current tab has changed.
		 */
		void on_tabWidget_currentChanged(void);

		/**
		 * The "OK" button was clicked.
		 */
		void accept(void) final;

		/**
		 * The "Apply" button was clicked.
		 */
		void apply(void);

		/**
		 * The "Reset" button was clicked.
		 */
		void reset(void);

		/**
		 * The "Defaults" button was clicked.
		 */
		void loadDefaults(void);

		/**
		 * A tab has been modified.
		 */
		void tabModified(void);
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_CONFIGDIALOG_HPP__ */
