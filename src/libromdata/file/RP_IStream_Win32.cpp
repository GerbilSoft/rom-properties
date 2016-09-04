/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RP_IStream_Win32.hpp: IStream wrapper for IRpFile. (Win32)              *
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

#include "RP_IStream_Win32.hpp"

#if defined(__GNUC__) && defined(__MINGW32__) && _WIN32_WINNT < 0x0502
/**
 * MinGW-w64 only defines ULONG overloads for the various atomic functions
 * if _WIN32_WINNT > 0x0502.
 */
static inline ULONG InterlockedIncrement(ULONG volatile *Addend)
{
	return (ULONG)(InterlockedIncrement(reinterpret_cast<LONG volatile*>(Addend)));
}
static inline ULONG InterlockedDecrement(ULONG volatile *Addend)
{
	return (ULONG)(InterlockedDecrement(reinterpret_cast<LONG volatile*>(Addend)));
}
#endif /* __GNUC__ && __MINGW32__ && _WIN32_WINNT < 0x0502 */

namespace LibRomData {

/**
 * Create an IStream wrapper for IRpFile.
 * The IRpFile is dup()'d.
 * @param file IRpFile.
 */
RP_IStream_Win32::RP_IStream_Win32(IRpFile *file)
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

RP_IStream_Win32::~RP_IStream_Win32()
{
	delete m_file;
}

/**
 * Get the IRpFile.
 * NOTE: The IRpFile is still owned by this object.
 * @return IRpFile.
 */
IRpFile *RP_IStream_Win32::file(void) const
{
	return m_file;
}

/**
 * Set the IRpFile.
 * @param file New IRpFile.
 */
void RP_IStream_Win32::setFile(IRpFile *file)
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

IFACEMETHODIMP RP_IStream_Win32::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	// Always set out parameter to NULL, validating it first.
	if (!ppvObj)
		return E_INVALIDARG;
	*ppvObj = NULL;

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
		return E_NOINTERFACE;
	}

	// Make sure we count this reference.
	AddRef();
	return NOERROR;
}

IFACEMETHODIMP_(ULONG) RP_IStream_Win32::AddRef(void)
{
	//InterlockedIncrement(&RP_ulTotalRefCount);	// FIXME
	InterlockedIncrement(&m_ulRefCount);
	return m_ulRefCount;
}

IFACEMETHODIMP_(ULONG) RP_IStream_Win32::Release(void)
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

IFACEMETHODIMP RP_IStream_Win32::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	if (!m_file) {
		return E_HANDLE;
	}

	size_t size = m_file->read(pv, cb);
	if (pcbRead) {
		*pcbRead = (ULONG)size;
	}

	return (size == (size_t)cb ? S_OK : S_FALSE);
}

IFACEMETHODIMP RP_IStream_Win32::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	if (!m_file) {
		return E_HANDLE;
	}

	size_t size = m_file->write(pv, cb);
	if (pcbWritten) {
		*pcbWritten = (ULONG)size;
	}

	return (size == (size_t)cb ? S_OK : S_FALSE);
}

/** IStream **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/aa380034(v=vs.85).aspx

IFACEMETHODIMP RP_IStream_Win32::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
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
			pos = m_file->fileSize();
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

IFACEMETHODIMP RP_IStream_Win32::SetSize(ULARGE_INTEGER libNewSize)
{
	((void)libNewSize);
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_IStream_Win32::CopyTo(IStream *pstm, ULARGE_INTEGER cb,
		ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
	// TODO: Implement this? Not sure if it's needed by GDI+.
	((void)pstm);
	((void)cb);
	((void)pcbRead);
	((void)pcbWritten);
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_IStream_Win32::Commit(DWORD grfCommitFlags)
{
	// NOTE: Returning S_OK, even though we're not doing anything here.
	((void)grfCommitFlags);
	return S_OK;
}

IFACEMETHODIMP RP_IStream_Win32::Revert(void)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_IStream_Win32::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	((void)libOffset);
	((void)cb);
	((void)dwLockType);
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_IStream_Win32::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	((void)libOffset);
	((void)cb);
	((void)dwLockType);
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_IStream_Win32::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
	// TODO: Initialize STATSTG on file open?
	if (!m_file) {
		return E_HANDLE;
	}

	if (grfStatFlag & STATFLAG_NONAME) {
		pstatstg->pwcsName = nullptr;
	} else {
		// FIXME: Store the filename in IRpFile?
		pstatstg->pwcsName = nullptr;
	}

	pstatstg->type = STGTY_STREAM;	// TODO: or STGTY_STORAGE?

	int64_t fileSize = m_file->fileSize();;
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

IFACEMETHODIMP RP_IStream_Win32::Clone(IStream **ppstm)
{
	((void)ppstm);
	return E_NOTIMPL;
}

}
