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

#include "TImageTypesConfig.hpp"
#include "librpbase/config/Config.hpp"

// librpbase
#include "librpbase/TextFuncs.hpp"
using LibRpBase::rp_string;

// RomData subclasses with images.
#include "../Amiibo.hpp"
#include "../DreamcastSave.hpp"
#include "../GameCube.hpp"
#include "../GameCubeSave.hpp"
#include "../NintendoDS.hpp"
#include "../Nintendo3DS.hpp"
#include "../PlayStationSave.hpp"
#include "../WiiU.hpp"

namespace LibRomData {

// Image type names.
template<typename ComboBox>
const rp_char *const TImageTypesConfig<ComboBox>::imageTypeNames[IMG_TYPE_COUNT] = {
	_RP("Internal\nIcon"),
	_RP("Internal\nBanner"),
	_RP("Internal\nMedia"),
	_RP("External\nMedia"),
	_RP("External\nCover"),
	_RP("External\n3D Cover"),
	_RP("External\nFull Cover"),
	_RP("External\nBox"),
};

// System data.
template<typename ComboBox>
const SysData_t TImageTypesConfig<ComboBox>::sysData[SYS_COUNT] = {
	SysDataEntry(Amiibo,		_RP("amiibo")),
	SysDataEntry(DreamcastSave,	_RP("Dreamcast Saves")),
	SysDataEntry(GameCube,		_RP("GameCube / Wii")),
	SysDataEntry(GameCubeSave,	_RP("GameCube Saves")),
	SysDataEntry(NintendoDS,	_RP("Nintendo DS(i)")),
	SysDataEntry(Nintendo3DS,	_RP("Nintendo 3DS")),
	SysDataEntry(PlayStationSave,	_RP("PlayStation Saves")),
	SysDataEntry(WiiU,		_RP("Wii U")),
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
	reset();
}

/**
 * (Re-)Load the configuration into the grid.
 */
template<typename ComboBox>
void TImageTypesConfig<ComboBox>::reset(void)
{
	// Reset all ComboBox objects first.
	for (int sys = SYS_COUNT-1; sys >= 0; sys--) {
		unsigned int cbid = sysAndImageTypeToCbid(sys, IMG_TYPE_COUNT-1);
		for (int imageType = IMG_TYPE_COUNT-1; imageType >= 0; imageType--, cbid--) {
			if (cboImageType[sys][imageType]) {
				// NOTE: Using the actual priority value, not the ComboBox index.
				cboImageType_setPriorityValue(cbid, 0xFF);
			}
		}
	}

	const Config *const config = Config::instance();
	for (int sys = SYS_COUNT-1; sys >= 0; sys--) {
		ComboBox *p_cboImageType = &cboImageType[sys][0];

		// Get the image priority.
		Config::ImgTypePrio_t imgTypePrio;
		Config::ImgTypeResult res = config->getImgTypePrio(sysData[sys].classNameA, &imgTypePrio);
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

		int nextPrio = 0;	// Next priority value to use.
		bool imageTypeSet[IMG_TYPE_COUNT];	// Element set to true once an image type priority is read.
		memset(imageTypeSet, 0, sizeof(imageTypeSet));

		p_cboImageType = &cboImageType[sys][0];
		for (unsigned int i = 0; i < imgTypePrio.length && nextPrio <= validImageTypes[sys]; i++)
		{
			uint8_t imageType = imgTypePrio.imgTypes[i];
			assert(imageType == 0xFF || imageType < IMG_TYPE_COUNT);
			if (imageType >= IMG_TYPE_COUNT && imageType != 0xFF) {
				// Invalid image type.
				continue;
			}
			if (p_cboImageType[imageType] && !imageTypeSet[imageType]) {
				// Set the image type.
				imageTypeSet[imageType] = true;
				if (imageType < IMG_TYPE_COUNT) {
					imageTypes[sys][imageType] = nextPrio;
					// NOTE: Using the actual priority value, not the ComboBox index.
					cboImageType_setPriorityValue(sysAndImageTypeToCbid(sys, imageType), nextPrio);
					nextPrio++;
				}
			}
		}
	}

	// No longer changed.
	changed = false;
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

	// NOTE: Using an rp_string with reserved storage
	// instead of ostringstream, since we had problems
	// with u16string ostringstream before.
	rp_string imageTypeList;
	imageTypeList.reserve(128);
	for (unsigned int sys = 0; sys < SYS_COUNT; sys++) {
		// Is this system using the default configuration?
		if (sysIsDefault[sys]) {
			// Default configuration. Write an empty string.
			ret = saveWriteEntry(sysData[sys].classNameRP, _RP(""));
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
		static const rp_char *const imageTypeNames[IMG_TYPE_COUNT] = {
			_RP("IntIcon"),
			_RP("IntBanner"),
			_RP("IntMedia"),
			_RP("ExtMedia"),
			_RP("ExtCover"),
			_RP("ExtCover3D"),
			_RP("ExtCoverFull"),
			_RP("ExtBox"),
		};
		static_assert(ARRAY_SIZE(imageTypeNames) == IMG_TYPE_COUNT, "imageTypeNames[] is the wrong size.");

		bool hasOne = false;
		for (unsigned int i = 0; i < ARRAY_SIZE(imgTypePrio); i++) {
			const uint8_t imageType = imgTypePrio[i];
			if (imageType < IMG_TYPE_COUNT) {
				if (hasOne)
					imageTypeList += _RP_CHR(',');
				hasOne = true;
				imageTypeList += imageTypeNames[imageType];
			}
		}

		if (hasOne) {
			// At least one image type is enabled.
			ret = saveWriteEntry(sysData[sys].classNameRP, imageTypeList.c_str());
		} else {
			// All image types are disabled.
			ret = saveWriteEntry(sysData[sys].classNameRP, _RP("No"));
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
 * A ComboBox index was changed by the user.
 * @param cbid ComboBox ID.
 * @param prio New priority value. (0xFF == no)
 */
template<typename ComboBox>
void TImageTypesConfig<ComboBox>::cboImageType_priorityValueChanged(unsigned int cbid, unsigned int prio)
{
	const unsigned int sys = sysFromCbid(cbid);
	const unsigned int imageType = imageTypeFromCbid(cbid);
	if (!validateSysImageType(sys, imageType))
		return;

	if (prio >= 0 && prio != 0xFF) {
		// Check for any image types that have the new priority.
		const uint8_t prev_prio = imageTypes[sys][imageType];
		for (int i = IMG_TYPE_COUNT-1; i >= 0; i--) {
			if (i == imageType)
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
}

}

#endif /* __ROMPROPERTIES_LIBROMDATA_CONFIG_TIMAGETYPESCONFIG_CPP__ */
