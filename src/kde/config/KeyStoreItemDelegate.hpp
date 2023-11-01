/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyStoreItemDelegate.hpp: KeyStore item delegate for QListView.         *
 *                                                                         *
 * Copyright (c) 2013-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

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
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option,
		const QModelIndex &index) const final;
	void setEditorData(QWidget *editor, const QModelIndex &index) const final;
	void setModelData(QWidget *editor, QAbstractItemModel *model,
		const QModelIndex &index) const final;
	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
		const QModelIndex &index) const final;

	void paint(QPainter *painter, const QStyleOptionViewItem &option,
		const QModelIndex &index) const final;

protected:
	// Validators.
	QValidator *m_validHexKey;
	QValidator *m_validHexKeyOrKanji;
};
