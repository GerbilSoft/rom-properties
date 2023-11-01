/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * SetFileOriginInfo.hpp: setFileOriginInfo() function.                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes. (C++ namespace)
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <ctime>

// tcharx
#include "tcharx.h"

namespace RpDownload {

#ifndef _WIN32
/**
 * Set the file origin info.
 * This uses xattrs on Linux and ADS on Windows.
 * @param file Open file. (Must be writable.)
 * @param url Origin URL.
 * @param mtime If >= 0, this value is set as the mtime.
 * @return 0 on success; negative POSIX error code on error.
 */
int setFileOriginInfo(FILE *file, const TCHAR *url, time_t mtime);
#endif /* !_WIN32 */

#ifdef _WIN32
/**
 * Set the file origin info.
 * This uses xattrs on Linux and ADS on Windows.
 * @param file Open file. (Must be writable.)
 * @param filename Filename. [FIXME: Make it so we don't need this on Windows.]
 * @param url Origin URL.
 * @param mtime If >= 0, this value is set as the mtime.
 * @return 0 on success; negative POSIX error code on error.
 */
int setFileOriginInfo(FILE *file, const TCHAR *filename, const TCHAR *url, time_t mtime);
#endif /* _WIN32 */

} //namespace RpDownload
