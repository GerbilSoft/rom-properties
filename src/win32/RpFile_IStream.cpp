/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RpFile_IStream.hpp: IRpFile using an IStream*.                          *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#include "stdafx.h"
#include "RpFile_IStream.hpp"

// libromdata
#include "libromdata/file/IRpFile.hpp"
using LibRomData::IRpFile;
using LibRomData::rp_string;

// C++ includes.
#include <string>
using std::string;

/**
 * Create an IRpFile using IStream* as the underlying storage mechanism.
 * @param pStream IStream*.
 */
RpFile_IStream::RpFile_IStream(IStream *pStream)
	: super()
	, m_pStream(pStream)
{
	pStream->AddRef();
}

RpFile_IStream::~RpFile_IStream()
{
	if (m_pStream) {
		m_pStream->Release();
	}
}

/**
 * Copy constructor.
 * @param other Other instance.
 */
RpFile_IStream::RpFile_IStream(const RpFile_IStream &other)
	: super()
	, m_pStream(other.m_pStream)
{
	// TODO: Combine with assignment constructor?
	m_pStream->AddRef();
	m_lastError = other.m_lastError;

	// Nothing else to do, since we can't actually
	// clone the stream.
}

/**
 * Assignment operator.
 * @param other Other instance.
 * @return This instance.
 */
RpFile_IStream &RpFile_IStream::operator=(const RpFile_IStream &other)
{
	// TODO: Combine with copy constructor?

	// If we have a stream open, close it first.
	if (m_pStream) {
		m_pStream->Release();
		m_pStream = nullptr;
	}

	m_pStream = other.m_pStream;
	m_pStream->AddRef();

	m_lastError = other.m_lastError;
	return *this;
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
 * dup() the file handle.
 *
 * Needed because IRpFile* objects are typically
 * pointers, not actual instances of the object.
 *
 * NOTE: The dup()'d IRpFile* does NOT have a separate
 * file pointer. This is due to how dup() works.
 *
 * @return dup()'d file, or nullptr on error.
 */
IRpFile *RpFile_IStream::dup(void)
{
	return new RpFile_IStream(*this);
}

/**
 * Close the file.
 */
void RpFile_IStream::close(void)
{
	if (m_pStream) {
		m_pStream->Release();
		m_pStream = nullptr;
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
	if (!m_pStream) {
		m_lastError = EBADF;
		return 0;
	}

	ULONG cbRead;
	HRESULT hr = m_pStream->Read(ptr, (ULONG)size, &cbRead);
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
 * @param pos File position.
 * @return 0 on success; -1 on error.
 */
int RpFile_IStream::seek(int64_t pos)
{
	if (!m_pStream) {
		m_lastError = EBADF;
		return -1;
	}

	LARGE_INTEGER dlibMove;
	dlibMove.QuadPart = pos;
	HRESULT hr = m_pStream->Seek(dlibMove, STREAM_SEEK_SET, nullptr);
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
int64_t RpFile_IStream::tell(void)
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

	return (int64_t)ulibNewPosition.QuadPart;
}

/**
 * Truncate the file.
 * @param size New size. (default is 0)
 * @return 0 on success; -1 on error.
 */
int RpFile_IStream::truncate(int64_t size)
{
	// TODO: Needs testing.
	if (!m_pStream) {
		m_lastError = EBADF;
		return -1;
	} else if (size < 0) {
		m_lastError = EINVAL;
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
	ulibNewSize.QuadPart = (uint64_t)size;
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

/** File properties. **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
int64_t RpFile_IStream::size(void)
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
	return (int64_t)statstg.cbSize.QuadPart;
}

/**
 * Get the filename.
 * @return Filename. (May be empty if the filename is not available.)
 */
rp_string RpFile_IStream::filename(void) const
{
	if (m_filename.empty()) {
		// Get the filename.
		// FIXME: This does NOT have the full path; only the
		// file portion is included. This is enough for the
		// file extension.
		STATSTG statstg;
		HRESULT hr = m_pStream->Stat(&statstg, STATFLAG_DEFAULT);
		if (FAILED(hr)) {
			// Stat() failed.
			return rp_string();
		}

		if (statstg.pwcsName) {
			// Save the filename.
			const_cast<RpFile_IStream*>(this)->m_filename = W2RP_c(statstg.pwcsName);
			CoTaskMemFree(statstg.pwcsName);
		}
	}

	// Return the filename.
	return m_filename;
}
