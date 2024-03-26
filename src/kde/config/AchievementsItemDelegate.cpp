/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * AchievementsItemDelegate.cpp: Achievements item delegate for rp-config. *
 *                                                                         *
 * Copyright (c) 2013-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchievementsItemDelegate.hpp"

// Qt includes
#include <QtGui/QPainter>
#include <QApplication>
#include <QtGui/QFont>
#include <QtGui/QFontMetrics>
#include <QStyle>

// C++ STL classes
using std::array;

#define LINE_COUNT 2

AchievementsItemDelegate::AchievementsItemDelegate(QObject *parent)
	: super(parent)
{}

QFont AchievementsItemDelegate::fontName(const QWidget *widget)
{
	// TODO: This should be cached, but we don't have a
	// reasonable way to update it if the system font
	// is changed...
	return (widget != nullptr)
		? widget->font()
		: QApplication::font();
}

QFont AchievementsItemDelegate::fontDesc(const QWidget *widget)
{
	// TODO: This should be cached, but we don't have a
	// reasonable way to update it if the system font
	// is changed...
	QFont font = fontName(widget);
	int pointSize = font.pointSize();
	if (pointSize >= 10)
		pointSize = (pointSize * 4 / 5);
	else
		pointSize--;
	font.setPointSize(pointSize);
	return font;
}

void AchievementsItemDelegate::paint(QPainter *painter,
			const QStyleOptionViewItem &option,
			const QModelIndex &index) const
{
	if (!index.isValid()) {
		// Index is invalid.
		// Use the default paint().
		super::paint(painter, option, index);
		return;
	}

	// TODO: Combine code with sizeHint().

	// Achievement text is separated by '\n'.
	// If no '\n' is present, assume this is regular text
	// and use the default paint().
	const QString s_ach = index.data().toString();
	const int nl_pos = s_ach.indexOf(QChar(L'\n'));
	if (nl_pos < 0) {
		// No '\n' is present.
		// Use the default paint().
		super::paint(painter, option, index);
		return;
	}

	array<QString, LINE_COUNT> sl = {
		s_ach.left(nl_pos),
		s_ach.mid(nl_pos + 1)
	};

	// Alignment flags.
	static const int HALIGN_FLAGS =
			Qt::AlignLeft |
			Qt::AlignRight |
			Qt::AlignHCenter |
			Qt::AlignJustify;
	static const int VALIGN_FLAGS =
			Qt::AlignTop |
			Qt::AlignBottom |
			Qt::AlignVCenter;

	// Get the text alignment.
	int textAlignment = 0;
	if (index.data(Qt::TextAlignmentRole).canConvert<int>()) {
		textAlignment = index.data(Qt::TextAlignmentRole).toInt();
	}
	if (textAlignment == 0) {
		textAlignment = option.displayAlignment;
	}

	const QRect textRect = option.rect;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QStyleOptionViewItem bgOption = option;
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
	QStyleOptionViewItemV4 bgOption = option;
#endif
	// TODO: initStyleOption()?

	// Horizontal margins.
	// Reference: http://doc.qt.io/qt-4.8/qitemdelegate.html#sizeHint
	QStyle *const style = bgOption.widget ? bgOption.widget->style() : QApplication::style();
	//const int hmargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &option) * 2;

	// Reduce the text rectangle by the hmargin.
	// TODO: vmargin?
	// FIXME: This cuts off the text and doesn't match the alignment
	// of the other columns... (Maybe it's only useful if we're showing
	// an icon in the same column as the text?)
	//textRect.adjust(hmargin, 0, -hmargin, 0);

	// Get the fonts.
	const QFont fontName = this->fontName(bgOption.widget);
	const QFont fontDesc = this->fontDesc(bgOption.widget);

	// Total text height.
	int textHeight = 0;

	// Text boundaries.
	array<QRect, LINE_COUNT> v_rect;

	QFontMetrics fm(fontName);
	for (size_t i = 0; i < sl.size(); i++) {
		// Name uses the normal font.
		// Description lines use a slightly smaller font.
		if (i == 1) {
			fm = QFontMetrics(fontDesc);
		}
		sl[i] = fm.elidedText(sl[i], Qt::ElideRight, textRect.width()-1);
		const QRect tmpRect(textRect.x(), textRect.y() + textHeight, textRect.width(), fm.height());
		textHeight += fm.height();
		v_rect[i] = fm.boundingRect(tmpRect, (textAlignment & HALIGN_FLAGS), sl[i]);
	}

	// Adjust for vertical alignment.
	int diff = 0;
	switch (textAlignment & VALIGN_FLAGS) {
		default:
		case Qt::AlignTop:
			// No adjustment is necessary.
			break;

		case Qt::AlignBottom:
			// Bottom alignment.
			diff = (textRect.height() - textHeight);
			break;

		case Qt::AlignVCenter:
			// Center alignment.
			diff = (textRect.height() - textHeight);
			diff /= 2;
			break;
	}

	if (diff != 0) {
		for (QRect &rect : v_rect) {
			rect.translate(0, diff);
		}
	}

	painter->save();

	// Draw the background color first.
	const QVariant bg_var = index.data(Qt::BackgroundRole);
	QBrush bg;
	if (bg_var.canConvert<QBrush>()) {
		bg = bg_var.value<QBrush>();
	} else if (bg_var.canConvert<QColor>()) {
		bg = QBrush(bg_var.value<QColor>());
	}
	if (bg.style() != Qt::NoBrush)
		bgOption.backgroundBrush = bg;

	// Draw the style element.
	style->drawControl(QStyle::CE_ItemViewItem, &bgOption, painter, bgOption.widget);
	bgOption.backgroundBrush = QBrush();

	// Font color.
	if (bgOption.state & QStyle::State_Selected) {
		painter->setPen(bgOption.palette.highlightedText().color());
	} else {
		painter->setPen(bgOption.palette.text().color());
	}

	// Draw the text lines.
	painter->setFont(fontName);
	for (size_t i = 0; i < sl.size(); i++) {
		if (i == 1) {
			painter->setFont(fontDesc);
		}
		painter->drawText(v_rect[i], sl[i]);
	}

	painter->restore();
}

QSize AchievementsItemDelegate::sizeHint(const QStyleOptionViewItem &option,
				    const QModelIndex &index) const
{
	if (!index.isValid()) {
		// Index is invalid.
		// Use the default sizeHint().
		return super::sizeHint(option, index);
	}

	// TODO: Combine code with paint().

	// Achievement text is separated by '\n'.
	// If no '\n' is present, assume this is regular text
	// and use the default paint().
	const QString s_ach = index.data().toString();
	const int nl_pos = s_ach.indexOf(QChar(L'\n'));
	if (nl_pos < 0) {
		// No '\n' is present.
		// Use the default sizeHint().
		return super::sizeHint(option, index);
	}

	const array<QString, LINE_COUNT> sl = {{
		s_ach.left(nl_pos),
		s_ach.mid(nl_pos + 1)
	}};

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	const QStyleOptionViewItem &bgOption = option;
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
	QStyleOptionViewItemV4 bgOption = option;
#endif
	// TODO: initStyleOption()?

	// Get the fonts.
	const QFont fontName = this->fontName(bgOption.widget);
	const QFont fontDesc = this->fontDesc(bgOption.widget);

	QFontMetrics fm(fontName);
	QSize sz;
	for (size_t i = 0; i < sl.size(); i++) {
		// Name uses the normal font.
		// Description lines use a slightly smaller font.
		if (i == 1) {
			fm = QFontMetrics(fontDesc);
		}
		const QSize szLine = fm.size(0, sl[i]);
		sz.setHeight(sz.height() + szLine.height());

		if (szLine.width() > sz.width()) {
			sz.setWidth(szLine.width());
		}
	}

	// Increase width by 1 to prevent accidental eliding.
	// NOTE: We can't just remove the "-1" from paint(),
	// because that still causes weird wordwrapping.
	if (sz.width() > 0) {
		sz.setWidth(sz.width() + 1);
	}

	return sz;
}
