/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IStreamWrapper.cpp: IStream wrapper for IRpFile. (Win32)                *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include "IStreamWrapper.hpp"

// C++ includes.
#include <string>
using std::wstring;

namespace LibRomData {

/**
 * Create an IStream wrapper for IRpFile.
 * The IRpFile is dup()'d.
 * @param file IRpFile.
 */
IStreamWrapper::IStreamWrapper(IRpFile *file)
	: m_ulRefCount(1)
{
	if (file) {
		// dup() the file.
		m_file = file->dup();
	} else {
		// No file specified.
		m_file = nullptr;
	}
}

IStreamWrapper::~IStreamWrapper()
{
	delete m_file;
}

/**
 * Get the IRpFile.
 * NOTE: The IRpFile is still owned by this object.
 * @return IRpFile.
 */
IRpFile *IStreamWrapper::file(void) const
{
	return m_file;
}

/**
 * Set the IRpFile.
 * @param file New IRpFile.
 */
void IStreamWrapper::setFile(IRpFile *file)
{
	if (m_file) {
		IRpFile *old = m_file;
		if (file) {
			m_file = file->dup();
		} else {
			m_file = nullptr;
		}
		delete old;
	} else {
		if (file) {
			m_file = file->dup();
		}
	}
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP IStreamWrapper::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	// Always set out parameter to NULL, validating it first.
	if (!ppvObj)
		return E_INVALIDARG;

	// Check if this interface is supported.
	// NOTE: static_cast<> is required due to vtable shenanigans.
	// Also, IID_IUnknown must always return the same pointer.
	// References:
	// - http://stackoverflow.com/questions/1742848/why-exactly-do-i-need-an-explicit-upcast-when-implementing-queryinterface-in-a
	// - http://stackoverflow.com/a/2812938
	if (riid == IID_IUnknown || riid == IID_ISequentialStream) {
		*ppvObj = static_cast<ISequentialStream*>(this);
	} else if (riid == IID_IStream) {
		*ppvObj = static_cast<IStream*>(this);
	} else {
		// Interface is not supported.
		*ppvObj = nullptr;
		return E_NOINTERFACE;
	}

	// Make sure we count this reference.
	AddRef();
	return NOERROR;
}

IFACEMETHODIMP_(ULONG) IStreamWrapper::AddRef(void)
{
	//InterlockedIncrement(&RP_ulTotalRefCount);	// FIXME
	InterlockedIncrement(&m_ulRefCount);
	return m_ulRefCount;
}

IFACEMETHODIMP_(ULONG) IStreamWrapper::Release(void)
{
	ULONG ulRefCount = InterlockedDecrement(&m_ulRefCount);
	if (ulRefCount == 0) {
		/* No more references. */
		delete this;
	}

	//InterlockedDecrement(&RP_ulTotalRefCount);	// FIXME
	return ulRefCount;
}

/** ISequentialStream **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/aa380010(v=vs.85).aspx

IFACEMETHODIMP IStreamWrapper::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	if (!m_file) {
		return E_HANDLE;
	}

	size_t size = m_file->read(pv, cb);
	if (pcbRead) {
		*pcbRead = (ULONG)size;
	}

	// FIXME: Return an error only if size == 0?
	return (size == (size_t)cb ? S_OK : S_FALSE);
}

IFACEMETHODIMP IStreamWrapper::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	if (!m_file) {
		return E_HANDLE;
	}

	size_t size = m_file->write(pv, cb);
	if (pcbWritten) {
		*pcbWritten = (ULONG)size;
	}

	// FIXME: Return an error only if size == 0?
	return (size == (size_t)cb ? S_OK : S_FALSE);
}

/** IStream **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/aa380034(v=vs.85).aspx

IFACEMETHODIMP IStreamWrapper::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
	if (!m_file) {
		return E_HANDLE;
	}

	int64_t pos;
	switch (dwOrigin) {
		case STREAM_SEEK_SET:
			m_file->seek(dlibMove.QuadPart);
			break;
		case STREAM_SEEK_CUR:
			pos = m_file->tell();
			pos += dlibMove.QuadPart;
			m_file->seek(pos);
			break;
		case STREAM_SEEK_END:
			pos = m_file->size();
			pos += dlibMove.QuadPart;
			m_file->seek(pos);
			break;
		default:
			return E_INVALIDARG;
	}

	if (plibNewPosition) {
		plibNewPosition->QuadPart = m_file->tell();
	}

	return S_OK;
}

IFACEMETHODIMP IStreamWrapper::SetSize(ULARGE_INTEGER libNewSize)
{
	if (!m_file) {
		return E_HANDLE;
	}

	int64_t size = (int64_t)libNewSize.QuadPart;
	if (size < 0) {
		// Out of bounds.
		return STG_E_INVALIDFUNCTION;
	}

	int ret = m_file->truncate(size);
	HRESULT hr = S_OK;
	if (ret != 0) {
		switch (m_file->lastError()) {
			case ENOSPC:
				hr = STG_E_MEDIUMFULL;
				break;
			case EIO:
				hr = STG_E_INVALIDFUNCTION;
				break;

			case ENOTSUP:	// NOT STG_E_INVALIDFUNCTION;
					// that's for "size not supported".
			default:
				// Unknown...
				hr = E_FAIL;
				break;
		}
	}

	return hr;
}

/**
 * Copy data from this stream to another stream.
 * @param pstm		[in] Destination stream.
 * @param cb		[in] Number of bytes to copy.
 * @param pcbRead	[out,opt] Number of bytes read from the source.
 * @param pcbWritten	[out,opt] Number of bytes written to the destination.
 */
IFACEMETHODIMP IStreamWrapper::CopyTo(IStream *pstm, ULARGE_INTEGER cb,
		ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
	// FIXME: Totally untested.

	if (!m_file) {
		// No file handle.
		return E_HANDLE;
	}

	// Copy 4 KB at a time.
	uint8_t buf[4096];
	ULARGE_INTEGER totalRead, totalWritten;
	totalRead.QuadPart = 0;
	totalWritten.QuadPart = 0;

	HRESULT hr = S_OK;
	while (cb.QuadPart > 0) {
		ULONG toRead = (cb.QuadPart > (ULONG)sizeof(buf) ? (ULONG)sizeof(buf) : (ULONG)cb.QuadPart);
		size_t szRead = m_file->read(buf, toRead);
		if (szRead == 0) {
			// Read error.
			hr = STG_E_READFAULT;
			break;
		}
		totalRead.QuadPart += szRead;

		// Write the data to the destination stream.
		ULONG ulWritten;
		hr = pstm->Write(buf, (ULONG)szRead, &ulWritten);
		if (FAILED(hr)) {
			// Write failed.
			break;
		}
		totalWritten.QuadPart += ulWritten;

		if ((ULONG)szRead != toRead || ulWritten != (ULONG)szRead) {
			// EOF or out of space.
			break;
		}

		// Next segment.
		cb.QuadPart -= toRead;
	}

	if (pcbRead) {
		*pcbRead = totalRead;
	}
	if (pcbWritten) {
		*pcbWritten = totalWritten;
	}

	return hr;
}

IFACEMETHODIMP IStreamWrapper::Commit(DWORD grfCommitFlags)
{
	// NOTE: Returning S_OK, even though we're not doing anything here.
	((void)grfCommitFlags);
	return S_OK;
}

IFACEMETHODIMP IStreamWrapper::Revert(void)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP IStreamWrapper::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	((void)libOffset);
	((void)cb);
	((void)dwLockType);
	return E_NOTIMPL;
}

IFACEMETHODIMP IStreamWrapper::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	((void)libOffset);
	((void)cb);
	((void)dwLockType);
	return E_NOTIMPL;
}

IFACEMETHODIMP IStreamWrapper::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
	// TODO: Initialize STATSTG on file open?
	if (!m_file) {
		return E_HANDLE;
	}

	if (grfStatFlag & STATFLAG_NONAME) {
		pstatstg->pwcsName = nullptr;
	} else {
		// Copy the filename
		// TODO: Is nullptr for empty filename allowed?
		// For now, we'll just return an empty name.
		// TODO: RP2W_ss() that returns a wstring?
		wstring filename = RP2W_s(m_file->filename());
		const size_t sz = (filename.size() + 1) * sizeof(wchar_t);
		pstatstg->pwcsName = static_cast<LPOLESTR>(CoTaskMemAlloc(sz));
		if (!pstatstg->pwcsName) {
			return E_OUTOFMEMORY;
		}
		memcpy(pstatstg->pwcsName, filename.c_str(), sz);
	}

	pstatstg->type = STGTY_STREAM;	// TODO: or STGTY_STORAGE?

	const int64_t fileSize = m_file->size();
	pstatstg->cbSize.QuadPart = (fileSize > 0 ? fileSize : 0);

	// No timestamps are available...
	// TODO: IRpFile::stat()?
	pstatstg->mtime.dwLowDateTime  = 0;
	pstatstg->mtime.dwHighDateTime = 0;
	pstatstg->ctime.dwLowDateTime  = 0;
	pstatstg->ctime.dwHighDateTime = 0;
	pstatstg->atime.dwLowDateTime  = 0;
	pstatstg->atime.dwHighDateTime = 0;

	pstatstg->grfMode = STGM_READ;	// TODO: STGM_WRITE?
	pstatstg->grfLocksSupported = 0;
	pstatstg->clsid = CLSID_NULL;
	pstatstg->grfStateBits = 0;
	pstatstg->reserved = 0;

	return S_OK;
}

IFACEMETHODIMP IStreamWrapper::Clone(IStream **ppstm)
{
	if (!ppstm)
		return STG_E_INVALIDPOINTER;
	*ppstm = new IStreamWrapper(m_file);
	return S_OK;
}

}
