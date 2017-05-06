/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Config.cpp: Configuration manager.                                      *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include "Config.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>

// C++ includes.
#include <memory>
#include <string>
#include <unordered_map>
using std::string;
using std::unique_ptr;
using std::unordered_map;

#include "file/FileSystem.hpp"
#include "file/RpFile.hpp"
#include "threads/Atomics.h"
#include "threads/Mutex.hpp"

// One-time initialization.
#ifdef _WIN32
#include "threads/InitOnceExecuteOnceXP.h"
#else
#include <pthread.h>
#endif

// Text conversion functions and macros.
#include "TextFuncs.hpp"
#ifdef _WIN32
#include "RpWin32.hpp"
#endif

// INI parser.
#include "ini.h"

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData {

class ConfigPrivate
{
	public:
		ConfigPrivate();

	private:
		RP_DISABLE_COPY(ConfigPrivate)

	public:
		// Static Config instance.
		// TODO: Q_GLOBAL_STATIC() equivalent, though we
		// may need special initialization if the compiler
		// doesn't support thread-safe statics.
		static Config instance;

	public:
		/**
		 * Initialize Config.
		 */
		void init(void);

		/**
		 * Initialize Once function.
		 * Called by pthread_once() or InitOnceExecuteOnce().
		 */
#ifdef _WIN32
		static BOOL WINAPI initOnceFunc(_Inout_ PINIT_ONCE_XP once, _In_ PVOID param, _Out_opt_ LPVOID *context);
#else
		static void initOnceFunc(void);
#endif

		/**
		 * Reset the configuration to the default values.
		 */
		void reset(void);

		/**
		 * Process a configuration line.
		 *
		 * NOTE: This function is static because it is used as a
		 * C-style callback from inih.
		 *
		 * @param user ConfigPrivate object.
		 * @param section Section.
		 * @param name Key.
		 * @param value Value.
		 * @return 1 on success; 0 on error.
		 */
		static int processConfigLine(void *user, const char *section, const char *name, const char *value);

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
		int load(bool force = false);

#ifdef _WIN32
		// InitOnceExecuteOnce() control variable.
		INIT_ONCE once_control;
		static const INIT_ONCE ONCE_CONTROL_INIT = INIT_ONCE_STATIC_INIT;
#else
		// pthread_once() control variable.
		pthread_once_t once_control;
		static const pthread_once_t ONCE_CONTROL_INIT = PTHREAD_ONCE_INIT;
#endif

		// load() mutex.
		Mutex mtxLoad;

		// rom-properties.conf status.
		rp_string conf_filename;
		bool conf_was_found;
		time_t conf_mtime;

	public:
		// Image type priority data.
		// Managed as a single block in order to reduce
		// memory allocations.
		ao::uvector<uint8_t> vImgTypePrio;

		/**
		 * Map of RomData subclass names to vImgTypePrio indexes.
		 * - Key: RomData subclass name.
		 * - Value: vImgTypePrio information.
		 *   - High byte: Data length.
		 *   - Low 3 bytes: Data offset.
		 */
		unordered_map<string, uint32_t> mapImgTypePrio;

		// Download options.
		bool extImgDownloadEnabled;
		bool useIntIconForSmallSizes;
		bool downloadHighResScans;
};

/** ConfigPrivate **/

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
Config ConfigPrivate::instance;

ConfigPrivate::ConfigPrivate()
	: once_control(ONCE_CONTROL_INIT)
	, conf_was_found(false)
	, conf_mtime(0)
	/* Download options */
	, extImgDownloadEnabled(true)
	, useIntIconForSmallSizes(true)
	, downloadHighResScans(true)
{
	// NOTE: Configuration is also initialized in the reset() function.
}

/**
 * Initialize Config.
 * Called by initOnceFunc().
 */
void ConfigPrivate::init(void)
{
	// Reserve 1 KB for the image type priorities store.
	vImgTypePrio.reserve(1024);

	// Configuration filename.
	conf_filename = FileSystem::getConfigDirectory();
	if (!conf_filename.empty()) {
		if (conf_filename.at(conf_filename.size()-1) != _RP_CHR(DIR_SEP_CHR)) {
			conf_filename += _RP_CHR(DIR_SEP_CHR);
		}
		conf_filename += _RP("rom-properties.conf");
	}

	// Make sure the configuration directory exists.
	// NOTE: The filename portion MUST be kept in config_path,
	// since the last component is ignored by rmkdir().
	int ret = FileSystem::rmkdir(conf_filename);
	if (ret != 0) {
		// rmkdir() failed.
		conf_filename.clear();
	}

	// Load the configuration.
	load(true);
}

/**
 * Initialize Once function.
 * Called by pthread_once() or InitOnceExecuteOnce().
 * TODO: Add a pthread_once() wrapper for Windows?
 */
#ifdef _WIN32
BOOL WINAPI ConfigPrivate::initOnceFunc(_Inout_ PINIT_ONCE_XP once, _In_ PVOID param, _Out_opt_ LPVOID *context)
#else
void ConfigPrivate::initOnceFunc(void)
#endif
{
	instance.d_ptr->init();
#ifdef _WIN32
	return TRUE;
#endif
}

/**
 * Reset the configuration to the default values.
 */
void ConfigPrivate::reset(void)
{
	// Image type priorities.
	vImgTypePrio.clear();
	mapImgTypePrio.clear();

	// Download options.
	extImgDownloadEnabled = true;
	useIntIconForSmallSizes = true;
	downloadHighResScans = true;
}

/**
 * Process a configuration line.
 *
 * NOTE: This function is static because it is used as a
 * C-style callback from inih.
 *
 * @param user ConfigPrivate object.
 * @param section Section.
 * @param name Key.
 * @param value Value.
 * @return 1 on success; 0 on error.
 */
int ConfigPrivate::processConfigLine(void *user, const char *section, const char *name, const char *value)
{
	ConfigPrivate *const d = static_cast<ConfigPrivate*>(user);

	// NOTE: Invalid lines are ignored, so we're always returning 1.

	// TODO: Load image type priorities.
	// For now, only loading the "Download" options.

	// Verify that the parameters are valid.
	if (!section || section[0] == 0 ||
	    !name || name[0] == 0 ||
	    !value || value[0] == 0)
	{
		// One or more components are either nullptr or empty strings.
		return 1;
	}

	// Which section are we in?
	if (!strcasecmp(section, "Downloads")) {
		// Downloads. Check for one of the three boolean options.
		bool *param;
		if (!strcasecmp(name, "ExtImageDownload")) {
			param = &d->extImgDownloadEnabled;
		} else if (!strcasecmp(name, "UseIntIconForSmallSizes")) {
			param = &d->useIntIconForSmallSizes;
		} else if (!strcasecmp(name, "DownloadHighResScans")) {
			param = &d->downloadHighResScans;
		} else {
			// Invalid option.
			return 1;
		}

		// Parse the value.
		// Acceptable values are "true", "false", "1", and "0".
		if (!strcasecmp(value, "true") || !strcasecmp(value, "1")) {
			*param = true;
		} else if (!strcasecmp(value, "false") || !strcasecmp(value, "0")) {
			*param = false;
		} else {
			// TODO: Show a warning or something?
		}
	}

	// We're done here.
	return 1;
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
int ConfigPrivate::load(bool force)
{
	if (conf_filename.empty()) {
		// Configuration filename is invalid...
		return -ENOENT;
	}

	if (!force && conf_was_found) {
		// Check if the keys.conf timestamp has changed.
		// Initial check. (fast path)
		time_t mtime;
		int ret = FileSystem::get_mtime(conf_filename, &mtime);
		if (ret != 0) {
			// Failed to retrieve the mtime.
			// Leave everything as-is.
			// TODO: Proper error code?
			return -EIO;
		}

		if (mtime == conf_mtime) {
			// Timestamp has not changed.
			return 0;
		}
	}

	// loadKeys() mutex.
	// NOTE: This may result in keys.conf being loaded twice
	// in some cases, but that's better than keys.conf being
	// loaded twice at the same time and causing collisions.
	MutexLocker mtxLocker(mtxLoad);

	if (!force && conf_was_found) {
		// Check if the keys.conf timestamp has changed.
		// NOTE: Second check once the mutex is locked.
		time_t mtime;
		int ret = FileSystem::get_mtime(conf_filename, &mtime);
		if (ret != 0) {
			// Failed to retrieve the mtime.
			// Leave everything as-is.
			// TODO: Proper error code?
			return -EIO;
		}

		if (mtime == conf_mtime) {
			// Timestamp has not changed.
			return 0;
		}
	}

	// Reset the configuration to the default values.
	reset();

	// Parse the configuration file.
	// NOTE: We're using the filename directly, since it's always
	// on the local file system, and it's easier to let inih
	// manage the file itself.
#ifdef _WIN32
	// Win32: Use ini_parse_w().
	int ret = ini_parse_w(RP2W_s(conf_filename), ConfigPrivate::processConfigLine, this);
#else /* !_WIN32 */
	// Linux or other systems: Use ini_parse().
	int ret = ini_parse(rp_string_to_utf8(conf_filename).c_str(), ConfigPrivate::processConfigLine, this);
#endif /* _WIN32 */
	if (ret != 0) {
		// Error parsing the INI file.
		reset();
		if (ret == -2)
			return -ENOMEM;
		return -EIO;
	}

	// Save the mtime from the keys.conf file.
	// TODO: IRpFile::get_mtime()?
	time_t mtime;
	ret = FileSystem::get_mtime(conf_filename, &mtime);
	if (ret == 0) {
		conf_mtime = mtime;
	} else {
		// mtime error...
		// TODO: What do we do here?
		conf_mtime = 0;
	}

	// Keys loaded.
	conf_was_found = true;
	return 0;
}

/** Config **/

Config::Config()
	: d_ptr(new ConfigPrivate())
{ }

Config::~Config()
{
	delete d_ptr;
}

/**
 * Get the Config instance.
 *
 * This automatically initializes the object and
 * reloads the configuration if it has been modified.
 *
 * @return Config instance.
 */
Config *Config::instance(void)
{
	// Initialize the singleton instance.
	Config *const q = &ConfigPrivate::instance;

#ifdef _WIN32
	// TODO: Handle errors.
	InitOnceExecuteOnce(&q->d_ptr->once_control,
		q->d_ptr->initOnceFunc,
		(PVOID)q, nullptr);
#else
	pthread_once(&q->d_ptr->once_control, q->d_ptr->initOnceFunc);
#endif

	// Reload the configuration if necessary.
	q->load(false);

	// Singleton instance.
	return &ConfigPrivate::instance;
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
bool Config::isLoaded(void) const
{
	RP_D(const Config);
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
int Config::load(bool force)
{
	RP_D(Config);
	return d->load(force);
}

/** Download options. **/

/**
 * Should we download images from external databases?
 * NOTE: Call load() before using this function.
 * @return True if downloads are enabled; false if not.
 */
bool Config::extImgDownloadEnabled(void) const
{
	RP_D(const Config);
	return d->extImgDownloadEnabled;
}

/**
 * Always use the internal icon (if present) for small sizes.
 * TODO: Clarify "small sizes".
 * NOTE: Call load() before using this function.
 * @return True if we should use the internal icon for small sizes; false if not.
 */
bool Config::useIntIconForSmallSizes(void) const
{
	RP_D(const Config);
	return d->useIntIconForSmallSizes;
}

/**
 * Download high-resolution scans if viewing large thumbnails.
 * NOTE: Call load() before using this function.
 * @return True if we should download high-resolution scans; false if not.
 */
bool Config::downloadHighResScans(void) const
{
	RP_D(const Config);
	return d->downloadHighResScans;
}

}
