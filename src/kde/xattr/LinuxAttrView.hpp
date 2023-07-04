/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * LinuxAttrView.hpp: Linux file system attribute viewer widget.           *
 *                                                                         *
 * Copyright (c) 2022 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Qt includes
#include <QWidget>

class LinuxAttrViewPrivate;
class LinuxAttrView : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(int flags READ flags WRITE setFlags RESET clearFlags)

	public:
		explicit LinuxAttrView(QWidget *parent = nullptr);

	private:
		typedef QWidget super;
		LinuxAttrViewPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(LinuxAttrView)
		Q_DISABLE_COPY(LinuxAttrView)

	protected:
		// State change event. (Used for switching the UI language at runtime.)
		void changeEvent(QEvent *event) final;

	public:
		/**
		 * Get the current Linux attributes.
		 * @return Linux attributes
		 */
		int flags(void) const;

		/**
		 * Set the current Linux attributes.
		 * @param flags Linux attributes
		 */
		void setFlags(int flags);

		/**
		 * Clear the current Linux attributes.
		 */
		void clearFlags(void);

	protected slots:
		/**
		 * Disable user modifications of checkboxes.
		 */
		void checkBox_clicked_slot(bool checked);
};
