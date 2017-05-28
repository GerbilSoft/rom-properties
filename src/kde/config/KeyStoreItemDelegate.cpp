/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyStoreItemDelegate.cpp: KeyStore item delegate for QListView.         *
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

// References:
// - https://stackoverflow.com/questions/26614678/validating-user-input-in-a-qtableview
// - https://stackoverflow.com/a/26614960
#include "KeyStoreItemDelegate.hpp"

// C includes. (C++ namespace)
#include <cassert>

// Qt includes.
#include <QtGui/QValidator>
#include <QLineEdit>

#if QT_VERSION > 0x050000
// Qt5 deprecated QStyleOptionViewItemV4 in favor of
// plain old QStyleOptionViewItem. However, for Qt4
// compatibility, we still need to use V4.
#define QStyleOptionViewItemV4 QStyleOptionViewItem
#endif

KeyStoreItemDelegate::KeyStoreItemDelegate(QObject *parent)
	: super(parent)
{
	// Initialize the validators.
	static const char regex_validHexKey[] = "[0-9a-fA-F]*";
	// TODO
	static const char regex_validHexKeyOrKanji[] = "[0-9a-fA-F]*";

	m_validHexKey = new QRegExpValidator(
		QRegExp(QLatin1String(regex_validHexKey)), this);
	m_validHexKeyOrKanji = new QRegExpValidator(
		QRegExp(QLatin1String(regex_validHexKeyOrKanji)), this);
}

QWidget *KeyStoreItemDelegate::createEditor(QWidget *parent,
	const QStyleOptionViewItem &option,
        const QModelIndex &index) const
{
	Q_UNUSED(option);
	QLineEdit *editor = new QLineEdit(parent);
	return editor;
}

void KeyStoreItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QString value = index.model()->data(index, Qt::EditRole).toString();
	// TODO: Set the Kanji validator if allowKanji is set.
	QLineEdit *line = qobject_cast<QLineEdit*>(editor);
	assert(line != nullptr);
	if (line) {
		line->setValidator(m_validHexKey);
		line->setText(value);
	}
}

void KeyStoreItemDelegate::setModelData(QWidget *editor,
	QAbstractItemModel *model,
	const QModelIndex &index) const
{
	QLineEdit *line = qobject_cast<QLineEdit*>(editor);
	assert(line != nullptr);
	if (line) {
		QString value = line->text();
		model->setData(index, value);
	}
}


void KeyStoreItemDelegate::updateEditorGeometry(QWidget *editor,
	const QStyleOptionViewItem &option,
	const QModelIndex &index) const
{
	editor->setGeometry(option.rect);
}
