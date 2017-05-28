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
#include "KeyStoreModel.hpp"

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
	// QRegularExpressionValidator is preferred because it's
	// significantly faster, but it was added in Qt 5.1, so
	// we need to use QRegExpValidator if it isn't available.

	// Hexadecimal
	static const char regex_validHexKey[] = "[0-9a-fA-F]*";

	// Hexadecimal + Kanji
	// Reference: http://www.localizingjapan.com/blog/2012/01/20/regular-expressions-for-japanese-text/
#if QT_VERSION >= 0x050100
	static const char regex_validHexKeyOrKanji[] = "[0-9a-fA-F\\p{Han}]*";
#else /* QT_VERSION < 0x050100 */
	// QRegExp doesn't support named Unicode properties.
	static const char regex_validHexKeyOrKanji[] = "[0-9a-fA-F\\x3400-\\x4DB5\\x4E00-\\x9FCB\\xF900-\\xFA6A]*";
#endif

#if QT_VERSION >= 0x050100
	// Create QRegularExpressionValidator objects.
	m_validHexKey = new QRegularExpressionValidator(
		QRegularExpression(QLatin1String(regex_validHexKey)), this);
	m_validHexKeyOrKanji = new QRegularExpressionValidator(
		QRegularExpression(QLatin1String(regex_validHexKeyOrKanji)), this);
#else /* QT_VERSION < 0x050100 */
	// Create QRegExpValidator objects.
	m_validHexKey = new QRegExpValidator(
		QRegExp(QLatin1String(regex_validHexKey)), this);
	m_validHexKeyOrKanji = new QRegExpValidator(
		QRegExp(QLatin1String(regex_validHexKeyOrKanji)), this);
#endif
}

QWidget *KeyStoreItemDelegate::createEditor(QWidget *parent,
	const QStyleOptionViewItem &option,
        const QModelIndex &index) const
{
	Q_UNUSED(option);
	Q_UNUSED(index);
	QLineEdit *editor = new QLineEdit(parent);
	return editor;
}

void KeyStoreItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	const QAbstractItemModel *model = index.model();
	QString value = model->data(index, Qt::EditRole).toString();
	bool allowKanji = model->data(index, KeyStoreModel::AllowKanjiRole).toBool();

	QLineEdit *line = qobject_cast<QLineEdit*>(editor);
	assert(line != nullptr);
	if (line) {
		line->setValidator(allowKanji ? m_validHexKeyOrKanji : m_validHexKey);
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
	Q_UNUSED(index);
	editor->setGeometry(option.rect);
}
