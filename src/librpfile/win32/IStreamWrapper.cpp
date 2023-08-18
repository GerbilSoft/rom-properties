/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * IStreamWrapper.cpp: IStream wrapper for IRpFile. (Win32)                *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "IStreamWrapper.hpp"

// librptext
#include "librptext/conversion.hpp"
#include "librptext/wchar.hpp"

// C++ STL classes
using std::shared_ptr;
using std::wstring;

namespace LibRpFile {

/** IUnknown **/
// Reference: https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/implementing-iunknown-in-c-plus-plus

IFACEMETHODIMP IStreamWrapper::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(IStreamWrapper, IStream),
		{ 0, 0 }
	};
#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::rp_QISearch(this, rgqit, riid, ppvObj);
}

/** ISequentialStream **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/api/objidl/nn-objidl-isequentialstream

IFACEMETHODIMP IStreamWrapper::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	if (!m_file) {
		return E_HANDLE;
	}

	size_t size = m_file->read(pv, cb);
	if (pcbRead) {
		*pcbRead = static_cast<ULONG>(size);
	}

	// FIXME: Return an error only if size == 0?
	return (size == static_cast<size_t>(cb) ? S_OK : S_FALSE);
}

IFACEMETHODIMP IStreamWrapper::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	if (!m_file) {
		return E_HANDLE;
	}

	size_t size = m_file->write(pv, cb);
	if (pcbWritten) {
		*pcbWritten = static_cast<ULONG>(size);
	}

	// FIXME: Return an error only if size == 0?
	return (size == static_cast<size_t>(cb) ? S_OK : S_FALSE);
}

/** IStream **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/api/objidl/nn-objidl-istream

IFACEMETHODIMP IStreamWrapper::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
	if (!m_file) {
		return E_HANDLE;
	}

	off64_t pos;
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

	const off64_t fileSize = static_cast<off64_t>(libNewSize.QuadPart);
	if (fileSize < 0) {
		// Out of bounds.
		return STG_E_INVALIDFUNCTION;
	}

	int ret = m_file->truncate(fileSize);
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
		ULONG toRead = (cb.QuadPart > static_cast<ULONG>(sizeof(buf))
			? static_cast<ULONG>(sizeof(buf))
			: static_cast<ULONG>(cb.QuadPart));
		size_t szRead = m_file->read(buf, toRead);
		if (szRead == 0) {
			// Read error.
			hr = STG_E_READFAULT;
			break;
		}
		totalRead.QuadPart += szRead;

		// Write the data to the destination stream.
		ULONG ulWritten;
		hr = pstm->Write(buf, static_cast<ULONG>(szRead), &ulWritten);
		if (FAILED(hr)) {
			// Write failed.
			break;
		}
		totalWritten.QuadPart += ulWritten;

		if (static_cast<ULONG>(szRead) != toRead ||
			ulWritten != static_cast<ULONG>(szRead))
		{
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
	RP_UNUSED(grfCommitFlags);
	return S_OK;
}

IFACEMETHODIMP IStreamWrapper::Revert(void)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP IStreamWrapper::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	RP_UNUSED(libOffset);
	RP_UNUSED(cb);
	RP_UNUSED(dwLockType);
	return E_NOTIMPL;
}

IFACEMETHODIMP IStreamWrapper::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	RP_UNUSED(libOffset);
	RP_UNUSED(cb);
	RP_UNUSED(dwLockType);
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
		const char *const u8_filename = m_file->filename();
		const wstring wfilename(u8_filename ? U82W_c(u8_filename) : L"");
		const size_t sz = (wfilename.size() + 1) * sizeof(wchar_t);
		pstatstg->pwcsName = static_cast<LPOLESTR>(CoTaskMemAlloc(sz));
		if (!pstatstg->pwcsName) {
			return E_OUTOFMEMORY;
		}
		memcpy(pstatstg->pwcsName, wfilename.c_str(), sz);
	}

	pstatstg->type = STGTY_STREAM;	// TODO: or STGTY_STORAGE?

	const off64_t fileSize = m_file->size();
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
