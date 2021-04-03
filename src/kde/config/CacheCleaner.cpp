/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * CacheCleaner.cpp: Cache cleaner object for CacheTab.                    *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CacheCleaner.hpp"

// libunixcommon
#include "libunixcommon/userdirs.hpp"

// librpfile
#include "librpfile/FileSystem.hpp"
using namespace LibRpFile;

// C++ STL classes
using std::string;

/**
 * Run the task.
 * This should be connected to QThread::started().
 */
void CacheCleaner::run(void)
{
	string dir;
	QString qs_err;
	switch (m_cacheDir) {
		default:
			assert(!"Invalid cache directory specified.");
			qs_err = tr("Invalid cache directory specified.");
			break;

		case CacheCleaner::CD_System:
			// System thumbnails. (~/.cache/thumbnails)
			dir = LibUnixCommon::getCacheDirectory();
			if (dir.empty()) {
				qs_err = tr("Unable to get the XDG cache directory.");
				break;
			}
			// Append "/thumbnails".
			dir += "/thumbnails";
			if (!LibUnixCommon::isWritableDirectory(dir.c_str())) {
				// Thumbnails subdirectory does not exist. (or is not writable)
				// TODO: Check specifically if it's not writable or doesn't exist?
				qs_err = tr("Thumbnails cache directory does not exist.");
				break;
			}
			break;

		case CacheCleaner::CD_RomProperties:
			// rom-properties cache. (~/.cache/rom-properties)
			dir = FileSystem::getCacheDirectory();
			if (dir.empty()) {
				qs_err = tr("Unable to get the rom-properties cache directory.");
				break;
			}
			break;
	}

	if (!qs_err.isEmpty()) {
		// An error occurred trying to get the directory.
		emit progress(1, 1);
		emit error(qs_err);
		emit finished();
		return;
	}

	// TODO: Process the directory.
	emit error(QLatin1String("not implemented!"));
	emit finished();
}
