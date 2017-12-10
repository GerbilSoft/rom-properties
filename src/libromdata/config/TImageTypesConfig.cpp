/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * TImageTypesConfig.cpp: Image Types editor template.                     *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_CONFIG_TIMAGETYPESCONFIG_CPP__
#define __ROMPROPERTIES_LIBROMDATA_CONFIG_TIMAGETYPESCONFIG_CPP__

// NOTE: This is #included in other files,
// so don't use any 'using' statements!

#include "TImageTypesConfig.hpp"
#include "librpbase/config/Config.hpp"

// librpbase
#include "librpbase/TextFuncs.hpp"
#include "librpbase/RomData.hpp"	// for IMG_* constants
#include "libi18n/i18n.h"
using namespace LibRpBase;

// RomData subclasses with images.
// Does not include texture files, since those are always
// thumbnailed using IMG_INT_IMAGE.
#include "Other/Amiibo.hpp"
#include "Console/Dreamcast.hpp"
#include "Console/DreamcastSave.hpp"
#include "Console/GameCube.hpp"
#include "Console/GameCubeSave.hpp"
#include "Other/NintendoBadge.hpp"
#include "Handheld/NintendoDS.hpp"
#include "Handheld/Nintendo3DS.hpp"
#include "Console/PlayStationSave.hpp"
#include "Console/WiiU.hpp"

namespace LibRomData {

// System data.
template<typename ComboBox>
const SysData_t TImageTypesConfig<ComboBox>::sysData[] = {
	SysDataEntry(Amiibo),
	SysDataEntry(NintendoBadge),
	SysDataEntry(Dreamcast),
	SysDataEntry(DreamcastSave),
	SysDataEntry(GameCube),
	SysDataEntry(GameCubeSave),
	SysDataEntry(NintendoDS),
	SysDataEntry(Nintendo3DS),
	SysDataEntry(PlayStationSave),
	SysDataEntry(WiiU),
};

template<typename ComboBox>
TImageTypesConfig<ComboBox>::TImageTypesConfig()
	: changed(false)
{
	static_assert(std::is_pointer<ComboBox>::value, "TImageTypesConfig template parameter must be a pointer.");

	// Clear the arrays.
	memset(cboImageType, 0, sizeof(cboImageType));
	memset(imageTypes, 0xFF, sizeof(imageTypes));
	memset(validImageTypes, 0, sizeof(validImageTypes));
	memset(sysIsDefault, 0, sizeof(sysIsDefault));
}

template<typename ComboBox>
TImageTypesConfig<ComboBox>::~TImageTypesConfig()
{ }

/**
 * Create the grid of labels and ComboBoxes.
 */
template<typename ComboBox>
void TImageTypesConfig<ComboBox>::createGrid(void)
{
	// TODO: Make sure the grid wasn't already created.

	// Create the grid labels.
	createGridLabels();

	// Create the ComboBoxes.
	for (unsigned int sys = 0; sys < SYS_COUNT; sys++) {
		// Get supported image types.
		uint32_t imgbf = sysData[sys].getTypes();
		assert(imgbf != 0);

		validImageTypes[sys] = 0;
		for (unsigned int imageType = 0; imgbf != 0 && imageType < IMG_TYPE_COUNT; imageType++, imgbf >>= 1) {
			if (!(imgbf & 1)) {
				// Current image type is not supported.
				continue;
			}

			// Create the ComboBox.
			createComboBox(sysAndImageTypeToCbid(sys, imageType));
			// Increment the valid image types counter.
			validImageTypes[sys]++;
		}

		// Add strings to the ComboBoxes.
		for (int imageType = IMG_TYPE_COUNT-1; imageType >= 0; imageType--) {
			if (cboImageType[sys][imageType] != nullptr) {
				addComboBoxStrings(sysAndImageTypeToCbid(sys, imageType), validImageTypes[sys]);
			}
		}

		// ComboBox finalization, if necessary.
		finishComboBoxes();
	}

	// Load the configuration.
	memset(imageTypes, 0xFF, sizeof(imageTypes));
	reset();
}

/**
 * (Re-)Load the configuration into the grid.
 * @param loadDefaults If true, use the default configuration instead of the user configuration.
 * @return True if anything was modified; false if not.
 */
template<typename ComboBox>
bool TImageTypesConfig<ComboBox>::reset_int(bool loadDefaults)
{
	bool hasChanged = false;

	// Which ComboBoxes need to be reset to "No"?
	bool cbo_needsReset[SYS_COUNT][IMG_TYPE_COUNT];
	memset(cbo_needsReset, true, sizeof(cbo_needsReset));

	const Config *const config = Config::instance();

	Config::ImgTypePrio_t imgTypePrio;
	if (loadDefaults) {
		// Use the default image priority for all types.
		memset(sysIsDefault, true, sizeof(sysIsDefault));
		config->getDefImgTypePrio(&imgTypePrio);
	}

	for (int sys = SYS_COUNT-1; sys >= 0; sys--) {
		if (!loadDefaults) {
			// Get the image priority.
			Config::ImgTypeResult res = config->getImgTypePrio(sysData[sys].className, &imgTypePrio);
			bool no_thumbs = false;
			switch (res) {
				case Config::IMGTR_SUCCESS:
					// Image type priority received successfully.
					sysIsDefault[sys] = false;
					break;
				case Config::IMGTR_SUCCESS_DEFAULTS:
					// Image type priority received successfully.
					// IMGTR_SUCCESS_DEFAULTS indicates the returned
					// data is the default priority, since a custom
					// configuration was not found for this class.
					sysIsDefault[sys] = true;
					break;
				case Config::IMGTR_DISABLED:
					// Thumbnails are disabled for this class.
					no_thumbs = true;
					break;
				default:
					// Should not happen...
					assert(!"Invalid return value from Config::getImgTypePrio().");
					no_thumbs = true;
					break;
			}
			if (no_thumbs)
				continue;
		}

		int nextPrio = 0;	// Next priority value to use.
		bool imageTypeSet[IMG_TYPE_COUNT];	// Element set to true once an image type priority is read.
		memset(imageTypeSet, 0, sizeof(imageTypeSet));

		ComboBox *p_cboImageType = &cboImageType[sys][0];
		for (unsigned int i = 0; i < imgTypePrio.length && nextPrio <= validImageTypes[sys]; i++)
		{
			uint8_t imageType = imgTypePrio.imgTypes[i];
			assert(imageType < IMG_TYPE_COUNT);
			if (imageType >= IMG_TYPE_COUNT) {
				// Invalid image type.
				// NOTE: 0xFF (no image) should not be encountered here.
				continue;
			}

			if (p_cboImageType[imageType] && !imageTypeSet[imageType]) {
				// Set the image type.
				imageTypeSet[imageType] = true;
				if (imageType < IMG_TYPE_COUNT) {
					if (imageTypes[sys][imageType] != nextPrio) {
						imageTypes[sys][imageType] = nextPrio;
						hasChanged = true;

						// NOTE: Using the actual priority value, not the ComboBox index.
						cboImageType_setPriorityValue(sysAndImageTypeToCbid(sys, imageType), nextPrio);
					}
					cbo_needsReset[sys][imageType] = false;
					nextPrio++;
				}
			}
		}
	}

	// Set ComboBoxes that don't have a priority to "No".
	for (int sys = SYS_COUNT-1; sys >= 0; sys--) {
		unsigned int cbid = sysAndImageTypeToCbid((unsigned int)sys, IMG_TYPE_COUNT-1);
		for (int imageType = IMG_TYPE_COUNT-1; imageType >= 0; imageType--, cbid--) {
			if (cbo_needsReset[sys][imageType] && cboImageType[sys][imageType]) {
				if (imageTypes[sys][imageType] != 0xFF) {
					hasChanged = true;
					cboImageType_setPriorityValue(cbid, 0xFF);
				}
			}
		}
	}

	return hasChanged;
}

/**
 * (Re-)Load the configuration into the grid.
 */
template<typename ComboBox>
void TImageTypesConfig<ComboBox>::reset(void)
{
	reset_int(false);
	// No longer changed.
	changed = false;
}

/**
 * Load the default configuration.
 * This does NOT save and will not clear 'changed'.
 * @return True if anything was modified; false if not.
 */
template<typename ComboBox>
bool TImageTypesConfig<ComboBox>::loadDefaults(void)
{
	bool bRet = reset_int(true);
	if (bRet) {
		changed = true;
	}
	return bRet;
}

/**
 * Save the configuration from the grid.
 * @return 0 on success; negative POSIX error code on error.
 */
template<typename ComboBox>
int TImageTypesConfig<ComboBox>::save(void)
{
	if (!changed) {
		// No changes. Nothing to save.
		return 0;
	}

	// Initialize the Save subsystem.
	int ret = saveStart();
	if (ret != 0)
		return ret;

	// Image types are stored in the imageTypes[] array.
	const uint8_t *pImageTypes = imageTypes[0];

	// TODO: Switch back to std::ostringstream since everything's
	// using UTF-8 now? (u16string ostringstream didn't work.)
	std::string imageTypeList;
	imageTypeList.reserve(128);
	for (unsigned int sys = 0; sys < SYS_COUNT; sys++) {
		// Is this system using the default configuration?
		if (sysIsDefault[sys]) {
			// Default configuration. Write an empty string.
			ret = saveWriteEntry(sysData[sys].className, "");
			if (ret != 0) {
				// Error...
				saveFinish();
				return ret;
			}
			pImageTypes += ARRAY_SIZE(imageTypes[0]);
			continue;
		}

		// Clear the imageTypeList.
		imageTypeList.clear();

		// Format of imageTypes[]:
		// - Index: Image type.
		// - Value: Priority.
		// We need to swap index and value.
		uint8_t imgTypePrio[IMG_TYPE_COUNT];
		memset(imgTypePrio, 0xFF, sizeof(imgTypePrio));
		for (unsigned int imageType = 0; imageType < IMG_TYPE_COUNT;
		     imageType++, pImageTypes++)
		{
			if (*pImageTypes >= IMG_TYPE_COUNT) {
				// Image type is either not valid for this system
				// or is set to "No".
				continue;
			}
			imgTypePrio[*pImageTypes] = imageType;
		}

		// Convert the image type priority to strings.
		// TODO: Export the string data from Config.
		static const char *const conf_imageTypeNames[] = {
			"IntIcon",
			"IntBanner",
			"IntMedia",
			"IntImage",
			"ExtMedia",
			"ExtCover",
			"ExtCover3D",
			"ExtCoverFull",
			"ExtBox",
		};
		static_assert(ARRAY_SIZE(conf_imageTypeNames) == IMG_TYPE_COUNT, "conf_imageTypeNames[] is the wrong size.");

		bool hasOne = false;
		for (unsigned int i = 0; i < ARRAY_SIZE(imgTypePrio); i++) {
			const uint8_t imageType = imgTypePrio[i];
			if (imageType < IMG_TYPE_COUNT) {
				if (hasOne)
					imageTypeList += ',';
				hasOne = true;
				imageTypeList += conf_imageTypeNames[imageType];
			}
		}

		if (hasOne) {
			// At least one image type is enabled.
			ret = saveWriteEntry(sysData[sys].className, imageTypeList.c_str());
		} else {
			// All image types are disabled.
			ret = saveWriteEntry(sysData[sys].className, "No");
		}
		if (ret != 0) {
			// Error...
			saveFinish();
			return ret;
		}
	}

	ret = saveFinish();
	if (ret == 0) {
		// No longer changed.
		changed = false;
	}
	return ret;
}

/**
 * Get an image type name.
 * @param imageType Image type ID.
 * @return Image type name, or nullptr if invalid.
 */
template<typename ComboBox>
const char *TImageTypesConfig<ComboBox>::imageTypeName(unsigned int imageType)
{
	// Image type names.
	static const char *const imageType_names[] = {
		/** Internal **/

		// tr: IMG_INT_ICON
		NOP_C_("TImageTypesConfig|ImageTypeDisp", "Internal\nIcon"),
		// tr: IMG_INT_BANNER
		NOP_C_("TImageTypesConfig|ImageTypeDisp", "Internal\nBanner"),
		// tr: IMG_INT_MEDIA
		NOP_C_("TImageTypesConfig|ImageTypeDisp", "Internal\nMedia"),
		// tr: IMG_INT_IMAGE
		NOP_C_("TImageTypesConfig|ImageTypeDisp", "Internal\nImage"),

		/** External **/

		// tr: IMG_EXT_MEDIA
		NOP_C_("TImageTypesConfig|ImageTypeDisp", "External\nMedia"),
		// tr: IMG_EXT_COVER
		NOP_C_("TImageTypesConfig|ImageTypeDisp", "External\nCover"),
		// tr: IMG_EXT_COVER_3D
		NOP_C_("TImageTypesConfig|ImageTypeDisp", "External\n3D Cover"),
		// tr: IMG_EXT_COVER_FULL
		NOP_C_("TImageTypesConfig|ImageTypeDisp", "External\nFull Cover"),
		// tr: IMG_EXT_BOX
		NOP_C_("TImageTypesConfig|ImageTypeDisp", "External\nBox"),
	};
	static_assert(ARRAY_SIZE(imageType_names) == IMG_TYPE_COUNT,
		"imageType_names[] needs to be updated.");

	return dpgettext_expr(RP_I18N_DOMAIN, "TImageTypesConfig|ImageTypeDisp", imageType_names[imageType]);
}

/**
 * Get a system name.
 * @param sys System ID.
 * @return System name, or nullptr if invalid.
 */
template<typename ComboBox>
const char *TImageTypesConfig<ComboBox>::sysName(unsigned int sys)
{
	// System names.
	static const char *const sysNames[] = {
		// tr: amiibo
		NOP_C_("TImageTypesConfig|SysName", "amiibo"),
		// tr: NintendoBadge
		NOP_C_("TImageTypesConfig|SysName", "Badge Arcade"),
		// tr: Dreamcast
		NOP_C_("TImageTypesConfig|SysName", "Dreamcast"),
		// tr: DreamcastSave
		NOP_C_("TImageTypesConfig|SysName", "Dreamcast Saves"),
		// tr: GameCube
		NOP_C_("TImageTypesConfig|SysName", "GameCube / Wii"),
		// tr: GameCubeSave
		NOP_C_("TImageTypesConfig|SysName", "GameCube Saves"),
		// tr: NintendoDS
		NOP_C_("TImageTypesConfig|SysName", "Nintendo DS(i)"),
		// tr: Nintendo3DS
		NOP_C_("TImageTypesConfig|SysName", "Nintendo 3DS"),
		// tr: PlayStationSave
		NOP_C_("TImageTypesConfig|SysName", "PlayStation Saves"),
		// tr: WiiU
		NOP_C_("TImageTypesConfig|SysName", "Wii U"),
	};
	static_assert(ARRAY_SIZE(sysNames) == SYS_COUNT,
		"sysNames[] needs to be updated.");

	return dpgettext_expr(RP_I18N_DOMAIN, "TImageTypesConfig|SysName", sysNames[sys]);
}

/**
 * A ComboBox index was changed by the user.
 * @param cbid ComboBox ID.
 * @param prio New priority value. (0xFF == no)
 * @return True if changed; false if not.
 */
template<typename ComboBox>
bool TImageTypesConfig<ComboBox>::cboImageType_priorityValueChanged(unsigned int cbid, unsigned int prio)
{
	const unsigned int sys = sysFromCbid(cbid);
	const unsigned int imageType = imageTypeFromCbid(cbid);
	if (!validateSysImageType(sys, imageType))
		return false;

	const uint8_t prev_prio = imageTypes[sys][imageType];
	if (prev_prio == prio) {
		// No change.
		return false;
	}

	if (prio >= 0 && prio != 0xFF) {
		// Check for any image types that have the new priority.
		for (int i = IMG_TYPE_COUNT-1; i >= 0; i--) {
			if ((unsigned int)i == imageType)
				continue;
			if (cboImageType[sys][i] != nullptr && imageTypes[sys][i] == (uint8_t)prio) {
				// Found a match! Swap the priority.
				imageTypes[sys][i] = prev_prio;
				cboImageType_setPriorityValue(sysAndImageTypeToCbid(sys, i), prev_prio);
				break;
			}
		}
	}

	// Save the image priority value.
	imageTypes[sys][imageType] = prio;
	// Mark this configuration as no longer being default.
	sysIsDefault[sys] = false;
	// Configuration has been changed.
	changed = true;
	return true;
}

}

#endif /* __ROMPROPERTIES_LIBROMDATA_CONFIG_TIMAGETYPESCONFIG_CPP__ */
