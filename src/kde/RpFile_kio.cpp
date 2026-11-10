/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpFile_kio.cpp: IRpFile implementation using KIO.                       *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpFile_kio.hpp"

// Qt includes
#include <QtCore/QEventLoop>

// KDE includes
#include <KIO/FileJob>

// C++ STL classes
using std::string;
using std::string_view;

#include "RpQt.hpp"

class RpFileKioPrivate
{
public:
	RpFileKioPrivate(RpFileKio *q, string_view uri);

	RpFileKioPrivate(RpFileKio *q, const QUrl &uri)
		: q_ptr(q)
		, uri(uri)
		, lastResult(0)
		, pos(0)
	{}

	~RpFileKioPrivate();

public:
	RpFileKio *const q_ptr;
	RP_DISABLE_COPY(RpFileKioPrivate)

public:
	KIO::FileJob *fileJob;	// File job
	QUrl uri;		// KIO URI

	// Last read data
	QByteArray lastData;

	// Last result
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

RpFileKioPrivate::RpFileKioPrivate(RpFileKio *q, string_view uri)
	: q_ptr(q)
	, fileJob(nullptr)
	, lastResult(0)
	, pos(0)
{
	// Check if the source filename is a URI.
	const QString qs_uri(U82Q(uri));
	QUrl url(qs_uri);
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
RpFileKio::RpFileKio(string_view uri)
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

	// NOTE: KIO::result() is a private signal, so we have to use the old
	// signal/slot syntax to connect to it. (...though lambda syntax may work?)
	QObject::connect(d->fileJob, SIGNAL(result(KJob*)), this, SLOT(fileJob_result(KJob*)));

	QObject::connect(d->fileJob, &KIO::FileJob::open, this, &RpFileKio::fileJob_open);
	QObject::connect(d->fileJob, &KIO::FileJob::data, this, &RpFileKio::fileJob_data);
	QObject::connect(d->fileJob, &KIO::FileJob::position, this, &RpFileKio::fileJob_position);

	// Run the loop.
	d->enterLoop();
	if (m_lastError != 0) {
		// An error occurred.
		// NOTE: If result() is called, the KIO::Job will delete itself
		// after the slot is finished running.
		d->fileJob = nullptr;
		return;
	}

	// File is open.
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
 * @param pos		[in] File position
 * @param whence	[in] Where to seek from
 * @return 0 on success; -1 on error.
 */
int RpFileKio::seek(off64_t pos, SeekWhence whence)
{
	RP_D(RpFileKio);
	if (!d->fileJob) {
		m_lastError = EBADF;
		return -1;
	}

	pos = adjust_file_pos_for_whence(pos, whence, d->pos, d->fileJob->size());
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
	// FIXME: Q2U8() returns a temporary, which may be gone by the time this function returns...
	return (!d->uri.isEmpty() ? Q2U8(d->uri.toString()) : nullptr);
}

/** Private slots **/

void RpFileKio::fileJob_result(KJob *job)
{
	RP_D(RpFileKio);
	assert(d->fileJob == job);
	if (d->fileJob != job) {
		// Wrong job...
		return;
	}

	// Did an error occur?
	d->lastResult = d->fileJob->error();
	if (d->lastResult != 0) {
		// An error occurred.
		// TODO: POSIX error code?
		m_lastError = EIO;
	}

	// NOTE: KIO::Job will delete itself after sending the result,
	// since this means an error occurred.
	d->fileJob = nullptr;
	emit exitLoop();
}

void RpFileKio::fileJob_open(KIO::Job *job)
{
	RP_D(RpFileKio);
	assert(d->fileJob == job);
	if (d->fileJob != job) {
		// Wrong job...
		return;
	}

	// FileJob has been opened.
	emit exitLoop();
}

void RpFileKio::fileJob_data(KIO::Job *job, const QByteArray &data)
{
	RP_D(RpFileKio);
	assert(d->fileJob == job);
	if (d->fileJob != job) {
		// Wrong job...
		return;
	}

	d->lastData = data;
	d->pos += data.size();
	emit exitLoop();
}

void RpFileKio::fileJob_position(KIO::Job *job, KIO::filesize_t offset)
{
	RP_D(RpFileKio);
	assert(d->fileJob == job);
	if (d->fileJob != job) {
		// Wrong job...
		return;
	}

	d->pos = offset;
	emit exitLoop();
}
