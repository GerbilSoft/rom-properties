/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IStreamWrapper.cpp: IStream wrapper for IRpFile. (Win32)                *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "IStreamWrapper.hpp"

// Text conversion functions and macros.
#include "../../TextFuncs.hpp"
#include "../../TextFuncs_wchar.hpp"

// C++ STL classes.
using std::wstring;

namespace LibRpBase {

/**
 * Create an IStream wrapper for IRpFile.
 * The IRpFile is ref()'d.
 * @param file IRpFile.
 */
IStreamWrapper::IStreamWrapper(IRpFile *file)
{
	if (file) {
		// ref() the file.
		m_file = file->ref();
	} else {
		// No file specified.
		m_file = nullptr;
	}
}

IStreamWrapper::~IStreamWrapper()
{
	if (m_file) {
		m_file->unref();
	}
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
		IRpFile *const old = m_file;
		if (file) {
			m_file = file->ref();
		} else {
			m_file = nullptr;
		}
		old->unref();
	} else {
		if (file) {
			m_file = file->ref();
		}
	}
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP IStreamWrapper::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(IStreamWrapper, IStream),
		{ 0, 0 }
	};
#ifdef _MSC_VER
# pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::pfnQISearch(this, rgqit, riid, ppvObj);
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

	int64_t size = static_cast<int64_t>(libNewSize.QuadPart);
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
		// TODO: U82W_ss() that returns a wstring?
		wstring filename = U82W_s(m_file->filename());
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
