/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * DosAttrView.hpp: MS-DOS file system attribute viewer widget.            *
 *                                                                         *
 * Copyright (c) 2022 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://doc.qt.io/qt-5/dnd.html

#ifndef __ROMPROPERTIES_KDE_XATTR_DOSATTRVIEW_HPP__
#define __ROMPROPERTIES_KDE_XATTR_DOSATTRVIEW_HPP__

// Qt includes
#include <QWidget>

class DosAttrViewPrivate;
class DosAttrView : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(unsigned int attrs READ attrs WRITE setAttrs RESET clearAttrs)

	public:
		explicit DosAttrView(QWidget *parent = nullptr);

	private:
		typedef QWidget super;
		DosAttrViewPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(DosAttrView)
		Q_DISABLE_COPY(DosAttrView)

	public:
		/**
		 * Get the current MS-DOS attributes.
		 * @return MS-DOS attributes
		 */
		unsigned int attrs(void) const;

		/**
		 * Set the current MS-DOS attributes.
		 * @param attrs MS-DOS attributes
		 */
		void setAttrs(unsigned int attrs);

		/**
		 * Clear the attributes.
		 */
		void clearAttrs(void);

	protected slots:
		/**
		 * Disable user modifications of checkboxes.
		 */
		void checkBox_clicked_slot(bool checked);
};

#endif /* __ROMPROPERTIES_KDE_XATTR_DOSATTRVIEW_HPP__ */
