/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * Config.hpp: Configuration manager.                                      *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ConfReader.hpp"

// C includes (C++ namespace)
#include <cstdint>

namespace LibRpBase {

class RP_LIBROMDATA_PUBLIC Config : public ConfReader
{
protected:
	/**
	 * Config class.
	 *
	 * This class is a Singleton, so the caller must obtain a
	 * pointer to the class using instance().
	 */
	Config();

private:
	typedef ConfReader super;
	RP_DISABLE_COPY(Config)

private:
	friend class ConfigPrivate;

public:
	/**
	 * Get the Config instance.
	 *
	 * This automatically initializes the object and
	 * reloads the configuration if it has been modified.
	 *
	 * @return Config instance.
	 */
	static Config *instance(void);

public:
	/** Image types **/

	// Image type priority data.
	struct ImgTypePrio_t {
		const uint8_t *imgTypes;	// Image types.
		uint32_t length;		// Length of imgTypes array.
	};

	// TODO: Function to get image type priority for a specified class.

public:
	/** Image types **/

	enum class ImgTypeResult {
		ErrorMapCorrupted	= -2,	// Internal map is corrupted.
		ErrorInvalidParams	= -1,	// Invalid parameters.
		Success			= 0,	// Image type priority data returned successfully.
		SuccessDefaults		= 1,	// Custom configuration not defined; returning defaults.
		Disabled		= 2,	// Thumbnails are disabled for this class.
	};

	/**
	 * Get the default image type priority data.
	 * This is the priority data used if a custom configuration
	 * is not defined for a given class.
	 * @param imgTypePrio	[out] Image type priority data.
	 */
	static void getDefImgTypePrio(ImgTypePrio_t *imgTypePrio);

	/**
	 * Get the image type priority data for the specified class name.
	 * NOTE: Call load() before using this function.
	 * @param className	[in] Class name. (ASCII)
	 * @param imgTypePrio	[out] Image type priority data.
	 * @return ImgTypeResult
	 */
	ImgTypeResult getImgTypePrio(const char *className, ImgTypePrio_t *imgTypePrio) const;

	/** Download options **/

	/**
	 * Get the array of language codes available on GameTDB.
	 * @return NULL-terminated array of language codes.
	 */
	static const uint32_t *get_all_pal_lcs(void);

	/**
	 * Language code for PAL titles on GameTDB.
	 * @return Language code.
	 */
	uint32_t palLanguageForGameTDB(void) const;

	/* Image bandwidth options */

	enum class ImgBandwidth : uint8_t {
		None = 0,	// Don't download any images
		NormalRes = 1,	// Download normal-resolution images
		HighRes = 2,	// Download high-resolution images
	};

	/**
	 * What type of images should be downloaded on unmetered connections?
	 * These connections do not charge for usage.
	 * @return ImgBandwidth for unmetered connections
	 */
	ImgBandwidth imgBandwidthUnmetered(void) const;

	/**
	 * What type of images should be downloaded on metered connections?
	 * These connections may charge for usage.
	 * @return ImgBandwidth for metered connections
	 */
	ImgBandwidth imgBandwidthMetered(void) const;

	/**
	 * Convert Config::ImgBandwidth to a configuration setting string.
	 * @param index Config::ImgBandwidth
	 * @return Configuration setting string (If the index is invalid, defaults to "HighRes".)
	 */
	static const char *imgBandwidthToConfSetting(Config::ImgBandwidth imgbw);

	/** DMG title screen mode **/

	enum class DMG_TitleScreen_Mode : uint8_t {
		DMG,	// Use DMG mode title screens.
		SGB,	// Use SGB mode title screens if available.
		CGB,	// Use CGB mode title screens if available.

		Max
	};

	/**
	 * Which title screen should we use for the specified DMG ROM type?
	 * @param romType DMG ROM type.
	 * @return Title screen to use.
	 */
	DMG_TitleScreen_Mode dmgTitleScreenMode(DMG_TitleScreen_Mode romType) const;

	/** Boolean configuration options **/

	enum class BoolConfig {
		Downloads_ExtImgDownloadEnabled,
		Downloads_UseIntIconForSmallSizes,
		Downloads_StoreFileOriginInfo,

		Options_ShowDangerousPermissionsOverlayIcon,
		Options_EnableThumbnailOnNetworkFS,
		Options_ShowXAttrView,
		Options_ThumbnailDirectoryPackages,

		Max
	};

	/**
	 * Get a boolean configuration option.
	 * @param option Boolean configuration option
	 * @return Value. (If the option is invalid, returns false.)
	 */
	bool getBoolConfigOption(BoolConfig option) const;

public:
	/**** Default values ****/

	/** Download options **/

	/**
	 * Language code for PAL titles on GameTDB. (default value)
	 * @return Language code.
	 */
	static uint32_t palLanguageForGameTDB_default(void);

	/* Image bandwidth options */

	/**
	 * What type of images should be downloaded on unmetered connections? (default value)
	 * These connections do not charge for usage.
	 * @return ImgBandwidth for unmetered connections
	 */
	static ImgBandwidth imgBandwidthUnmetered_default(void);

	/**
	 * What type of images should be downloaded on metered connections? (default value)
	 * These connections may charge for usage.
	 * @return ImgBandwidth for metered connections
	 */
	static ImgBandwidth imgBandwidthMetered_default(void);

	/** DMG title screen mode **/

	/**
	 * Which title screen should we use for the specified DMG ROM type? (default value)
	 * @param romType DMG ROM type.
	 * @return Title screen to use.
	 */
	static DMG_TitleScreen_Mode dmgTitleScreenMode_default(DMG_TitleScreen_Mode romType);

	/** Boolean configuration options **/

	/**
	 * Get the default value for a boolean configuration option.
	 * @param option Boolean configuration option
	 * @return Value. (If the option is invalid, returns false.)
	 */
	static bool getBoolConfigOption_default(BoolConfig option);
};

}
