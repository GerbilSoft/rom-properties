/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RpFile_IStream.hpp: IRpFile using an IStream*.                          *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpbase.h"
#include "RpFile_IStream.hpp"

// Other rom-properties libraries
#include "librpbyteswap/byteswap_rp.h"
#include "librptext/wchar.hpp"

// C++ STL classes
using std::string;
using std::unique_ptr;

// zlib buffer size.
static constexpr size_t ZLIB_BUFFER_SIZE = 16384U;

#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

/**
 * Create an IRpFile using IStream* as the underlying storage mechanism.
 * NOTE: RpFile_IStream does NOT take ownership of the IStream*.
 * @param pStream	[in] IStream*
 */
RpFile_IStream::RpFile_IStream(IStream *pStream)
	: super()
	, m_pStream(pStream, true)	// true == call AddRef()
{
	// TODO: Proper writable check.
	m_isWritable = true;
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpFile_IStream::isOpen(void) const
{
	return (m_pStream != nullptr);
}

/**
 * Close the file.
 */
void RpFile_IStream::close(void)
{
	if (m_pStream) {
		m_pStream.Release();
	}
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpFile_IStream::read(void *ptr, size_t size)
{
	HRESULT hr;

	if (!m_pStream) {
		m_lastError = EBADF;
		return 0;
	}

	ULONG cbRead;
	hr = m_pStream->Read(ptr, (ULONG)size, &cbRead);
	if (FAILED(hr)) {
		// An error occurred.
		// TODO: Convert hr to POSIX?
		m_lastError = EIO;
		return 0;
	}

	return (size_t)cbRead;
}

/**
 * Write data to the file.
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t RpFile_IStream::write(const void *ptr, size_t size)
{
	if (!m_pStream) {
		// TODO: Read-only check?
		m_lastError = EBADF;
		return 0;
	}

	ULONG cbWritten;
	HRESULT hr = m_pStream->Write(ptr, (ULONG)size, &cbWritten);
	if (FAILED(hr)) {
		// An error occurred.
		// TODO: Convert HRESULT to POSIX?
		m_lastError = EIO;
		return 0;
	}

	return (size_t)cbWritten;
}

/**
 * Set the file position.
 * @param pos		[in] File position
 * @param whence	[in] Where to seek from
 * @return 0 on success; -1 on error.
 */
int RpFile_IStream::seek(off64_t pos, SeekWhence whence)
{
	LARGE_INTEGER dlibMove;
	HRESULT hr;

	if (!m_pStream) {
		m_lastError = EBADF;
		return -1;
	}

	// Seek in the base stream.
	dlibMove.QuadPart = pos;
	hr = m_pStream->Seek(dlibMove, static_cast<DWORD>(whence), nullptr);
	if (FAILED(hr)) {
		// TODO: Convert hr to POSIX?
		m_lastError = EIO;
		return -1;
	}

	return 0;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
off64_t RpFile_IStream::tell(void)
{
	if (!m_pStream) {
		m_lastError = EBADF;
		return -1;
	}

	LARGE_INTEGER dlibMove;
	ULARGE_INTEGER ulibNewPosition;
	dlibMove.QuadPart = 0;
	HRESULT hr = m_pStream->Seek(dlibMove, STREAM_SEEK_CUR, &ulibNewPosition);
	if (FAILED(hr)) {
		// TODO: Convert HRESULT to POSIX?
		m_lastError = EIO;
		return -1;
	}

	return static_cast<off64_t>(ulibNewPosition.QuadPart);
}

/**
 * Truncate the file.
 * @param size New size. (default is 0)
 * @return 0 on success; -1 on error.
 */
int RpFile_IStream::truncate(off64_t size)
{
	// TODO: Needs testing.
	if (!m_pStream) {
		m_lastError = EBADF;
		return -1;
	} else if (size < 0) {
		m_lastError = EINVAL;
		return -1;
	} else if (!isWritable()) {
		// Not writable...
		m_lastError = EPERM;
		return -1;
	}

	// Get the current stream position.
	LARGE_INTEGER dlibMove;
	ULARGE_INTEGER ulibNewPosition;
	dlibMove.QuadPart = 0;
	HRESULT hr = m_pStream->Seek(dlibMove, STREAM_SEEK_CUR, &ulibNewPosition);
	if (FAILED(hr)) {
		// TODO: Convert HRESULT to POSIX?
		m_lastError = EIO;
		return -1;
	}

	// Truncate the stream.
	ULARGE_INTEGER ulibNewSize;
	ulibNewSize.QuadPart = static_cast<ULONGLONG>(size);
	hr = m_pStream->SetSize(ulibNewSize);
	if (FAILED(hr)) {
		// TODO: Convert HRESULT to POSIX?
		m_lastError = EIO;
		return -1;
	}

	// If the previous position was past the new
	// stream size, reset the pointer.
	if (ulibNewPosition.QuadPart > ulibNewSize.QuadPart) {
		dlibMove.QuadPart = size;
		hr = m_pStream->Seek(dlibMove, STREAM_SEEK_SET, nullptr);
		if (FAILED(hr)) {
			// TODO: Convert HRESULT to POSIX?
			m_lastError = EIO;
			return -1;
		}
	}

	// Stream truncated.
	return 0;
}

/**
 * Flush buffers.
 * This operation only makes sense on writable files.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpFile_IStream::flush(void)
{
	// TODO: Needs testing.
	if (!m_pStream) {
		m_lastError = EBADF;
		return -1;
	}

	if (isWritable()) {
		HRESULT hr = m_pStream->Commit(STGC_DEFAULT);
		if (FAILED(hr)) {
			// TODO: Convert HRESULT to POSIX?
			m_lastError = EIO;
			return -m_lastError;
		}
		return 0;
	}

	// Ignore flush operations if the file isn't writable.
	return 0;
}

/** File properties **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
off64_t RpFile_IStream::size(void)
{
	if (!m_pStream) {
		m_lastError = EBADF;
		return -1;
	}

	// Use Stat() instead of Seek().
	// TODO: Fallback if Stat() has no size?
	STATSTG statstg;
	HRESULT hr = m_pStream->Stat(&statstg, STATFLAG_NONAME);
	if (FAILED(hr)) {
		// Stat() failed.
		// TODO: Try Seek() instead?
		return -1;
	}

	// TODO: Make sure cbSize is valid?
	return static_cast<off64_t>(statstg.cbSize.QuadPart);
}

/**
 * Get the filename.
 * @return Filename. (May be nullptr if the filename is not available.)
 */
const char *RpFile_IStream::filename(void) const
{
	// NOTE: Assuming that if m_filename is empty, it hasn't been obtained yet.
	// TODO: Cache "filename obtained" separately?
	if (m_filename.empty()) {
		// Get the filename.
		// FIXME: This does NOT have the full path; only the
		// file portion is included. This is enough for the
		// file extension.
		STATSTG statstg;
		HRESULT hr = m_pStream->Stat(&statstg, STATFLAG_DEFAULT);
		if (FAILED(hr)) {
			// Stat() failed.
			return nullptr;
		}

		if (statstg.pwcsName) {
			// Save the filename.
			const_cast<RpFile_IStream*>(this)->m_filename = W2U8_str(statstg.pwcsName);
			CoTaskMemFree(statstg.pwcsName);
		}
	}

	// Return the filename.
	return (likely(!m_filename.empty())) ? m_filename.c_str() : nullptr;
}
