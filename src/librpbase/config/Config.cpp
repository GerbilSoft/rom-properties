/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * Config.cpp: Configuration manager.                                      *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "Config.hpp"
#include "ConfReader_p.hpp"
#include "ctypex.h"

// C++ STL classes
using std::array;
using std::string;
using std::unordered_map;

#include "RomData.hpp"

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
	 *
	 * TODO: Per-system defaults?
	 */
	static const array<uint8_t, 8> defImgTypePrio;

	// Image type priority data.
	// Managed as a single block in order to reduce
	// memory allocations.
	rp::uvector<uint8_t> vImgTypePrio;

	/**
	 * Map of RomData subclass names to vImgTypePrio indexes.
	 * - Key: RomData subclass name.
	 * - Value: vImgTypePrio information.
	 *   - High byte: Data length.
	 *   - Low 3 bytes: Data offset.
	 */
	unordered_map<string, uint32_t> mapImgTypePrio;

	// PAL language codes for GameTDB (NULL-terminated array)
	// NOTE: 'au' is technically not a language code, but
	// GameTDB handles it as a separate language.
	static const array<uint32_t, 9+1> pal_lc;

	// Download options
	uint32_t palLanguageForGameTDB;
	bool extImgDownloadEnabled;
	bool useIntIconForSmallSizes;
	bool storeFileOriginInfo;

	// Image bandwidth options
	Config::ImgBandwidth imgBandwidthUnmetered;
	Config::ImgBandwidth imgBandwidthMetered;
	// Compatibility with older settings
	bool isNewBandwidthOptionSet;
	bool downloadHighResScans;

	// DMG title screen mode [index is ROM type]
	array<Config::DMG_TitleScreen_Mode, static_cast<size_t>(Config::DMG_TitleScreen_Mode::Max)> dmgTSMode;

	// Other options
	bool showDangerousPermissionsOverlayIcon;
	bool enableThumbnailOnNetworkFS;
	bool showXAttrView;
	bool thumbnailDirectoryPackages;

public:
	/** Default values **/

	// Download options
	static constexpr uint32_t palLanguageForGameTDB_default = 'en';
	static constexpr bool extImgDownloadEnabled_default = true;
	static constexpr bool useIntIconForSmallSizes_default = true;
	static constexpr bool storeFileOriginInfo_default = true;

	// Image bandwidth options
	static constexpr Config::ImgBandwidth imgBandwidthUnmetered_default = Config::ImgBandwidth::HighRes;
	static constexpr Config::ImgBandwidth imgBandwidthMetered_default = Config::ImgBandwidth::NormalRes;

	// DMG title screen mode [index is ROM type]
	static const array<Config::DMG_TitleScreen_Mode, static_cast<size_t>(Config::DMG_TitleScreen_Mode::Max)> dmgTSMode_default;

	// Other options
	static constexpr bool showDangerousPermissionsOverlayIcon_default = true;
	static constexpr bool enableThumbnailOnNetworkFS_default = false;
	static constexpr bool showXAttrView_default = true;
	static constexpr bool thumbnailDirectoryPackages_default = true;
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
const array<uint8_t, 8> ConfigPrivate::defImgTypePrio = {{
	RomData::IMG_EXT_TITLE_SCREEN,	// WiiWare only
	RomData::IMG_EXT_MEDIA,
	RomData::IMG_EXT_COVER,
	RomData::IMG_EXT_BOX,
	RomData::IMG_INT_IMAGE,
	RomData::IMG_INT_MEDIA,
	RomData::IMG_INT_ICON,
	RomData::IMG_INT_BANNER,
}};

// PAL language codes for GameTDB (NULL-terminated array)
// NOTE: 'au' is technically not a language code, but
// GameTDB handles it as a separate language.
const array<uint32_t, 9+1> ConfigPrivate::pal_lc = {{'au', 'de', 'en', 'es', 'fr', 'it', 'nl', 'pt', 'ru', 0}};

// DMG title screen mode [index is ROM type]
const array<Config::DMG_TitleScreen_Mode, static_cast<size_t>(Config::DMG_TitleScreen_Mode::Max)> ConfigPrivate::dmgTSMode_default = {{
	Config::DMG_TitleScreen_Mode::DMG,
	Config::DMG_TitleScreen_Mode::SGB,
	Config::DMG_TitleScreen_Mode::CGB
}};

ConfigPrivate::ConfigPrivate()
	: super("rom-properties.conf")
	// Download options
	, palLanguageForGameTDB(palLanguageForGameTDB_default)
	, extImgDownloadEnabled(extImgDownloadEnabled_default)
	, useIntIconForSmallSizes(useIntIconForSmallSizes_default)
	, storeFileOriginInfo(storeFileOriginInfo_default)
	// Image bandwidth options
	, imgBandwidthUnmetered(imgBandwidthUnmetered_default)
	, imgBandwidthMetered(imgBandwidthMetered_default)
	// Compatibility with older settings
	, isNewBandwidthOptionSet(false)
	, downloadHighResScans(true)
	// Overlay icon
	, showDangerousPermissionsOverlayIcon(showDangerousPermissionsOverlayIcon_default)
	// Enable thumbnailing and metadata on network FS
	, enableThumbnailOnNetworkFS(enableThumbnailOnNetworkFS_default)
	// Show the Extended Attributes tab
	, showXAttrView(showXAttrView_default)
	// Thumbnail directory packages (e.g. Wii U)
	, thumbnailDirectoryPackages(thumbnailDirectoryPackages_default)
{
	// NOTE: Configuration is also initialized in the reset() function.
	dmgTSMode = dmgTSMode_default;
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
	extImgDownloadEnabled = extImgDownloadEnabled_default;
	useIntIconForSmallSizes = useIntIconForSmallSizes_default;
	storeFileOriginInfo = storeFileOriginInfo_default;

	// Image bandwidth options
	imgBandwidthUnmetered = Config::ImgBandwidth::HighRes;
	imgBandwidthMetered = Config::ImgBandwidth::NormalRes;
	// Compatibility with older settings
	isNewBandwidthOptionSet = false;
	downloadHighResScans = false;

	// DMG title screen mode
	dmgTSMode = dmgTSMode_default;

	// Overlay icon
	showDangerousPermissionsOverlayIcon = showDangerousPermissionsOverlayIcon_default;
	// Enable thumbnail and metadata on network FS
	enableThumbnailOnNetworkFS = enableThumbnailOnNetworkFS_default;
	// Show the Extended Attributes tab
	showXAttrView = showXAttrView_default;
	// Thumbnail directory packages (e.g. Wii U)
	thumbnailDirectoryPackages = thumbnailDirectoryPackages_default;
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
		// Downloads. Check for one of the boolean options.
		bool *bParam = nullptr;
		Config::ImgBandwidth *ibParam = nullptr;

		if (!strcasecmp(name, "ExtImageDownload")) {
			bParam = &extImgDownloadEnabled;
		} else if (!strcasecmp(name, "UseIntIconForSmallSizes")) {
			bParam = &useIntIconForSmallSizes;
		} else if (!strcasecmp(name, "StoreFileOriginInfo")) {
			bParam = &storeFileOriginInfo;
		} else if (!strcasecmp(name, "PalLanguageForGameTDB")) {
			// PAL language. Parse the language code.
			// NOTE: Converting to lowercase.
			// TODO: Only allow valid language codes?
			palLanguageForGameTDB = 0;
			for (unsigned int i = 0; i < 4 && *value != '\0'; i++, value++) {
				palLanguageForGameTDB <<= 8;
				palLanguageForGameTDB |= TOLOWER(*value);
			}
			return 1;
		} else if (!strcasecmp(name, "ImgBandwidthUnmetered")) {
			isNewBandwidthOptionSet = true;
			ibParam = &imgBandwidthUnmetered;
		} else if (!strcasecmp(name, "ImgBandwidthMetered")) {
			isNewBandwidthOptionSet = true;
			ibParam = &imgBandwidthMetered;
		} else if (!strcasecmp(name, "DownloadHighResScans")) {
			bParam = &downloadHighResScans;
		} else {
			// Invalid option.
			return 1;
		}

		if (bParam) {
			// Boolean value.
			// Acceptable values are "true", "false", "1", and "0".
			if (!strcasecmp(value, "true") || !strcmp(value, "1")) {
				*bParam = true;
			} else if (!strcasecmp(value, "false") || !strcmp(value, "0")) {
				*bParam = false;
			} else {
				// TODO: Show a warning or something?
			}
		} else if (ibParam) {
			// ImgBandwidth value.
			if (!strcasecmp(value, "None")) {
				*ibParam = Config::ImgBandwidth::None;
			} else if (!strcasecmp(value, "NormalRes")) {
				*ibParam = Config::ImgBandwidth::NormalRes;
			} else if (!strcasecmp(value, "HighRes")) {
				*ibParam = Config::ImgBandwidth::HighRes;
			} else {
				// Invalid value.
				return 1;
			}
		} else {
			// Neither are set!
			assert(!"Missing bParam or ibParam.");
			return 1;
		}
	} else if (!strcasecmp(section, "DMGTitleScreenMode")) {
		// DMG title screen mode.
		Config::DMG_TitleScreen_Mode dmg_key, dmg_value;

		// Parse the key.
		if (!strcasecmp(name, "DMG")) {
			dmg_key = Config::DMG_TitleScreen_Mode::DMG;
		} else if (!strcasecmp(name, "SGB")) {
			dmg_key = Config::DMG_TitleScreen_Mode::SGB;
		} else if (!strcasecmp(name, "CGB")) {
			dmg_key = Config::DMG_TitleScreen_Mode::CGB;
		} else {
			// Invalid key.
			return 1;
		}

		// Parse the value.
		if (!strcasecmp(value, "DMG")) {
			dmg_value = Config::DMG_TitleScreen_Mode::DMG;
		} else if (!strcasecmp(value, "SGB")) {
			dmg_value = Config::DMG_TitleScreen_Mode::SGB;
		} else if (!strcasecmp(value, "CGB")) {
			dmg_value = Config::DMG_TitleScreen_Mode::CGB;
		} else {
			// Invalid value.
			return 1;
		}

		dmgTSMode[static_cast<size_t>(dmg_key)] = dmg_value;
	} else if (!strcasecmp(section, "Options")) {
		// Options.
		bool *bParam;
		if (!strcasecmp(name, "ShowDangerousPermissionsOverlayIcon")) {
			bParam = &showDangerousPermissionsOverlayIcon;
		} else if (!strcasecmp(name, "EnableThumbnailOnNetworkFS")) {
			bParam = &enableThumbnailOnNetworkFS;
		} else if (!strcasecmp(name, "ShowXAttrView")) {
			bParam = &showXAttrView;
		} else if (!strcasecmp(name, "ThumbnailDirectoryPackages")) {
			bParam = &thumbnailDirectoryPackages;
		} else {
			// Invalid option.
			return 1;
		}

		// Parse the value.
		// Acceptable values are "true", "false", "1", and "0".
		if (!strcasecmp(value, "true") || !strcmp(value, "1")) {
			*bParam = true;
		} else if (!strcasecmp(value, "false") || !strcmp(value, "0")) {
			*bParam = false;
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
			// First byte of 'name' is a length value for optimization purposes.
			// NOTE: "\x08ExtMedia" is interpreted as a 0x8E byte by both
			// MSVC 2015 and gcc-4.5.2. In order to get it to work correctly,
			// we have to store the length byte separately from the actual
			// image type name.
			static const array<const char*, RomData::IMG_EXT_MAX+1> imageTypeNames = {{
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
			}};
			static_assert(ARRAY_SIZE(imageTypeNames) == RomData::IMG_EXT_MAX+1, "imageTypeNames[] is the wrong size.");

			RomData::ImageType imgType = static_cast<RomData::ImageType>(-1);
			for (size_t i = 0; i < imageTypeNames.size(); i++) {
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
			if (imgbf & (1U << imgType)) {
				// Duplicate image type!
				continue;
			}
			imgbf |= (1U << imgType);

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
			std::transform(className.begin(), className.end(), className.begin(),
				[](unsigned char c) noexcept -> char { return std::tolower(c); });

			// Add the class name information to the map.
			uint32_t keyIdx = static_cast<uint32_t>(vStartPos);
			keyIdx |= (count << 24);
			mapImgTypePrio.emplace(className, keyIdx);
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
	return q;
}

/** Image types **/

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
		return ImgTypeResult::ErrorInvalidParams;
	}

	// Find the class name in the map.
	RP_D(const Config);
	string className_lower(className);
	std::transform(className_lower.begin(), className_lower.end(), className_lower.begin(), ::tolower);
	auto iter = d->mapImgTypePrio.find(className_lower);
	if (iter == d->mapImgTypePrio.end()) {
		// Class name not found.
		// Use the global defaults.
		imgTypePrio->imgTypes = d->defImgTypePrio.data();
		imgTypePrio->length = d->defImgTypePrio.size();
		return ImgTypeResult::SuccessDefaults;
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
		return ImgTypeResult::ErrorMapCorrupted;
	}

	// Is the first entry RomData::IMG_DISABLED?
	if (d->vImgTypePrio[idx] == static_cast<uint8_t>(RomData::IMG_DISABLED)) {
		// Thumbnails are disabled for this class.
		return ImgTypeResult::Disabled;
	}

	// Return the starting address and length.
	imgTypePrio->imgTypes = &d->vImgTypePrio[idx];
	imgTypePrio->length = len;
	return ImgTypeResult::Success;
}

/**
 * Get the default image type priority data.
 * This is the priority data used if a custom configuration
 * is not defined for a given class.
 * @param imgTypePrio	[out] Image type priority data.
 */
void Config::getDefImgTypePrio(ImgTypePrio_t *imgTypePrio)
{
	assert(imgTypePrio != nullptr);
	if (imgTypePrio) {
		imgTypePrio->imgTypes = ConfigPrivate::defImgTypePrio.data();
		imgTypePrio->length = ConfigPrivate::defImgTypePrio.size();
	}
}

/** Download options **/

/**
 * Get the array of language codes available on GameTDB.
 * @return NULL-terminated array of language codes.
 */
const uint32_t *Config::get_all_pal_lcs(void)
{
	return ConfigPrivate::pal_lc.data();
}

/**
 * Language code for PAL titles on GameTDB.
 * @return Language code.
 */
uint32_t Config::palLanguageForGameTDB(void) const
{
	RP_D(const Config);
	return d->palLanguageForGameTDB;
}

/* Image bandwidth settings */

/**
 * What type of images should be downloaded on unmetered connections?
 * These connections do not charge for usage.
 * @return ImgBandwidth for unmetered connections
 */
Config::ImgBandwidth Config::imgBandwidthUnmetered(void) const
{
	RP_D(const Config);
	if (d->isNewBandwidthOptionSet) {
		// New options are set.
		return d->imgBandwidthUnmetered;
	} else {
		// New options are *not* set.
		// Use the old option to select between high-res and normal-res.
		return (d->downloadHighResScans) ? ImgBandwidth::HighRes : ImgBandwidth::NormalRes;
	}
}

/**
 * What type of images should be downloaded on metered connections?
 * These connections may charge for usage.
 * @return ImgBandwidth for metered connections
 */
Config::ImgBandwidth Config::imgBandwidthMetered(void) const
{
	RP_D(const Config);
	if (d->isNewBandwidthOptionSet) {
		// New options are set.
		return d->imgBandwidthMetered;
	} else {
		// New options are *not* set.
		// Default to normal resolution for metered connections.
		return ImgBandwidth::NormalRes;
	}
}

/** DMG title screen mode **/

/**
 * Which title screen should we use for the specified DMG ROM type?
 * @param romType DMG ROM type.
 * @return Title screen to use.
 */
Config::DMG_TitleScreen_Mode Config::dmgTitleScreenMode(DMG_TitleScreen_Mode romType) const
{
	assert(romType >= DMG_TitleScreen_Mode::DMG);
	assert(romType <  DMG_TitleScreen_Mode::Max);
	if (romType <  DMG_TitleScreen_Mode::DMG ||
	    romType >= DMG_TitleScreen_Mode::Max)
	{
		// Invalid ROM type. Return DMG.
		return DMG_TitleScreen_Mode::DMG;
	}

	RP_D(const Config);
	return d->dmgTSMode[static_cast<size_t>(romType)];
}

/** Boolean configuration options **/

/**
 * Get a boolean configuration option.
 * @param option Boolean configuration option
 * @return Value. (If the option is invalid, returns false.)
 */
bool Config::getBoolConfigOption(BoolConfig option) const
{
	RP_D(const Config);

	switch (option) {
		default:
			assert(!"Invalid BoolConfig option.");
			return false;

		case BoolConfig::Downloads_ExtImgDownloadEnabled:
			return d->extImgDownloadEnabled;
		case BoolConfig::Downloads_UseIntIconForSmallSizes:
			return d->useIntIconForSmallSizes;
		case BoolConfig::Downloads_StoreFileOriginInfo:
			return d->storeFileOriginInfo;

		case BoolConfig::Options_ShowDangerousPermissionsOverlayIcon:
			return d->showDangerousPermissionsOverlayIcon;
		case BoolConfig::Options_EnableThumbnailOnNetworkFS:
			return d->enableThumbnailOnNetworkFS;
		case BoolConfig::Options_ShowXAttrView:
			return d->showXAttrView;
		case BoolConfig::Options_ThumbnailDirectoryPackages:
			return d->thumbnailDirectoryPackages;
	}
}

/**** Default values ****/

/** Download options **/

#define DEFAULT_VALUE_IMPL(T, name) \
T Config::name##_default(void) \
{ \
	return ConfigPrivate::name##_default; \
}
DEFAULT_VALUE_IMPL(uint32_t, palLanguageForGameTDB)
DEFAULT_VALUE_IMPL(Config::ImgBandwidth, imgBandwidthUnmetered)
DEFAULT_VALUE_IMPL(Config::ImgBandwidth, imgBandwidthMetered)

Config::DMG_TitleScreen_Mode Config::dmgTitleScreenMode_default(DMG_TitleScreen_Mode romType)
{
	assert(romType >= DMG_TitleScreen_Mode::DMG);
	assert(romType <  DMG_TitleScreen_Mode::Max);
	if (romType <  DMG_TitleScreen_Mode::DMG ||
	    romType >= DMG_TitleScreen_Mode::Max)
	{
		// Invalid ROM type. Return DMG.
		return DMG_TitleScreen_Mode::DMG;
	}

	return ConfigPrivate::dmgTSMode_default[static_cast<size_t>(romType)];
}

/**
 * Get the default value for a boolean configuration option.
 * @param option Boolean configuration option
 * @return Value. (If the option is invalid, returns false.)
 */
bool Config::getBoolConfigOption_default(BoolConfig option)
{
	switch (option) {
		default:
			assert(!"Invalid BoolConfig option.");
			return false;

		case BoolConfig::Downloads_ExtImgDownloadEnabled:
			return ConfigPrivate::extImgDownloadEnabled_default;
		case BoolConfig::Downloads_UseIntIconForSmallSizes:
			return ConfigPrivate::useIntIconForSmallSizes_default;
		case BoolConfig::Downloads_StoreFileOriginInfo:
			return ConfigPrivate::storeFileOriginInfo_default;

		case BoolConfig::Options_ShowDangerousPermissionsOverlayIcon:
			return ConfigPrivate::showDangerousPermissionsOverlayIcon_default;
		case BoolConfig::Options_EnableThumbnailOnNetworkFS:
			return ConfigPrivate::enableThumbnailOnNetworkFS_default;
		case BoolConfig::Options_ShowXAttrView:
			return ConfigPrivate::showXAttrView_default;
		case BoolConfig::Options_ThumbnailDirectoryPackages:
			return ConfigPrivate::thumbnailDirectoryPackages_default;
	}
}

}
