/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * XAttrView.hpp: Extended attribute viewer property page.                 *
 *                                                                         *
 * Copyright (c) 2022-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Qt includes
#include <QtCore/QUrl>
#include <QWidget>

class XAttrViewPrivate;
class XAttrView : public QWidget
{
Q_OBJECT

Q_PROPERTY(QUrl filename READ filename WRITE setFilename NOTIFY filenameChanged)

public:
	explicit XAttrView(QWidget *parent = nullptr);
	explicit XAttrView(const QUrl &url, QWidget *parent = nullptr);

private:
	typedef QWidget super;
	XAttrViewPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(XAttrView)
	Q_DISABLE_COPY(XAttrView)

public:
	/**
	 * Get the current filename.
	 * @return Current filename
	 */
	QUrl filename(void) const;

	/**
	 * Set the current filename.
	 * @param url Filename
	 */
	void setFilename(const QUrl &url);

public:
	/**
	 * Do we have attributes for the specified filename?
	 * @return True if we have attributes; false if not.
	 */
	bool hasAttributes(void) const;

public:
	/**
	 * Apply changes to attributes.
	 * @return 0 on success; non-zero on error.
	 */
	void applyChanges(void);

signals:
	/**
	 * The filename has been changed.
	 * @param url New filename
	 */
	void filenameChanged(const QUrl &url);

	/**
	 * The user has modified attributes.
	 */
	void modified(void);
};
