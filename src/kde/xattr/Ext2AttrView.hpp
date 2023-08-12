/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * Ext2AttrView.hpp: Ext2 file system attribute viewer widget.             *
 *                                                                         *
 * Copyright (c) 2022-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Qt includes
#include <QWidget>

class Ext2AttrViewPrivate;
class Ext2AttrView : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(int flags READ flags WRITE setFlags RESET clearFlags)

	public:
		explicit Ext2AttrView(QWidget *parent = nullptr);

	private:
		typedef QWidget super;
		Ext2AttrViewPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(Ext2AttrView)
		Q_DISABLE_COPY(Ext2AttrView)

	protected:
		// State change event. (Used for switching the UI language at runtime.)
		void changeEvent(QEvent *event) final;

	public:
		/**
		 * Get the current Ext2 attributes.
		 * @return Ext2 attributes
		 */
		int flags(void) const;

		/**
		 * Set the current Ext2 attributes.
		 * @param flags Ext2 attributes
		 */
		void setFlags(int flags);

		/**
		 * Clear the current Ext2 attributes.
		 */
		void clearFlags(void);

	protected slots:
		/**
		 * Disable user modifications of checkboxes.
		 */
		void checkBox_clicked_slot(bool checked);
};
