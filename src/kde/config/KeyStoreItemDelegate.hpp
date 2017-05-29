/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyStoreItemDelegate.hpp: KeyStore item delegate for QListView.         *
 *                                                                         *
 * Copyright (c) 2013-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_KDE_CONFIG_KEYSTOREITEMDELEGATE_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_KEYSTOREITEMDELEGATE_HPP__

// References:
// - https://stackoverflow.com/questions/26614678/validating-user-input-in-a-qtableview
// - https://stackoverflow.com/a/26614960

// Qt includes and classes.
#include <QStyledItemDelegate>
class QValidator;

class KeyStoreItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

	public:
		explicit KeyStoreItemDelegate(QObject *parent);

	private:
		typedef QStyledItemDelegate super;
		Q_DISABLE_COPY(KeyStoreItemDelegate)

	public:
		virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option,
			const QModelIndex &index) const override final;
		virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override final;
		virtual void setModelData(QWidget *editor, QAbstractItemModel *model,
			const QModelIndex &index) const override final;
		virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
			const QModelIndex &index) const override final;

		virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
			const QModelIndex &index) const override final;

	protected:
		// Validators.
		QValidator *m_validHexKey;
		QValidator *m_validHexKeyOrKanji;
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_KEYSTOREITEMDELEGATE_HPP__ */
