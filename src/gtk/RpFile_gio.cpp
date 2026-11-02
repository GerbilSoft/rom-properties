/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_gio.cpp: IRpFile implementation using GIO/GVfs.                  *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpFile_gio.hpp"

// gio
#include <gio/gio.h>

// C++ STL classes
#include <array>
using std::array;
using std::string;
using std::string_view;

class RpFileGioPrivate
{
public:
	explicit RpFileGioPrivate(string_view uri, RpFileGio::FileMode mode);
	~RpFileGioPrivate();

public:
	RP_DISABLE_COPY(RpFileGioPrivate)

public:
	GObject *stream;		// may be GFileInputStream (read-only) or GFileIOStream (read-write)
	string uri;			// GVfs URI
	RpFileGio::FileMode mode;	// File mode

public:
	/**
	 * Convert a GIO error code to POSIX.
	 * @param gioerr GIO error code.
	 * @return POSIX error code.
	 */
	static int gioerr_to_posix(gint gioerr);

	/**
	 * Get the GInputStream.
	 * @return GInputStream, or nullptr if not available.
	 */
	GInputStream *get_GInputStream(void);

	/**
	 * Get the GOutputStream.
	 * @return GOutputStream, or nullptr if not available.
	 */
	GOutputStream *get_GOutputStream(void);
};

/** RpFileGioPrivate **/

RpFileGioPrivate::RpFileGioPrivate(string_view uri, RpFileGio::FileMode mode)
	: mode(mode)
{
	stream = nullptr;
	STRING_VIEW_ASSIGN(this->uri, uri);
}

RpFileGioPrivate::~RpFileGioPrivate()
{
	g_clear_object(&stream);
}

/**
 * Convert a GIO error code to POSIX.
 * @param gioerr GIO error code.
 * @return POSIX error code.
 */
int RpFileGioPrivate::gioerr_to_posix(gint gioerr)
{
	int err;
	switch (gioerr) {
		case G_IO_ERROR_NOT_FOUND:
			err = ENOENT;
			break;
		case G_IO_ERROR_IS_DIRECTORY:
			err = EISDIR;
			break;
		default:
			err = EIO;
			break;
	}
	return err;
}

/**
 * Get the GInputStream.
 * @return GInputStream, or nullptr if not available.
 */
GInputStream *RpFileGioPrivate::get_GInputStream(void)
{
	// Likely a GInputStream, so check that first.
	if (G_IS_INPUT_STREAM(stream)) {
		return G_INPUT_STREAM(stream);
	} else if (G_IS_IO_STREAM(stream)) {
		return g_io_stream_get_input_stream(G_IO_STREAM(stream));
	}
	return nullptr;
}

/**
 * Get the GOutputStream.
 * @return GOutputStream, or nullptr if not available.
 */
GOutputStream *RpFileGioPrivate::get_GOutputStream(void)
{
	// Likely a GIOStream, so check that first.
	if (G_IS_OUTPUT_STREAM(stream)) {
		return G_OUTPUT_STREAM(stream);
	} else if (G_IS_IO_STREAM(stream)) {
		return g_io_stream_get_output_stream(G_IO_STREAM(stream));
	}
	return nullptr;
}

/** RpFileGio **/

/**
 * Open a file.
 * NOTE: Files are always opened as read-only in binary mode.
 * @param uri GVfs URI.
 */
RpFileGio::RpFileGio(string_view uri, FileMode mode)
	: d_ptr(new RpFileGioPrivate(uri, mode))
{
	RP_D(RpFileGio);
	GError *err = nullptr;

	// Open the file.
	GFile *const file = g_file_new_for_uri(d->uri.c_str());

	switch (mode & FM_MODE_MASK) {
		case FM_OPEN_READ:
			d->stream = (GObject*)g_file_read(file, nullptr, &err);
			m_isWritable = false;
			break;
		case FM_CREATE_WRITE:
			// FIXME: sftp:// doesn't support read/write. Using write-only instead.
			//d->stream = (GObject*)g_file_replace_readwrite(file, nullptr, FALSE, (GFileCreateFlags)0, nullptr, &err);
			d->stream = (GObject*)g_file_replace(file, nullptr, FALSE, (GFileCreateFlags)0, nullptr, &err);
			m_isWritable = true;
			break;
		case FM_OPEN_WRITE:
			// FIXME: Not supported due to e.g. sftp:// not supporting read/write.
			//d->stream = (GObject*)g_file_open_readwrite(file, nullptr, &err);
			m_lastError = ENOTSUP;
			g_object_unref(file);
			return;
		default:
			// Invalid mode.
			m_lastError = EINVAL;
			g_object_unref(file);
			return;
	}
	g_object_unref(file);

	if (!d->stream || err != nullptr) {
		// An error occurred.
		if (err) {
			m_lastError = d->gioerr_to_posix(err->code);
			g_error_free(err);
		} else {
			// No GError...
			m_lastError = EIO;
		}

		g_clear_object(&d->stream);
		return;
	}

	// File is open.
	// TODO: Transparent gzip decompression?
}

RpFileGio::~RpFileGio()
{
	delete d_ptr;
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpFileGio::isOpen(void) const
{
	RP_D(const RpFileGio);
	return (d->stream != nullptr);
}

/**
 * Close the file.
 */
void RpFileGio::close(void)
{
	RP_D(RpFileGio);
	g_clear_object(&d->stream);
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpFileGio::read(void *ptr, size_t size)
{
	RP_D(RpFileGio);
	if (!d->stream) {
		m_lastError = EBADF;
		return 0;
	}

	GInputStream *const i_stream = d->get_GInputStream();
	if (!i_stream) {
		// No input stream...
		m_lastError = EBADF;
		return 0;
	}

	// TODO: Check if g_input_stream_read() has the same partial read
	// behavior as g_output_stream_write()'s partial write behavior.
	GError *err = nullptr;
	gssize ret = g_input_stream_read(i_stream, ptr, size, nullptr, &err);
	if (ret < 0 || err != nullptr) {
		// An error occurred.
		if (err) {
			m_lastError = d->gioerr_to_posix(err->code);
			g_error_free(err);
		} else {
			// No GError...
			m_lastError = EIO;
		}
		ret = 0;
	}

	return ret;
}

/**
 * Write data to the file.
 * (NOTE: Not valid for RpFileGio; this will always return 0.)
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t RpFileGio::write(const void *ptr, size_t size)
{
	RP_D(RpFileGio);
	if (!d->stream || !(d->mode & FM_WRITE)) {
		// Either the file isn't open,
		// or it's read-only.
		m_lastError = EBADF;
		return 0;
	}

	GOutputStream *const o_stream = d->get_GOutputStream();
	if (!o_stream) {
		// No input stream...
		m_lastError = EBADF;
		return 0;
	}

	// NOTE: g_output_stream_write() may do a partial write without an error.
	// If this happens, keep writing data until either we're done or it fails.
	// FIXME: "Out of disk space" doesn't return a valid error???
	GError *err = nullptr;
	size_t sz_total = 0;
	const uint8_t *ptr8 = static_cast<const uint8_t*>(ptr);
	do {
		gssize ret = g_output_stream_write(o_stream, ptr8, size, nullptr, &err);
		if (ret <= 0 || err != nullptr) {
			// An error occurred.
			if (err) {
				m_lastError = d->gioerr_to_posix(err->code);
				g_error_free(err);
			} else {
				// No GError...
				m_lastError = EIO;
			}
			break;
		}

		sz_total += ret;
		ptr8 += ret;
		if (likely(static_cast<size_t>(ret) <= size)) {
			size -= ret;
		} else {
			// Wrote more bytes than requested? Shouldn't happen...
			size = 0;
		}
	} while (size > 0);

	return sz_total;
}

/**
 * Set the file position.
 * @param pos		[in] File position
 * @param whence	[in] Where to seek from
 * @return 0 on success; -1 on error.
 */
int RpFileGio::seek(off64_t pos, SeekWhence whence)
{
	RP_D(RpFileGio);
	if (!G_IS_SEEKABLE(d->stream)) {
		m_lastError = EBADF;
		return -1;
	}

	GSeekable *const seekable = G_SEEKABLE(d->stream);
	if (!seekable || !g_seekable_can_seek(seekable)) {
		// Not a seekable stream.
		m_lastError = EBADF;
		return -1;
	}

	// NOTE: GSeekType does *not* map to stdio SeekWhence values.
	static const array<uint8_t /*GSeekType*/, 3> whenceToSeekType = {
		G_SEEK_SET,
		G_SEEK_CUR,
		G_SEEK_END,
	};
	if ((int)whence < 0 || (int)whence > (int)whenceToSeekType.size()) {
		// Invalid `whence`.
		m_lastError = EINVAL;
		return -1;
	}
	const GSeekType seekType = static_cast<GSeekType>(whenceToSeekType[(int)whence]);

	int ret = 0;
	GError *err = nullptr;

	// Seek in the stream.
	// NOTE: Assuming GFileIOStream will adjust seek pointers on both streams,
	// and/or both streams share the same seek pointer.
	gboolean bRet = g_seekable_seek(seekable, pos, seekType, nullptr, &err);
	if (bRet < 0 || err != nullptr) {
		// An error occurred.
		if (err) {
			m_lastError = d->gioerr_to_posix(err->code);
			g_error_free(err);
		} else {
			// No GError...
			m_lastError = EIO;
		}
		ret = -1;
	}

	return ret;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
off64_t RpFileGio::tell(void)
{
	RP_D(RpFileGio);
	if (!G_SEEKABLE(d->stream)) {
		m_lastError = EBADF;
		return -1;
	}

	GSeekable *const seekable = G_SEEKABLE(d->stream);
	if (!seekable || !g_seekable_can_seek(seekable)) {
		// Not a seekable.
		m_lastError = EBADF;
		return -1;
	}

	return g_seekable_tell(seekable);
}

/**
 * Truncate the file.
 * @param size New size. (default is 0)
 * @return 0 on success; -1 on error.
 */
int RpFileGio::truncate(off64_t size)
{
	RP_D(RpFileGio);
	if (!G_SEEKABLE(d->stream) || !(d->mode & FM_WRITE)) {
		// Either the file isn't open,
		// or it's read-only.
		m_lastError = EBADF;
		return -1;
	} else if (size < 0) {
		m_lastError = EINVAL;
		return -1;
	}

	GSeekable *const seekable = G_SEEKABLE(d->stream);
	if (!seekable || !g_seekable_can_seek(seekable) || !g_seekable_can_truncate(seekable)) {
		// Not a seekable or truncatable stream.
		m_lastError = EBADF;
		return -1;
	}

	// Get the current position.
	off64_t pos = g_seekable_tell(seekable);
	if (pos < 0) {
		m_lastError = EIO;
		return -1;
	}

	// Truncate the file.
	GError *err = nullptr;
	gboolean bRet = g_seekable_truncate(seekable, size, nullptr, &err);
	if (!bRet || err != nullptr) {
		// An error occurred.
		if (err) {
			m_lastError = d->gioerr_to_posix(err->code);
			g_error_free(err);
		} else {
			// No GError...
			m_lastError = EIO;
		}

		return -1;
	}

	// If the previous position was past the new
	// file size, reset the pointer.
	if (pos > size) {
		bRet = g_seekable_seek(seekable, pos, G_SEEK_SET, nullptr, &err);
		if (bRet < 0 || err != nullptr) {
			// An error occurred.
			if (err) {
				m_lastError = d->gioerr_to_posix(err->code);
				g_error_free(err);
			} else {
				// No GError...
				m_lastError = EIO;
			}
			return -1;
		}
	}

	// File truncated.
	return 0;
}

/** File properties **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
off64_t RpFileGio::size(void)
{
	RP_D(RpFileGio);
	if (!d->stream) {
		m_lastError = EBADF;
		return -1;
	}

	// TODO: Better error checking?
	GError *err = nullptr;
	GFileInfo *fileInfo = nullptr;
	if (G_IS_FILE_INPUT_STREAM(d->stream)) {
		fileInfo = g_file_input_stream_query_info(G_FILE_INPUT_STREAM(d->stream),
			G_FILE_ATTRIBUTE_STANDARD_SIZE, nullptr, &err);
	} else if (G_IS_FILE_OUTPUT_STREAM(d->stream)) {
		fileInfo = g_file_output_stream_query_info(G_FILE_OUTPUT_STREAM(d->stream),
			G_FILE_ATTRIBUTE_STANDARD_SIZE, nullptr, &err);
	} else if (G_IS_FILE_IO_STREAM(d->stream)) {
		fileInfo = g_file_io_stream_query_info(G_FILE_IO_STREAM(d->stream),
			G_FILE_ATTRIBUTE_STANDARD_SIZE, nullptr, &err);
	}

	if (!fileInfo || err != nullptr) {
		// An error occurred.
		if (err) {
			m_lastError = d->gioerr_to_posix(err->code);
			g_error_free(err);
		} else {
			// No GError...
			m_lastError = EIO;
		}

		g_clear_object(&fileInfo);
		return -1;
	}

	// Get the file size.
	const goffset fileSize = g_file_info_get_size(fileInfo);
	g_object_unref(fileInfo);
	return fileSize;
}

/**
 * Get the filename.
 * NOTE: For RpFileGio, this returns a GVfs URI.
 * @return Filename. (May be nullptr if the filename is not available.)
 */
const char *RpFileGio::filename(void) const
{
	RP_D(const RpFileGio);
	return d->uri.c_str();
}
