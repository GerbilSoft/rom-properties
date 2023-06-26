/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * CacheCleaner.cpp: Cache cleaner object for CacheTab.                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpfile.h"
#include "CacheCleaner.hpp"

// libunixcommon
#include "libunixcommon/userdirs.hpp"

// librpfile
#include "librpfile/FileSystem.hpp"
#include "librpfile/RecursiveScan.hpp"
using namespace LibRpFile;

// C++ STL classes
using std::forward_list;
using std::pair;
using std::string;

/** CacheCleaner **/

CacheCleaner::CacheCleaner(QObject *parent, CacheCleaner::CacheDir cacheDir)
	: super(parent)
	, m_cacheDir(cacheDir)
{
	qRegisterMetaType<CacheDir>();
}

/**
 * Run the task.
 * This should be connected to QThread::started().
 */
void CacheCleaner::run(void)
{
	string cacheDir;
	const char *s_err = nullptr;
	switch (m_cacheDir) {
		default:
			assert(!"Invalid cache directory specified.");
			s_err = C_("CacheCleaner", "Invalid cache directory specified.");
			break;

		case CacheCleaner::CD_System:
			// System thumbnails. (~/.cache/thumbnails)
			cacheDir = LibUnixCommon::getCacheDirectory();
			if (cacheDir.empty()) {
				s_err = C_("CacheCleaner", "Unable to get the XDG cache directory.");
				break;
			}
			// Append "/thumbnails".
			cacheDir += "/thumbnails";
			if (!LibUnixCommon::isWritableDirectory(cacheDir.c_str())) {
				// Thumbnails subdirectory does not exist. (or is not writable)
				// TODO: Check specifically if it's not writable or doesn't exist?
				s_err = C_("CacheCleaner", "Thumbnails cache directory does not exist.");
				break;
			}
			break;

		case CacheCleaner::CD_RomProperties:
			// rom-properties cache. (~/.cache/rom-properties)
			cacheDir = FileSystem::getCacheDirectory();
			if (cacheDir.empty()) {
				s_err = C_("CacheCleaner", "Unable to get the rom-properties cache directory.");
				break;
			}

			// Does the cache directory exist?
			// If it doesn't, we'll act like it's empty.
			if (FileSystem::access(cacheDir.c_str(), R_OK) != 0) {
				emit progress(1, 1, false);
				emit cacheIsEmpty(m_cacheDir);
				emit finished();
				return;
			}

			break;
	}

	if (s_err != nullptr) {
		// An error occurred trying to get the directory.
		emit progress(1, 1, true);
		emit error(U82Q(s_err));
		emit finished();
		return;
	}

	// Recursively scan the cache directory.
	// TODO: Do we really want to store everything in a list? (Wastes memory.)
	// Maybe do a simple counting scan first, then delete.
	forward_list<pair<string, uint32_t> > rlist;
	int ret = recursiveScan(cacheDir.c_str(), rlist);
	if (ret != 0) {
		// Non-image file found.
		QString qs_err;
		switch (m_cacheDir) {
			default:
				assert(!"Invalid cache directory specified.");
				s_err = C_("CacheCleaner", "Invalid cache directory specified.");
				break;
			case CacheCleaner::CD_System:
				s_err = C_("CacheCleaner", "System thumbnail cache has unexpected files. Not clearing it.");
				break;
			case CacheCleaner::CD_RomProperties:
				s_err = C_("CacheCleaner", "rom-properties cache has unexpected files. Not clearing it.");
				break;
		}
		emit progress(1, 1, true);
		emit error(U82Q(s_err));
		emit finished();
		return;
	} else if (rlist.empty()) {
		// Cache directory is empty.
		emit progress(1, 1, false);
		emit cacheIsEmpty(m_cacheDir);
		emit finished();
		return;
	}

	// NOTE: std::forward_list doesn't have size().
	const size_t rlist_size = std::distance(rlist.cbegin(), rlist.cend());

	// Delete all of the files and subdirectories.
	emit progress(0, static_cast<int>(rlist_size), false);
	unsigned int count = 0;
	unsigned int dirErrs = 0, fileErrs = 0;
	bool hasErrors = false;
	for (const auto &p : rlist) {
		if (p.second == DT_DIR) {
			// Remove the directory.
			int ret = rmdir(p.first.c_str());
			if (ret != 0) {
				dirErrs++;
				hasErrors = true;
			}
		} else {
			// Delete the file.
			// TODO: Does the parent directory mode need to be changed to writable?
			int ret = unlink(p.first.c_str());
			if (ret != 0) {
				fileErrs++;
				hasErrors = true;
			}
		}

		// TODO: Restrict update frequency to X number of files/directories?
		count++;
		emit progress(count, static_cast<int>(rlist_size), hasErrors);
	}

	// Directory processed.
	emit cacheCleared(m_cacheDir, dirErrs, fileErrs);
	emit finished();
}
