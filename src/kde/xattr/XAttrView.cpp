/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * XAttrView.cpp: Extended attribute viewer property page.                 *
 *                                                                         *
 * Copyright (c) 2022 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://doc.qt.io/qt-5/dnd.html
#include "stdafx.h"
#include "XAttrView.hpp"

// EXT2 flags (also used for EXT3, EXT4, and other Linux file systems)
#include "ext2_flags.h"

// for EXT2 flags [TODO: move to librpbase/libromdata]
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
// for FS_IOC_GETFLAGS (equivalent to EXT2_IOC_GETFLAGS)
#include <linux/fs.h>

// C++ STL classes
using std::string;

/** XAttrViewPrivate **/

#include "ui_XAttrView.h"
class XAttrViewPrivate
{
	public:
		// TODO: Reomve localizeQUrl() once non-local QUrls are supported.
		XAttrViewPrivate(const QUrl &filename)
			: filename(localizeQUrl(filename))
			, hasAttributes(false)
		{ }

	private:
		Q_DISABLE_COPY(XAttrViewPrivate)

	public:
		Ui::XAttrView ui;
		QUrl filename;

		// Do we have attributes for this file?
		bool hasAttributes;

	public:
		/**
		 * Load the attributes from the specified file.
		 * The attributes will be loaded into the display widgets.
		 * @return 0 on success; non-zero on error.
		 */
		int loadAttributes(void);

		/**
		 * Clear the display widgets.
		 */
		void clearDisplayWidgets();
};

/**
 * Load the attributes from the specified file.
 * The attributes will be loaded into the display widgets.
 * @return 0 on success; non-zero on error.
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

#ifdef __gnu_linux__
	// Make sure this is a regular file.
	// TODO: Use statx() if available.
	struct stat sb;
	errno = 0;
	if (!stat(s_local_filename.c_str(), &sb) && !S_ISREG(sb.st_mode) && !S_ISDIR(sb.st_mode)) {
		// stat() failed, or this is neither a regular file nor a directory.
		int err = -errno;
		if (err == 0) {
			err = -ENOTSUP;
		}
		hasAttributes = false;
		clearDisplayWidgets();
		return err;
	}

	// Open the file to get attributes.
	// TODO: Move this to librpbase or libromdata,
	// and add configure checks for HAVE_EXT2_IOCTLS.
	// (e2fsprogs enables HAVE_EXT2_IOCTLS on Linux only)
	#define OPEN_FLAGS (O_RDONLY|O_NONBLOCK|O_LARGEFILE|O_NOFOLLOW)
	int fd = open(s_local_filename.c_str(), OPEN_FLAGS);
	if (fd < 0) {
		// Error opening the file.
		const int err = -errno;
		hasAttributes = false;
		clearDisplayWidgets();
		return err;
	}

	// Verify the file mode again using fstat().
	errno = 0;
	if (!fstat(fd, &sb) && !S_ISREG(sb.st_mode) && !S_ISDIR(sb.st_mode)) {
		// fstat() failed, or this is neither a regular file nor a directory.
		int err = -errno;
		if (err == 0) {
			err = -ENOTSUP;
		}
		close(fd);
		hasAttributes = false;
		clearDisplayWidgets();
		return err;
	}

	// Attempt to get EXT2 flags.
	// NOTE: The ioctl is defined as using long, but the actual
	// kernel code uses int.
	int ext2_flags = 0;
	errno = 0;
	if (!ioctl(fd, FS_IOC_GETFLAGS, &ext2_flags)) {
		// ioctl() succeeded. We have EXT2 flags.
		ui.linuxAttrView->setFlags(static_cast<int>(ext2_flags));
	} else {
		// No EXT2 flags on this file.
		// TODO: Check errno to see if it doesn't have flags vs. an error occurred?
		ui.linuxAttrView->clearFlags();
	}

	// Done using the file.
	close(fd);
	hasAttributes = true;
#else /* !__gnu_linux__ */
	// Can't use HAVE_EXT2_IOCTLs.
	hasAttributes = false;
	clearDisplayWidgets();
#endif /* __gnu_linux__ */

	return 0;
}

/**
 * Clear the display widgets.
 */
void XAttrViewPrivate::clearDisplayWidgets()
{
	// TODO: Other widgets.
	ui.linuxAttrView->clearFlags();
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
	QUrl localUrl = localizeQUrl(filename);
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
