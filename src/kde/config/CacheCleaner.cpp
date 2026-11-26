/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * CacheCleaner.cpp: Cache cleaner object for CacheTab.                    *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpfile.h"
#include "CacheCleaner.hpp"

#include "d_type.h"

// Other rom-properties libraries
#include "libi18n/i18n.hpp"
#include "librpfile/FileSystem.hpp"
#include "librpfile/RecursiveScan.hpp"
#include "libunixcommon/userdirs.hpp"
using namespace LibRpFile;

// C++ STL classes
using std::pair;
using std::string;

// libfmt
#include "rp-libfmt.h"

#include "RpQt.hpp"

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
	const char *s_cache_type = nullptr;
	const char *s_err = nullptr;
	switch (m_cacheDir) {
		default:
			assert(!"Invalid cache directory specified.");
			// NOT TRANSLATABLE: This error shouldn't happen!
			s_err = "Invalid cache directory specified.";
			break;

		case CacheDir::System:
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

			s_cache_type = C_("CacheCleaner", "System thumbnail cache");
			break;

		case CacheDir::RomProperties:
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

			s_cache_type = C_("CacheCleaner", "rom-properties cache");
			break;
	}

	if (s_err != nullptr) {
		// An error occurred trying to get the directory.
		emit progress(1, 1, true);
		emit error(U82Q(s_err));
		emit finished();
		return;
	}

	// Recursively scan the cache directory to get an initial maximum progress value.
	RecursiveScanState_t state;
	int ret = recursiveScan(state, cacheDir.c_str(), RecursiveScanMode::ScanOnly, nullptr, nullptr);
	if (ret != 0) {
		// Recursive scan failed.
		string stds_err;
		switch (ret) {
			case RSE_FOUND_UNEXPECTED_FILE_TYPE:
				fmt::format_to(std::back_inserter(stds_err),
					FRUN("{:s} has non-regular files. Not clearing it."), s_cache_type);
				break;
			case RSE_FOUND_SYMBOLIC_LINK:
				fmt::format_to(std::back_inserter(stds_err),
					FRUN("{:s} has a symbolic link. Not clearing it."), s_cache_type);
				break;
			case RSE_FOUND_UNEXPECTED_FILE_EXT:
				fmt::format_to(std::back_inserter(stds_err),
					FRUN("{:s} has unexpected files. Not clearing it."), s_cache_type);
				break;
			default:
				if (ret < 0) {
					fmt::format_to(std::back_inserter(stds_err), FRUN("Initial scan failed: {:s}"), strerror(-ret));
				} else {
					fmt::format_to(std::back_inserter(stds_err), FRUN("Initial scan failed: error code {:d}"), ret);
				}
				break;
		}
		emit progress(1, 1, true);
		emit error(U82Q(stds_err));
		emit finished();
		return;
	} else if (state.isEmpty()) {
		// Cache directory is empty.
		emit progress(1, 1, false);
		emit cacheIsEmpty(m_cacheDir);
		emit finished();
		return;
	}

	struct CallbackData_t {
		size_t cache_entries_total;
		CacheCleaner *obj;
	};
	CallbackData_t callbackData;

	// Get the number of files and directories.
	// NOTE: This is an estimate; it might change due to TOCTOU.
	callbackData.cache_entries_total = state.totalCount();
	callbackData.obj = this;

	// Delete all of the files and subdirectories.
	emit progress(0, static_cast<int>(callbackData.cache_entries_total), false);
	state.reset();

	ret = recursiveScan(state, cacheDir.c_str(), RecursiveScanMode::ScanAndDelete, &callbackData,
		[](const RecursiveScanState_t &state, void *user_data) -> void {
			// TODO: Restrict update frequency to X number of files/directories?
			const CallbackData_t *const callbackData = static_cast<const CallbackData_t*>(user_data);
			emit callbackData->obj->progress(state.totalCount(),
				static_cast<int>(callbackData->cache_entries_total),
				state.hasErrors());
		});
	if (ret != 0) {
		// Recursive scan failed.
		string stds_err;
		switch (ret) {
			case RSE_FOUND_UNEXPECTED_FILE_TYPE:
				fmt::format_to(std::back_inserter(stds_err),
					FRUN("{:s} has non-regular files. Cache clearing aborted."), s_cache_type);
				break;
			case RSE_FOUND_SYMBOLIC_LINK:
				fmt::format_to(std::back_inserter(stds_err),
					FRUN("{:s} has a symbolic link. Cache clearing aborted."), s_cache_type);
				break;
			case RSE_FOUND_UNEXPECTED_FILE_EXT:
				fmt::format_to(std::back_inserter(stds_err),
					FRUN("{:s} has unexpected files. Cache clearing aborted."), s_cache_type);
				break;
			default:
				if (ret < 0) {
					fmt::format_to(std::back_inserter(stds_err), FRUN("Secondary scan failed: {:s}"), strerror(-ret));
				} else {
					fmt::format_to(std::back_inserter(stds_err), FRUN("Secondary scan failed: error code {:d}"), ret);
				}
				break;
		}
		emit progress(1, 1, true);
		emit error(U82Q(stds_err));
		emit finished();
	} else {
		// Directory processed.
		emit cacheCleared(m_cacheDir, state.dirErrs, state.fileErrs);
		emit finished();
	}
}
