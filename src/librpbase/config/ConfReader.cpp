/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ConfReader.hpp: Configuration reader base class.                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ConfReader.hpp"
#include "ConfReader_p.hpp"

// Other rom-properties libraries
#include "librpfile/FileSystem.hpp"
using namespace LibRpFile;

// OS-specific includes
#ifdef _WIN32
// for U82T_s()
#  include "librptext/wchar.hpp"
#endif

namespace LibRpBase {

/** ConfReaderPrivate **/

ConfReaderPrivate::ConfReaderPrivate(const char *filename)
	: conf_rel_filename(filename)
	, conf_mtime(0)
	, conf_last_checked(0)
	, conf_was_found(false)
{ }

/**
 * Process a configuration line.
 * Static function; used by inih as a C-style callback function.
 *
 * @param user ConfReaderPrivate object.
 * @param section Section.
 * @param name Key.
 * @param value Value.
 * @return 1 on success; 0 on error.
 */
int ConfReaderPrivate::processConfigLine_static(void *user, const char *section, const char *name, const char *value)
{
	return static_cast<ConfReaderPrivate*>(user)->processConfigLine(section, name, value);
}

/** ConfReader **/

/**
 * Configuration reader.
 * @param d ConfReaderPrivate subclass.
 */
ConfReader::ConfReader(ConfReaderPrivate *d)
	: d_ptr(d)
{ }

ConfReader::~ConfReader()
{
	delete d_ptr;
}

/**
 * Has the configuration been loaded yet?
 *
 * This function will *not* load the configuration.
 * To load the configuration, call load().
 *
 * If this function returns false after calling get(),
 * rom-properties.conf is probably missing.
 *
 * @return True if the configuration have been loaded; false if not.
 */
bool ConfReader::isLoaded(void) const
{
	RP_D(const ConfReader);
	return d->conf_was_found;
}

/**
 * Load the configuration.
 *
 * If the configuration has been modified since the last
 * load, it will be reloaded. Otherwise, this function
 * won't do anything.
 *
 * @param force If true, force a reload, even if the file hasn't been modified.
 * @return 0 on success; negative POSIX error code on error.
 */
int ConfReader::load(bool force)
{
	RP_D(ConfReader);

	if (!force && d->conf_was_found) {
		// Have we checked the timestamp recently?
		// TODO: Define the threshold somewhere.
		const time_t now = time(nullptr);
		if (llabs(now - d->conf_last_checked) < 2) {
			// We checked it recently. Assume it's up to date.
			return 0;
		}
		d->conf_last_checked = now;

		// Check if the keys.conf timestamp has changed.
		// Initial check. (fast path)
		time_t mtime;
		int ret = FileSystem::get_mtime(d->conf_filename, &mtime);
		if (ret != 0) {
			// Failed to retrieve the mtime.
			// Leave everything as-is.
			// TODO: Proper error code?
			return -EIO;
		}

		if (mtime == d->conf_mtime) {
			// Timestamp has not changed.
			return 0;
		}
	}

	// loadKeys() mutex
	// NOTE: This may result in the configuration being loaded
	// twice in some cases, but that's better than the configuration
	// being loaded twice at the same time and causing collisions.
	std::lock_guard<std::mutex> mtxLocker(d->mtxLoad);

	if (d->conf_filename.empty()) {
		// Get the configuration filename.
		d->conf_filename = FileSystem::getConfigDirectory();
		if (!d->conf_filename.empty()) {
			if (d->conf_filename.at(d->conf_filename.size()-1) != DIR_SEP_CHR) {
				d->conf_filename += DIR_SEP_CHR;
			}
			d->conf_filename += d->conf_rel_filename;
		}
	} else if (!force && d->conf_was_found) {
		// Check if the keys.conf timestamp has changed.
		// NOTE: Second check once the mutex is locked.
		time_t mtime;
		int ret = FileSystem::get_mtime(d->conf_filename, &mtime);
		if (ret != 0) {
			// Failed to retrieve the mtime.
			// Leave everything as-is.
			// TODO: Proper error code?
			return -EIO;
		}

		if (mtime == d->conf_mtime) {
			// Timestamp has not changed.
			return 0;
		}
	}

	// Reset the configuration to the default values.
	d->reset();

	// Parse the configuration file.
	// NOTE: We're using the filename directly, since it's always
	// on the local file system, and it's easier to let inih
	// manage the file itself.
#ifdef _WIN32
	// Win32: Open the file using _tfopen(),
	// then parse it using ini_parse_file().
	int ret = 0;
	errno = 0;
	FILE *f_ini = _tfopen(U82T_s(d->conf_filename), _T("rb"));
	if (f_ini) {
		// Parse the INI file.
		ret = ini_parse_file(f_ini, ConfReaderPrivate::processConfigLine_static, d);
		fclose(f_ini);
	} else {
		// Error opening the INI file.
		ret = errno;
	}
#else /* !_WIN32 */
	// Linux or other systems: Use ini_parse().
	int ret = ini_parse(d->conf_filename.c_str(),
		ConfReaderPrivate::processConfigLine_static, d);
#endif /* _WIN32 */
	if (ret != 0) {
		// Error parsing the INI file.
		d->reset();
		if (ret == -2)
			return -ENOMEM;
		return -EIO;
	}

	// Save the mtime from the keys.conf file.
	// TODO: Combine with earlier check?
	time_t mtime;
	ret = FileSystem::get_mtime(d->conf_filename, &mtime);
	if (ret == 0) {
		d->conf_mtime = mtime;
	} else {
		// mtime error...
		// TODO: What do we do here?
		d->conf_mtime = 0;
	}

	// Keys loaded.
	d->conf_was_found = true;
	return 0;
}

/**
 * Get the configuration filename.
 *
 * If the configuration's directory does not exist, this
 * will return nullptr. Otherwise, the filename will be
 * returned, even if the file doesn't exist yet.
 *
 * @return Configuration filename, or nullptr on error.
 */
const char *ConfReader::filename(void) const
{
	RP_D(const ConfReader);
	if (d->conf_filename.empty()) {
		// No filename yet. Try to load the file.
		int ret = const_cast<ConfReader*>(this)->load(false);
		if (ret != 0 || d->conf_filename.empty()) {
			// Still unable to get the filename.
			return nullptr;
		}
	}

	return d->conf_filename.c_str();
}

}
