/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_gio.cpp: IRpFile implementation using GIO/GVfs.                  *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpFile_gio.hpp"

// librpfile
using LibRpFile::IRpFile;

// gio
#include <gio/gio.h>

// C includes (C++ namespace)
#include <cerrno>

// C++ STL classes
#include <array>
#include <string>
using std::array;
using std::string;
using std::string_view;

class RpFileGioPrivate
{
public:
	explicit RpFileGioPrivate(string_view uri, IRpFile::FileMode mode);
	~RpFileGioPrivate();

public:
	RP_DISABLE_COPY(RpFileGioPrivate)

public:
	// File stream. May be one of the following:
	// - GFileInputStream (read-only)
	// - GFileIOStream (read-write)
	// - GFileOutputStream (if opened with FM_CREATE_WRITE but using e.g. sftp://,
	//                      which doesn't support read-write)
	GObject *stream;
	GFile *file;

	string uri;		// GVfs URI
	IRpFile::FileMode mode;	// File mode

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

	/**
	 * (Re-)Open the stream.
	 * d->file must already be initialized.
	 * @param mode		[in] Requested file mode
	 * @param p_isWritable	[out] Indicates if the opened stream is writable
	 * @param p_err		[out] POSIX error code on error
	 * @return GIO stream object, or nullptr on error.
	 */
	GObject *reOpenStream(IRpFile::FileMode mode, bool *p_isWritable, int *p_err);
};

/** RpFileGioPrivate **/

RpFileGioPrivate::RpFileGioPrivate(string_view uri, IRpFile::FileMode mode)
	: stream(nullptr)
	, file(nullptr)
	, mode(mode)
{
	if (!uri.empty()) {
		STRING_ASSIGN_SV(this->uri, uri);
		file = g_file_new_for_uri(this->uri.c_str());
	}
}

RpFileGioPrivate::~RpFileGioPrivate()
{
	g_clear_object(&stream);
	g_clear_object(&file);
}

/**
 * Convert a GIO error code to POSIX.
 * @param gioerr GIO error code.
 * @return POSIX error code.
 */
int RpFileGioPrivate::gioerr_to_posix(gint gioerr)
{
	// glib has g_io_error_from_errno(), but not the other way around.
	// This function implements the other way around, using the most
	// likely POSIX error code. If a particular error was not listed
	// in g_io_error_from_errno(), we'll come up with our own mapping.

	// NOTE: Special values:
	// - 0: Not actually an error.
	// - If an error code isn't defined by the system, we'll default to EIO.

#ifndef EFTYPE
// Not available on Linux as of glibc-2.44.
#  define EFTYPE EIO
#endif

	static const uint8_t gio_err_to_posix[] = {
		EIO,		// G_IO_ERROR_FAILED
		ENOENT,		// G_IO_ERROR_NOT_FOUND
		EEXIST,		// G_IO_ERROR_EXISTS
		EISDIR,		// G_IO_ERROR_IS_DIRECTORY
		ENOTDIR,	// G_IO_ERROR_NOT_DIRECTORY
		ENOTEMPTY,	// G_IO_ERROR_NOT_EMPTY
		EFTYPE,		// G_IO_ERROR_NOT_REGULAR_FILE (FIXME)
		EFTYPE,		// G_IO_ERROR_NOT_SYMBOLIC_LINK (FIXME)
		EFTYPE,		// G_IO_ERROR_NOT_MOUNTABLE_FILE (FIXME)
		ENAMETOOLONG,	// G_IO_ERROR_FILENAME_TOO_LONG
		EINVAL,		// G_IO_ERROR_INVALID_FILENAME (FIXME)
		EMLINK,		// G_IO_ERROR_TOO_MANY_LINKS
		ENOSPC,		// G_IO_ERROR_NO_SPACE
		ENOTSOCK,	// G_IO_ERROR_INVALID_ARGUMENT (FIXME: Not EINVAL?)
		EACCES,		// G_IO_ERROR_PERMISSION_DENIED
		ENOTSUP,	// G_IO_ERROR_NOT_SUPPORTED
		EIO,		// G_IO_ERROR_NOT_MOUNTED (FIXME)
		EIO,		// G_IO_ERROR_ALREADY_MOUNTED (FIXME)
		EBADF,		// G_IO_ERROR_CLOSED (FIXME)
		ECANCELED,	// G_IO_ERROR_CANCELLED
		EAGAIN,		// G_IO_ERROR_PENDING (FIXME)
		EROFS,		// G_IO_ERROR_READ_ONLY (FIXME)
		EIO,		// G_IO_ERROR_CANT_CREATE_BACKUP (FIXME)
		EIO,		// G_IO_ERROR_WRONG_ETAG (FIXME)
		ETIMEDOUT,	// G_IO_ERROR_TIMED_OUT (FIXME)
		ELOOP,		// G_IO_ERROR_WOULD_RECURSE (FIXME)
		EBUSY,		// G_IO_ERROR_BUSY
		EWOULDBLOCK,	// G_IO_ERROR_WOULD_BLOCK (NOTE: May be the same as EAGAIN)
		ENOENT,		// G_IO_ERROR_HOST_NOT_FOUND (FIXME)
		EEXIST,		// G_IO_ERROR_WOULD_MERGE (FIXME)
		0,		// G_IO_ERROR_FAILED_HANDLED (FIXME)
		EMFILE,		// G_IO_ERROR_TOO_MANY_OPEN_FILES
		EINVAL,		// G_IO_ERROR_NOT_INITIALIZED (FIXME)
		EADDRINUSE,	// G_IO_ERROR_ADDRESS_IN_USE
		EAGAIN,		// G_IO_ERROR_PARTIAL_INPUT (FIXME)
		EBADMSG,	// G_IO_ERROR_INVALID_DATA
		EIO,		// G_IO_ERROR_DBUS_ERROR (FIXME)
		EHOSTUNREACH,	// G_IO_ERROR_HOST_UNREACHABLE
		ENETUNREACH,	// G_IO_ERROR_NETWORK_UNREACHABLE
		ECONNREFUSED,	// G_IO_ERROR_CONNECTION_REFUSED
		EIO,		// G_IO_ERROR_PROXY_FAILED (FIXME)
		EIO,		// G_IO_ERROR_PROXY_AUTH_FAILED (FIXME)
		EIO,		// G_IO_ERROR_PROXY_NEED_AUTH (FIXME)
		EPERM,		// G_IO_ERROR_PROXY_NOT_ALLOWED (FIXME)
		/*not used*/	// G_IO_ERROR_BROKEN_PIPE
		ECONNRESET,	// G_IO_ERROR_CONNECTION_CLOSED = G_IO_ERROR_BROKEN_PIPE
		ENOTCONN,	// G_IO_ERROR_NOT_CONNECTED
		EMSGSIZE,	// G_IO_ERROR_MESSAGE_TOO_LARGE
		ENXIO,		// G_IO_ERROR_NO_SUCH_DEVICE
		EDESTADDRREQ,	// G_IO_ERROR_DESTINATION_UNSET
	};

	if (gioerr >= 0 && gioerr < ARRAY_SIZE_I(gio_err_to_posix)) {
		return gio_err_to_posix[gioerr];
	}

	// Default to EIO.
	return EIO;
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
	if (G_IS_IO_STREAM(stream)) {
		return g_io_stream_get_output_stream(G_IO_STREAM(stream));
	} else if (G_IS_OUTPUT_STREAM(stream)) {
		return G_OUTPUT_STREAM(stream);
	}
	return nullptr;
}

/**
 * (Re-)Open the stream.
 * d->file must already be initialized.
 * @param mode		[in] Requested file mode
 * @param p_isWritable	[out] Indicates if the opened stream is writable
 * @param p_err		[out] POSIX error code on error
 * @return GIO stream object, or nullptr on error.
 */
GObject *RpFileGioPrivate::reOpenStream(IRpFile::FileMode mode, bool *p_isWritable, int *p_err)
{
	assert(p_isWritable != nullptr);
	assert(p_err != nullptr);

	// Initialize the output variables.
	*p_isWritable = false;
	*p_err = 0;

	// Make sure the GFile has been opened.
	assert(file != nullptr);
	if (!file) {
		// Not opened...
		*p_err = EBADF;
		return nullptr;
	}

	GObject *stream = nullptr;
	GError *g_err = nullptr;

	switch (mode & IRpFile::FM_MODE_MASK) {
		case IRpFile::FM_OPEN_READ:
			stream = (GObject*)g_file_read(file, nullptr, &g_err);
			break;

		case IRpFile::FM_CREATE_WRITE:
			stream = (GObject*)g_file_replace_readwrite(file, nullptr, FALSE, (GFileCreateFlags)0, nullptr, &g_err);
			if (stream) {
				// Read/write stream opened successfully.
				*p_isWritable = true;
				break;
			}

			// g_file_replace_readwrite() failed...
			if (g_err) {
				const int g_err_code = g_err->code;
				g_clear_error(&g_err);
				if (g_err_code != G_IO_ERROR_NOT_SUPPORTED) {
					// G_IO_ERROR_NOT_SUPPORTED is returned if the protocl does not support read/write.
					// Anything else is an actual error.
					*p_err = gioerr_to_posix(g_err_code);
					return nullptr;
				}
			}

			// URI does not support simultaneous read/write. (e.g. sftp://)
			// We'll still open it for write-only access.
			stream = (GObject*)g_file_replace(file, nullptr, FALSE, (GFileCreateFlags)0, nullptr, &g_err);
			if (stream) {
				*p_isWritable = true;
			}
			break;

		case IRpFile::FM_OPEN_WRITE:
			stream = (GObject*)g_file_open_readwrite(file, nullptr, &g_err);
			if (stream) {
				// Read/write stream opened successfully.
				*p_isWritable = true;
				break;
			}

			// g_file_open_readwrite() failed...
			if (g_err) {
				const int g_err_code = g_err->code;
				g_clear_error(&g_err);
				if (g_err_code != G_IO_ERROR_NOT_SUPPORTED) {
					// G_IO_ERROR_NOT_SUPPORTED is returned if the protocl does not support read/write.
					// Anything else is an actual error.
					*p_err = gioerr_to_posix(g_err_code);
					return nullptr;
				}
			}

			// URI does not support simultaneous read/write. (e.g. sftp://)
			// We'll still open it for read-only access.
			stream = (GObject*)g_file_read(file, nullptr, &g_err);
			break;

		default:
			// Invalid mode.
			*p_err = EINVAL;
			return nullptr;
	}

	if (g_err) {
		// An error occurred...
		*p_isWritable = false;
		*p_err = gioerr_to_posix(g_err->code);
		g_clear_error(&g_err);
	} else if (!stream) {
		// No stream?
		*p_isWritable = false;
		*p_err = EIO;
	}

	return stream;
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
	// Make sure the GFile has been opened.
	RP_D(RpFileGio);
	assert(d->file != nullptr);
	if (!d->file) {
		// Not opened...
		m_lastError = EBADF;
		return;
	}

	// Get the file type.
	GFileInfo *const fi = g_file_query_info(d->file,
		G_FILE_ATTRIBUTE_STANDARD_TYPE,
		G_FILE_QUERY_INFO_NONE, nullptr, nullptr);
	if (fi) {
		uint32_t fileType = g_file_info_get_attribute_uint32(fi, G_FILE_ATTRIBUTE_STANDARD_TYPE);
		switch (fileType) {
			default:
			case G_FILE_TYPE_UNKNOWN:
			case G_FILE_TYPE_SHORTCUT:	// Windows .lnk shortcut
				// Unknown file type...
				m_fileType = DT_UNKNOWN;
				m_lastError = EIO;	// TODO: Get error?
				break;

			case G_FILE_TYPE_REGULAR:
				m_fileType = DT_REG;
				break;

			case G_FILE_TYPE_DIRECTORY:
			case G_FILE_TYPE_MOUNTABLE:	// Mount point
				// This is a directory.
				// Directories for e.g. NUS packages are handled separately in RomDataFactory.
				// TODO: That doesn't work for GIO...
				m_fileType = DT_DIR;
				m_lastError = EISDIR;
				break;

			case G_FILE_TYPE_SYMBOLIC_LINK:
				// Shouldn't happen, but we'll treat it as DT_REG for now.
				assert(!"Symbolic link???");
				m_fileType = DT_REG;
				break;

			case G_FILE_TYPE_SPECIAL:
				// FIXME: May be a socket, fifo, block device, or character device.
				// Block devices are allowed, but remote block device access
				// doesn't make much sense, so prevent it from working in case
				// it's a problem.
				m_fileType = DT_BLK;
				m_lastError = ENOTSUP;
				break;
		}

		if (m_lastError != 0) {
			return;
		}
	}

	int err = 0;
	d->stream = d->reOpenStream(mode, &m_isWritable, &err);
	if (!d->stream) {
		// Error opening the stream.
		m_isWritable = false;
		m_lastError = err;
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
		m_lastError = ESPIPE;
		return -1;
	}

	// NOTE: GSeekType does *not* map to stdio SeekWhence values.
	static const array<uint8_t /*GSeekType*/, 3> whenceToSeekType = {
		G_SEEK_SET,
		G_SEEK_CUR,
		G_SEEK_END,
	};
	if (static_cast<int>(whence) < 0 || static_cast<int>(whence) > static_cast<int>(whenceToSeekType.size())) {
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
		// Not a seekable stream.
		m_lastError = ESPIPE;
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
		m_lastError = ESPIPE;
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
	// TODO: Use d->file instead of d->stream?
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

	// NOTE: We *could* retrieve the URI from the GFile instead of saving
	// it in a separate std::string, but that returns an allocated string,
	// and the string returned by this function is not expected to be
	// freed by the caller, so that would result in a memory leak.
	return d->uri.c_str();
}

/**
 * Get the file modification time.
 * @return File modification time, or -1 if not available.
 */
rp_time_t RpFileGio::mtime(void)
{
	RP_D(RpFileGio);
	if (!d->file) {
		m_lastError = EBADF;
		return -1;
	}

	GError *err = nullptr;
	GFileInfo *const fi = g_file_query_info(d->file,
		G_FILE_ATTRIBUTE_TIME_MODIFIED,
		G_FILE_QUERY_INFO_NONE, nullptr, &err);
	if (!fi) {
		if (err) {
			m_lastError = d->gioerr_to_posix(err->code);
			g_error_free(err);
		} else {
			m_lastError = EBADF;
		}
		return -1;
	}

	// Get the modification time.
	// NOTE: The GVfs mtime is **uint64_t**, not int64_t.
	rp_time_t mtime = static_cast<rp_time_t>(
		g_file_info_get_attribute_uint64(
			fi, G_FILE_ATTRIBUTE_TIME_MODIFIED));
	g_object_unref(fi);
	return mtime;
}
