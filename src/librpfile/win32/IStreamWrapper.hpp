/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * IStreamWrapper.hpp: IStream wrapper for IRpFile. (Win32)                *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifndef _WIN32
#  error IStreamWrapper.hpp is Windows only.
#endif

#include "../IRpFile.hpp"
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/ComBase.hpp"
#include <objidl.h>

namespace LibRpFile {

class IStreamWrapper final : public LibWin32Common::ComBase<IStream>
{
public:
	/**
	 * Create an IStream wrapper for IRpFile.
	 * @param file IRpFile
	 */
	explicit IStreamWrapper(LibRpFile::IRpFilePtr file)
		: m_file(file)
	{}

private:
	typedef LibWin32Common::ComBase<IStream> super;
public:
	RP_DISABLE_COPY(IStreamWrapper)

public:
	/**
	 * Get the IRpFile.
	 * @return IRpFile
	 */
	inline IRpFile *file(void) const
	{
		return m_file.get();
	}

	/**
	 * Set the IRpFile.
	 * @param file New IRpFile (must *not* be deleted while in use)
	 */
	inline void setFile(const LibRpFile::IRpFilePtr &file)
	{
		m_file = file;
	}

	/**
	 * Set the IRpFile.
	 * @param file New IRpFile (must *not* be deleted while in use)
	 */
	inline void setFile(LibRpFile::IRpFilePtr &&file)
	{
		m_file = file;
	}

public:
	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) noexcept final;

	// ISequentialStream
	IFACEMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead) noexcept final;
	IFACEMETHODIMP Write(const void *pv, ULONG cb, ULONG *pcbWritten) noexcept final;

	// IStream
	IFACEMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition) noexcept final;
	IFACEMETHODIMP SetSize(ULARGE_INTEGER libNewSize) noexcept final;
	IFACEMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten) noexcept final;
	IFACEMETHODIMP Commit(DWORD grfCommitFlags) noexcept final;
	IFACEMETHODIMP Revert(void) noexcept final;
	IFACEMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) noexcept final;
	IFACEMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) noexcept final;
	IFACEMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag) noexcept final;
	IFACEMETHODIMP Clone(IStream **ppstm) noexcept final;

protected:
	IRpFilePtr m_file;
};

}
