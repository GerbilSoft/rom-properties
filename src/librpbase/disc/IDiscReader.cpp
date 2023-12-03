/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IDiscReader.cpp: Disc reader interface.                                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "IDiscReader.hpp"

// Other rom-properties libraries
using namespace LibRpFile;

namespace LibRpBase {

IDiscReader::IDiscReader(const IRpFilePtr &file)
	: m_file(file)
{
	// Set properties
	if (!file) {
		m_lastError = EBADF;
		return;
	}

	m_fileType = file->fileType();
}

}
