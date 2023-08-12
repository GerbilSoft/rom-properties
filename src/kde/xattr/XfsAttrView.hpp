/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * XfsAttrView.hpp: XFS file system attribute viewer widget.             *
 *                                                                         *
 * Copyright (c) 2022-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Qt includes
#include <QWidget>

class XfsAttrViewPrivate;
class XfsAttrView : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(uint32_t xflags READ xflags WRITE setXFlags RESET clearXFlags)
	Q_PROPERTY(uint32_t projectId READ projectId WRITE setProjectId RESET clearProjectId)

	public:
		explicit XfsAttrView(QWidget *parent = nullptr);

	private:
		typedef QWidget super;
		XfsAttrViewPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(XfsAttrView)
		Q_DISABLE_COPY(XfsAttrView)

	protected:
		// State change event. (Used for switching the UI language at runtime.)
		void changeEvent(QEvent *event) final;

	public:
		/**
		 * Get the current XFS xflags.
		 * @return XFS xflags
		 */
		uint32_t xflags(void) const;

		/**
		 * Set the current XFS xflags.
		 * @param flags XFS xflags
		 */
		void setXFlags(uint32_t xflags);

		/**
		 * Clear the current XFS xflags.
		 */
		void clearXFlags(void);

		/**
		 * Get the current XFS project ID.
		 * @return XFS project ID
		 */
		uint32_t projectId(void) const;

		/**
		 * Set the current XFS project ID.
		 * @param flags XFS project ID
		 */
		void setProjectId(uint32_t projectId);

		/**
		 * Clear the current XFS project ID.
		 */
		void clearProjectId(void);

	protected slots:
		/**
		 * Disable user modifications of checkboxes.
		 */
		void checkBox_clicked_slot(bool checked);
};
