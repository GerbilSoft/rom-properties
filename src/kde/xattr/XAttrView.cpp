/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * XAttrView.cpp: Extended attribute viewer property page.                 *
 *                                                                         *
 * Copyright (c) 2022-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://doc.qt.io/qt-5/dnd.html
#include "stdafx.h"
#include "XAttrView.hpp"

// XAttrReader
#include "librpfile/xattr/XAttrReader.hpp"
using LibRpFile::XAttrReader;

// C++ STL classes
using std::string;

/** XAttrViewPrivate **/

#include "ui_XAttrView.h"
class XAttrViewPrivate
{
	public:
		// TODO: Reomve localizeQUrl() once non-local QUrls are supported.
		explicit XAttrViewPrivate(const QUrl &filename)
			: filename(localizeQUrl(filename))
			, xattrReader(nullptr)
			, hasAttributes(false)
		{ }

		~XAttrViewPrivate()
		{
			delete xattrReader;
		}

	private:
		Q_DISABLE_COPY(XAttrViewPrivate)

	public:
		Ui::XAttrView ui;
		QUrl filename;

		// XAttrReader
		XAttrReader *xattrReader;

		// Do we have attributes for this file?
		bool hasAttributes;

	private:
		/**
		 * Load Ext2 attributes, if available.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadExt2Attrs(void);

		/**
		 * Load XFS attributes, if available.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadXfsAttrs(void);

		/**
		 * Load MS-DOS attributes, if available.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadDosAttrs(void);

		/**
		 * Load POSIX xattrs, if available.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadPosixXattrs(void);

	public:
		/**
		 * Load the attributes from the specified file.
		 * The attributes will be loaded into the display widgets.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadAttributes(void);

		/**
		 * Clear the display widgets.
		 */
		void clearDisplayWidgets();
};

/**
 * Load Ext2 attributes, if available.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrViewPrivate::loadExt2Attrs(void)
{
	// Hide by default.
	// If we do have attributes, we'll show the widgets there.
	ui.grpExt2Attributes->hide();

	if (!xattrReader->hasExt2Attributes()) {
		// No Ext2 attributes.
		return -ENOENT;
	}

	// We have Ext2 attributes.
	ui.ext2AttrView->setFlags(xattrReader->ext2Attributes());
	ui.grpExt2Attributes->show();
	return 0;
}

/**
 * Load XFS attributes, if available.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrViewPrivate::loadXfsAttrs(void)
{
	// Hide by default.
	// If we do have attributes, we'll show the widgets there.
	ui.grpXfsAttributes->hide();

	if (!xattrReader->hasXfsAttributes()) {
		// No XFS attributes.
		return -ENOENT;
	}

	// We have XFS attributes.
	// NOTE: If all attributes are 0, don't bother showing it.
	// XFS isn't *nearly* as common as Ext2/Ext3/Ext4.
	// TODO: ...unless this is an XFS file system?
	const uint32_t xflags = xattrReader->xfsXFlags();
	const uint32_t projectId = xattrReader->xfsProjectId();
	if (xflags == 0 && projectId == 0) {
		// All zero.
		return -ENOENT;
	}

	// Show the XFS attributes.
	ui.xfsAttrView->setXFlags(xattrReader->xfsXFlags());
	ui.xfsAttrView->setProjectId(xattrReader->xfsProjectId());
	ui.grpXfsAttributes->show();
	return 0;
}

/**
 * Load MS-DOS attributes, if available.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrViewPrivate::loadDosAttrs(void)
{
	// Hide by default.
	// If we do have attributes, we'll show the widgets there.
	ui.grpDosAttributes->hide();

	if (!xattrReader->hasDosAttributes()) {
		// No MS-DOS attributes.
		return -ENOENT;
	}

	// We have MS-DOS attributes.
	ui.dosAttrView->setAttrs(xattrReader->dosAttributes());
	ui.grpDosAttributes->show();
	return 0;
}

/**
 * Load POSIX xattrs, if available.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrViewPrivate::loadPosixXattrs(void)
{
	// Hide by default.
	// If we do have attributes, we'll show the widgets there.
	ui.grpXAttr->hide();

	ui.treeXAttr->clear();
	if (!xattrReader->hasGenericXAttrs()) {
		// No generic attributes.
		return -ENOENT;
	}

	const XAttrReader::XAttrList &xattrList = xattrReader->genericXAttrs();
	for (const auto &xattr : xattrList) {
		QTreeWidgetItem *const treeWidgetItem = new QTreeWidgetItem(ui.treeXAttr);
		treeWidgetItem->setData(0, Qt::DisplayRole, QString::fromUtf8(xattr.first.c_str(), xattr.first.size()));
		// NOTE: Trimming leading and trailing spaces from the value.
		// TODO: If copy is added, include the spaces.
		treeWidgetItem->setData(1, Qt::DisplayRole, QString::fromUtf8(xattr.second.c_str(), xattr.second.size()).trimmed());
	}

	// Set column stretch modes.
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QHeaderView *const pHeader = ui.treeXAttr->header();
	pHeader->setStretchLastSection(false);
	pHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	pHeader->setSectionResizeMode(1, QHeaderView::Stretch);
#else /* QT_VERSION <= QT_VERSION_CHECK(5,0,0) */
	// Qt 4 doesn't have QHeaderView::setSectionResizeMode().
	// We'll run a manual resize on each column initially.
	for (int i = 0; i < 2; i++) {
		ui.treeXAttr->resizeColumnToContents(i);
	}
#endif

	// Extended attributes retrieved.
	ui.grpXAttr->show();
	return 0;
}

/**
 * Load the attributes from the specified file.
 * The attributes will be loaded into the display widgets.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrViewPrivate::loadAttributes(void)
{
	// TODO: Handle non-local QUrls?
	if (filename.isEmpty()) {
		// Empty. Clear the display widgets.
		hasAttributes = false;
		clearDisplayWidgets();
		return -EINVAL;
	}

	if (filename.scheme().isEmpty() || filename.isLocalFile()) {
		// Local URL. We'll allow it.
	} else {
		// Not a local URL. Clear the display widgets.
		hasAttributes = false;
		clearDisplayWidgets();
		return -ENOTSUP;
	}

	const string s_local_filename = filename.toLocalFile().toUtf8().constData();

	// Close the XAttrReader if it's already open.
	delete xattrReader;

	// Open an XAttrReader.
	xattrReader = new XAttrReader(s_local_filename.c_str());
	int err = xattrReader->lastError();
	if (err != 0) {
		// Error reading attributes.
		delete xattrReader;
		xattrReader = nullptr;
		return err;
	}

	// Load the attributes.
	bool hasAnyAttrs = false;
	int ret = loadExt2Attrs();
	if (ret == 0) {
		hasAnyAttrs = true;
	}
	ret = loadXfsAttrs();
	if (ret == 0) {
		hasAnyAttrs = true;
	}
	ret = loadDosAttrs();
	if (ret == 0) {
		hasAnyAttrs = true;
	}
	ret = loadPosixXattrs();
	if (ret == 0) {
		hasAnyAttrs = true;
	}

	// If we have attributes, great!
	// If not, clear the display widgets.
	if (hasAnyAttrs) {
		hasAttributes = true;
	} else {
		hasAttributes = false;
		clearDisplayWidgets();
	}
	return 0;
}

/**
 * Clear the display widgets.
 */
void XAttrViewPrivate::clearDisplayWidgets()
{
	ui.ext2AttrView->clearFlags();
	ui.dosAttrView->clearAttrs();
	ui.treeXAttr->clear();
}

/** XAttrView **/

XAttrView::XAttrView(QWidget *parent)
	: super(parent)
	, d_ptr(new XAttrViewPrivate(QUrl()))
{
	Q_D(XAttrView);
	d->ui.setupUi(this);
}

XAttrView::XAttrView(const QUrl &filename, QWidget *parent)
	: super(parent)
	, d_ptr(new XAttrViewPrivate(filename))
{
	Q_D(XAttrView);
	d->ui.setupUi(this);

	// Load the attributes.
	d->loadAttributes();
}

/**
 * Get the current filename.
 * @return Current filename
 */
QUrl XAttrView::filename(void) const
{
	Q_D(const XAttrView);
	return d->filename;
}

/**
 * Set the current filename.
 * @param url Filename
 */
void XAttrView::setFilename(const QUrl &filename)
{
	Q_D(XAttrView);

	// TODO: Handle non-local URLs.
	// For now, converting to local.
	const QUrl localUrl = localizeQUrl(filename);
	if (d->filename != localUrl) {
		d->filename = localUrl;
		d->loadAttributes();
		emit filenameChanged(filename);
	}
}

/**
 * Do we have attributes for the specified filename?
 * @return True if we have attributes; false if not.
 */
bool XAttrView::hasAttributes(void) const
{
	Q_D(const XAttrView);
	return d->hasAttributes;
}
