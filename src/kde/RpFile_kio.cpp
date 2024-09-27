/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpFile_kio.cpp: IRpFile implementation using KIO.                       *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpFile_kio.hpp"

// Qt includes.
#include <QtCore/QEventLoop>

// KDE includes.
#include <KIO/FileJob>

// C++ STL classes.
using std::string;

class RpFileKioPrivate
{
public:
	RpFileKioPrivate(RpFileKio *q, const char *uri);
	RpFileKioPrivate(RpFileKio *q, const QUrl &uri)
		: q_ptr(q)
		, uri(uri)
		, lastResult(0)
		, pos(0) { }

	~RpFileKioPrivate();

private:
	RP_DISABLE_COPY(RpFileKioPrivate)
	RpFileKio *const q_ptr;

public:
	KIO::FileJob *fileJob;	// File job.
	QUrl uri;		// KIO URI.

	// Last read data.
	QByteArray lastData;

	// Last result.
	int lastResult;

	// Current file position.
	// There doesn't seem to be an easy way to
	// retrieve this from KIO::FileJob...
	off64_t pos;

	/**
	 * Enter a QEventLoop while waiting for a KJob to complete.
	 * Reference: https://github.com/KDE/kio/blob/master/autotests/jobremotetest.cpp
	 */
	void enterLoop(void);
};

/** RpFileKioPrivate **/

RpFileKioPrivate::RpFileKioPrivate(RpFileKio *q, const char *uri)
	: q_ptr(q)
	, fileJob(nullptr)
	, lastResult(0)
	, pos(0)
{
	// Check if the source filename is a URI.
	const QString qs_uri(QString::fromUtf8(uri));
	QUrl url(QString::fromUtf8(uri));
	if (url.scheme().isEmpty()) {
		// No scheme. This is a plain old filename.
		this->uri = QUrl::fromLocalFile(qs_uri);
	} else {
		// Other scheme. Use it as-is.
		this->uri = url;
	}
}

RpFileKioPrivate::~RpFileKioPrivate()
{
	if (fileJob) {
		// NOTE: Using deleteLater() in case something is still
		// using this object in another thread.
		fileJob->close();
		fileJob->deleteLater();
	}
}

/**
 * Enter a QEventLoop while waiting for a KJob to complete.
 * Reference: https://github.com/KDE/kio/blob/master/autotests/jobremotetest.cpp
 */
void RpFileKioPrivate::enterLoop(void)
{
	RP_Q(RpFileKio);
	q->m_lastError = 0;

	QEventLoop eventLoop;
	QObject::connect(q, &RpFileKio::exitLoop, &eventLoop, &QEventLoop::quit);
	eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

/** RpFileKio **/

/**
 * Open a file.
 * NOTE: Files are always opened as read-only in binary mode.
 * @param uri KIO URI.
 */
RpFileKio::RpFileKio(const char *uri)
	: super()
	, d_ptr(new RpFileKioPrivate(this, uri))
{
	init();
}

/**
 * Open a file.
 * NOTE: Files are always opened as read-only in binary mode.
 * @param uri KIO URI.
 */
RpFileKio::RpFileKio(const string &uri)
	: super()
	, d_ptr(new RpFileKioPrivate(this, uri.c_str()))
{
	init();
}

/**
 * Open a file.
 * NOTE: Files are always opened as read-only in binary mode.
 * @param uri KIO URI.
 */
RpFileKio::RpFileKio(const QUrl &uri)
	: super()
	, d_ptr(new RpFileKioPrivate(this, uri))
{
	init();
}

/**
 * Common initialization function for RpFile's constructors.
 * Filename must be set in d->filename.
 */
void RpFileKio::init(void)
{
	RP_D(RpFileKio);

	// Open the file.
	m_lastError = 0;
	d->fileJob = KIO::open(d->uri, QIODevice::ReadOnly);
	d->fileJob->setUiDelegate(nullptr);
	/** Signals **/

	// open(): File was opened.
	QObject::connect(d->fileJob, &KIO::FileJob::open, [d, this]() {
		// FileJob has been opened.
		emit exitLoop();
	});
	// result(): An operation finished. (ONLY emitted on failure)
	QObject::connect(d->fileJob, &KIO::FileJob::result, [d, this]() {
		// Did an error occur?
		d->lastResult = d->fileJob->error();
		if (d->lastResult != 0) {
			// An error occurred.
			// TODO: POSIX error code?
			this->m_lastError = EIO;
		}
		emit exitLoop();
	});
	// read(): Data has been read.
	QObject::connect(d->fileJob, &KIO::FileJob::data, [d, this](KIO::Job*, const QByteArray &data) {
		d->lastData = data;
		d->pos += data.size();
		emit exitLoop();
	});
	// position(): File position has been set.
	QObject::connect(d->fileJob, &KIO::FileJob::position, [d, this](KIO::Job*, KIO::filesize_t offset) {
		d->pos = offset;
		emit exitLoop();
	});

	// Run the loop.
	d->enterLoop();
	if (m_lastError != 0) {
		// An error occurred.
		d->fileJob->close();
		d->fileJob->deleteLater();
		d->fileJob = nullptr;
		return;
	}

	// File is open.
	// TODO: Transparent gzip decompression?
}

RpFileKio::~RpFileKio()
{
	delete d_ptr;
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpFileKio::isOpen(void) const
{
	RP_D(const RpFileKio);
	return (d->fileJob != nullptr);
}

/**
 * Close the file.
 */
void RpFileKio::close(void)
{
	RP_D(RpFileKio);
	if (d->fileJob) {
		d->fileJob->close();
		d->fileJob->deleteLater();
		d->fileJob = nullptr;
	}
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpFileKio::read(void *ptr, size_t size)
{
	RP_D(RpFileKio);
	if (!d->fileJob) {
		m_lastError = EBADF;
		return 0;
	}

	// NOTE: kioslaves don't necessarily return the requested
	// amount of data. Keep reading until we get 0 bytes.
	size_t sz_read_total = 0;
	while (size > 0) {
		d->fileJob->read(size);
		d->enterLoop();

		if (m_lastError != 0) {
			// An error occurred.
			return 0;
		}

		// Data is now in d->lastData.
		if (d->lastData.isEmpty()) {
			// No data read...
			break;
		}

		size_t sz_read = std::min(size, static_cast<size_t>(d->lastData.size()));
		if (sz_read > 0) {
			memcpy(ptr, d->lastData.constData(), sz_read);
			sz_read_total += sz_read;
			size -= sz_read;
		}
	}

	return sz_read_total;
}

/**
 * Write data to the file.
 * (NOTE: Not valid for RpFileKio; this will always return 0.)
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t RpFileKio::write(const void *ptr, size_t size)
{
	// Not a valid operation for RpFileKio.
	RP_UNUSED(ptr);
	RP_UNUSED(size);
	m_lastError = EBADF;
	return 0;
}

/**
 * Set the file position.
 * @param pos File position.
 * @return 0 on success; -1 on error.
 */
int RpFileKio::seek(off64_t pos)
{
	RP_D(RpFileKio);
	if (!d->fileJob) {
		m_lastError = EBADF;
		return -1;
	}

	d->fileJob->seek(pos);
	d->enterLoop();

	return (m_lastError == 0) ? 0 : -1;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
off64_t RpFileKio::tell(void)
{
	RP_D(const RpFileKio);
	if (!d->fileJob) {
		m_lastError = EBADF;
		return -1;
	}

	return d->pos;
}

/** File properties **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
off64_t RpFileKio::size(void)
{
	RP_D(RpFileKio);
	if (!d->fileJob) {
		m_lastError = EBADF;
		return -1;
	}

	// Get the file size directly.
	return d->fileJob->size();
}

/**
 * Get the filename.
 * NOTE: For RpFileKio, this returns a KIO URI.
 * @return Filename. (May be nullptr if the filename is not available.)
 */
const char *RpFileKio::filename(void) const
{
	RP_D(const RpFileKio);
	return (!d->uri.isEmpty() ? d->uri.toString().toUtf8().constData() : nullptr);
}
