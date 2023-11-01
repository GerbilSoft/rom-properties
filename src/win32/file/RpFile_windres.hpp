/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RpFile_windres.hpp: Windows resource wrapper for IRpFile. (Win32)       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpfile/MemFile.hpp"
#include "libwin32common/RpWin32_sdk.h"

class RpFile_windres final : public LibRpFile::MemFile
{
public:
	/**
	 * Open an IRpFile backed by a Win32 resource.
	 * The resulting IRpFile is read-only.
	 *
	 * @param hModule Module handle.
	 * @param lpName Resource name.
	 * @param lpType Resource type.
	 */
	RpFile_windres(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType);
public:
	~RpFile_windres() final;

private:
	typedef LibRpFile::MemFile super;
	RP_DISABLE_COPY(RpFile_windres)

public:
	/**
	 * Close the file.
	 */
	void close(void) final;

protected:
	HGLOBAL m_hGlobal;	// Resource handle.
};
