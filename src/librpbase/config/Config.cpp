/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * Config.cpp: Configuration manager.                                      *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "Config.hpp"
#include "ConfReader_p.hpp"

// C++ STL classes.
using std::string;
using std::unique_ptr;
using std::unordered_map;

#include "RomData.hpp"

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRpBase {

class ConfigPrivate : public ConfReaderPrivate
{
	public:
		ConfigPrivate();

	private:
		typedef ConfReaderPrivate super;
		RP_DISABLE_COPY(ConfigPrivate)

	public:
		// Static Config instance.
		// TODO: Q_GLOBAL_STATIC() equivalent, though we
		// may need special initialization if the compiler
		// doesn't support thread-safe statics.
		static Config instance;

	public:
		/**
		 * Reset the configuration to the default values.
		 */
		void reset(void) final;

		/**
		 * Process a configuration line.
		 * Virtual function; must be reimplemented by subclasses.
		 *
		 * @param section Section.
		 * @param name Key.
		 * @param value Value.
		 * @return 1 on success; 0 on error.
		 */
		int processConfigLine(const char *section,
			const char *name, const char *value) final;

	public:
		/**
		 * Default image type priority.
		 * Used if a custom configuration is not defined
		 * for a given system.
		 */
		static const uint8_t defImgTypePrio[];

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
		bool showDangerousPermissionsOverlayIcon;
		bool enableThumbnailOnNetworkFS;
};

/** ConfigPrivate **/

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
Config ConfigPrivate::instance;

/**
 * Default image type priority.
 * Used if a custom configuration is not defined
 * for a given system.
 *
 * TODO: Per-system defaults?
 */
const uint8_t ConfigPrivate::defImgTypePrio[] = {
	RomData::IMG_EXT_TITLE_SCREEN,	// WiiWare only
	RomData::IMG_EXT_MEDIA,
	RomData::IMG_EXT_COVER,
	RomData::IMG_EXT_BOX,
	RomData::IMG_INT_IMAGE,
	RomData::IMG_INT_MEDIA,
	RomData::IMG_INT_ICON,
	RomData::IMG_INT_BANNER,
};

ConfigPrivate::ConfigPrivate()
	: super("rom-properties.conf")
	/* Download options */
	, extImgDownloadEnabled(true)
	, useIntIconForSmallSizes(true)
	, downloadHighResScans(true)
	/* Overlay icon */
	, showDangerousPermissionsOverlayIcon(true)
	/* Enable thumbnailing and metadata on network FS */
	, enableThumbnailOnNetworkFS(false)
{
	// NOTE: Configuration is also initialized in the reset() function.
}

/**
 * Reset the configuration to the default values.
 */
void ConfigPrivate::reset(void)
{
	// Clear the image type priorities vector and map.
	vImgTypePrio.clear();
	mapImgTypePrio.clear();

	// Reserve 1 KB for the image type priorities store.
	vImgTypePrio.reserve(1024);
#ifdef HAVE_UNORDERED_MAP_RESERVE
	// Reserve 16 entries for the map.
	mapImgTypePrio.reserve(16);
#endif

	// Download options
	extImgDownloadEnabled = true;
	useIntIconForSmallSizes = true;
	downloadHighResScans = true;
	// Overlay icon
	showDangerousPermissionsOverlayIcon = true;
	// Enable thumbnail and metadata on network FS
	enableThumbnailOnNetworkFS = false;
}

/**
 * Process a configuration line.
 * Virtual function; must be reimplemented by subclasses.
 *
 * @param section Section.
 * @param name Key.
 * @param value Value.
 * @return 1 on success; 0 on error.
 */
int ConfigPrivate::processConfigLine(const char *section, const char *name, const char *value)
{
	// NOTE: Invalid lines are ignored, so we're always returning 1.

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
			param = &extImgDownloadEnabled;
		} else if (!strcasecmp(name, "UseIntIconForSmallSizes")) {
			param = &useIntIconForSmallSizes;
		} else if (!strcasecmp(name, "DownloadHighResScans")) {
			param = &downloadHighResScans;
		} else {
			// Invalid option.
			return 1;
		}

		// Parse the value.
		// Acceptable values are "true", "false", "1", and "0".
		if (!strcasecmp(value, "true") || !strcmp(value, "1")) {
			*param = true;
		} else if (!strcasecmp(value, "false") || !strcmp(value, "0")) {
			*param = false;
		} else {
			// TODO: Show a warning or something?
		}
	} else if (!strcasecmp(section, "Options")) {
		// Options.
		bool *param;
		if (!strcasecmp(name, "ShowDangerousPermissionsOverlayIcon")) {
			param = &showDangerousPermissionsOverlayIcon;
		} else if (!strcasecmp(name, "EnableThumbnailOnNetworkFS")) {
			param = &enableThumbnailOnNetworkFS;
		} else {
			// Invalid option.
			return 1;
		}

		// Parse the value.
		// Acceptable values are "true", "false", "1", and "0".
		if (!strcasecmp(value, "true") || !strcmp(value, "1")) {
			*param = true;
		} else if (!strcasecmp(value, "false") || !strcmp(value, "0")) {
			*param = false;
		} else {
			// TODO: Show a warning or something?
		}
	} else if (!strcasecmp(section, "ImageTypes")) {
		// NOTE: Duplicates will overwrite previous entries in the map,
		// though all of the data will remain in the vector.

		// inih automatically trims spaces from the
		// start and end of the string.

		// If the first character is '"', ignore it.
		// Needed because QSettings encloses strings in
		// double-quotes if they contain commas.
		// (Unquoted strings represent QStringList.)
		const char *pos = value;
		if (*pos == '"') {
			pos++;
		}

		// Parse the comma-separated values.
		const size_t vStartPos = vImgTypePrio.size();
		unsigned int count = 0;	// Number of image types.
		uint32_t imgbf = 0;	// Image type bitfield to prevent duplicates.
		while (*pos) {
			// Find the next comma.
			const char *comma = strchr(pos, ',');
			if (comma == pos) {
				// Empty field.
				pos++;
				continue;
			}

			// If no comma was found, read the remainder of the field.
			// Otherwise, read from pos to comma-1.
			size_t len = (comma ? static_cast<size_t>(comma-pos) : strlen(pos));

			// If the first entry is "no", then all thumbnails
			// for this system are disabled.
			if (count == 0 && len == 2 && !strncasecmp(pos, "no", 2)) {
				// Thumbnails are disabled.
				vImgTypePrio.push_back((uint8_t)RomData::IMG_DISABLED);
				count = 1;
				break;
			}

			// If the last character is '"', ignore it.
			// Needed because QSettings encloses strings in
			// double-quotes if they contain commas.
			// (Unquoted strings represent QStringList.)
			if (!comma && pos[len-1] == '"') {
				len--;
			}

			// Check for spaces at the start of the string.
			while (ISSPACE(*pos) && len > 0) {
				pos++;
				len--;
			}
			// Check for spaces at the end of the string.
			while (len > 0 && ISSPACE(pos[len-1])) {
				len--;
			}

			if (len == 0) {
				// Empty string.
				if (!comma)
					break;
				// Continue after the comma.
				pos = comma + 1;
				continue;
			}

			// Check the image type.
			// TODO: Hash comparison?
			// First byte of 'name' is a length value for optimization purposes.
			// NOTE: "\x08ExtMedia" is interpreted as a 0x8E byte by both
			// MSVC 2015 and gcc-4.5.2. In order to get it to work correctly,
			// we have to store the length byte separately from the actual
			// image type name.
			static const char *const imageTypeNames[] = {
				"\x07" "IntIcon",
				"\x09" "IntBanner",
				"\x08" "IntMedia",
				"\x08" "IntImage",
				"\x08" "ExtMedia",
				"\x08" "ExtCover",
				"\x0A" "ExtCover3D",
				"\x0C" "ExtCoverFull",
				"\x06" "ExtBox",
				"\x0E" "ExtTitleScreen",
			};
			static_assert(ARRAY_SIZE(imageTypeNames) == RomData::IMG_EXT_MAX+1, "imageTypeNames[] is the wrong size.");

			RomData::ImageType imgType = static_cast<RomData::ImageType>(-1);
			for (int i = 0; i < ARRAY_SIZE(imageTypeNames); i++) {
				if (static_cast<size_t>(imageTypeNames[i][0]) == len &&
				    !strncasecmp(pos, &imageTypeNames[i][1], len))
				{
					// Found a match!
					imgType = static_cast<RomData::ImageType>(i);
					break;
				}
			}

			if (imgType < 0) {
				// Not a match.
				if (!comma)
					break;
				// Continue after the comma.
				pos = comma + 1;
				continue;
			}

			// Check for duplicates.
			if (imgbf & (1 << imgType)) {
				// Duplicate image type!
				continue;
			}
			imgbf |= (1 << imgType);

			// Add the image type.
			// Maximum of 32 due to imgbf width.
			assert(count < 32);
			if (count >= 32) {
				// Too many image types...
				break;
			}
			vImgTypePrio.push_back(static_cast<uint8_t>(imgType));
			count++;

			if (!comma)
				break;

			// Continue after the comma.
			pos = comma + 1;
		}

		if (count > 0) {
			// Convert the class name to lowercase.
			string className(name);
			std::transform(className.begin(), className.end(), className.begin(), ::tolower);

			// Add the class name information to the map.
			uint32_t keyIdx = static_cast<uint32_t>(vStartPos);
			keyIdx |= (count << 24);
			mapImgTypePrio.insert(std::make_pair(className, keyIdx));
		}
	}

	// We're done here.
	return 1;
}

/** Config **/

Config::Config()
	: super(new ConfigPrivate())
{ }

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
	// Load the configuration if necessary.
	q->load(false);
	// Return the singleton instance.
	return &ConfigPrivate::instance;
}

/** Image types. **/

/**
 * Get the image type priority data for the specified class name.
 * NOTE: Call load() before using this function.
 * @param className	[in] Class name. (ASCII)
 * @param imgTypePrio	[out] Image type priority data.
 * @return ImgTypeResult
 */
Config::ImgTypeResult Config::getImgTypePrio(const char *className, ImgTypePrio_t *imgTypePrio) const
{
	assert(className != nullptr);
	assert(imgTypePrio != nullptr);
	if (!className || !imgTypePrio) {
		return IMGTR_ERR_INVALID_PARAMS;
	}

	// Find the class name in the map.
	RP_D(const Config);
	string className_lower(className);
	std::transform(className_lower.begin(), className_lower.end(), className_lower.begin(), ::tolower);
	auto iter = d->mapImgTypePrio.find(className_lower);
	if (iter == d->mapImgTypePrio.end()) {
		// Class name not found.
		// Use the global defaults.
		imgTypePrio->imgTypes = d->defImgTypePrio;
		imgTypePrio->length = ARRAY_SIZE(d->defImgTypePrio);
		return IMGTR_SUCCESS_DEFAULTS;
	}

	// Class name found.
	// Check its entry.
	const uint32_t keyIdx = iter->second;
	const uint32_t idx = (keyIdx & 0xFFFFFF);
	const uint8_t len = ((keyIdx >> 24) & 0xFF);
	assert(len > 0);
	assert(idx < d->vImgTypePrio.size());
	assert(idx + len <= d->vImgTypePrio.size());
	if (len == 0 || idx >= d->vImgTypePrio.size() || idx + len > d->vImgTypePrio.size()) {
		// Entry is invalid...
		// TODO: Force a configuration reload?
		return IMGTR_ERR_MAP_CORRUPTED;
	}

	// Is the first entry RomData::IMG_DISABLED?
	if (d->vImgTypePrio[idx] == static_cast<uint8_t>(RomData::IMG_DISABLED)) {
		// Thumbnails are disabled for this class.
		return IMGTR_DISABLED;
	}

	// Return the starting address and length.
	imgTypePrio->imgTypes = &d->vImgTypePrio[idx];
	imgTypePrio->length = len;
	return IMGTR_SUCCESS;
}

/**
 * Get the default image type priority data.
 * This is the priority data used if a custom configuration
 * is not defined for a given class.
 * @param imgTypePrio	[out] Image type priority data.
 */
void Config::getDefImgTypePrio(ImgTypePrio_t *imgTypePrio) const
{
	assert(imgTypePrio != nullptr);
	if (imgTypePrio) {
		RP_D(const Config);
		imgTypePrio->imgTypes = d->defImgTypePrio;
		imgTypePrio->length = ARRAY_SIZE(d->defImgTypePrio);
	}
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

/**
 * Show an overlay icon for "dangerous" permissions?
 * NOTE: Call load() before using this function.
 * @return True if we should show the overlay icon; false if not.
 */
bool Config::showDangerousPermissionsOverlayIcon(void) const
{
	RP_D(const Config);
	return d->showDangerousPermissionsOverlayIcon;
}

/**
 * Enable thumbnailing and metadata on network filesystems?
 * @return True if we should enable; false if not.
 */
bool Config::enableThumbnailOnNetworkFS(void) const
{
	RP_D(const Config);
	return d->enableThumbnailOnNetworkFS;
}

}
