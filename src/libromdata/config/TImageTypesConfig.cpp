/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * TImageTypesConfig.cpp: Image Types editor template.                     *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// NOTE: This is #included in other files,
// so don't use any 'using' statements!

#include "TImageTypesConfig.hpp"
#include "librpbase/config/Config.hpp"

// librpbase
#include "librpbase/RomData.hpp"	// for IMG_* constants

// C includes (C++ namespace)
#include <cassert>

// C++ includes
#include <array>
#include <string>

namespace LibRomData {

template<typename ComboBox>
TImageTypesConfig<ComboBox>::TImageTypesConfig()
	: changed(false)
{
	static_assert(std::is_pointer<ComboBox>::value, "TImageTypesConfig template parameter must be a pointer.");

	// Resize the v_sysData vector to handle all supported systems.
	const int imageTypes = ImageTypesConfig::imageTypeCount();
	v_sysData.resize(ImageTypesConfig::sysCount());
	for (auto &sys : v_sysData) {
		// Resize the image types arrays.
		sys.cboImageType.resize(imageTypes);
		sys.imageTypes.resize(imageTypes);
		std::fill(sys.imageTypes.begin(), sys.imageTypes.end(), 0xFF);
	}
}

/**
 * Create the grid of labels and ComboBoxes.
 */
template<typename ComboBox>
void TImageTypesConfig<ComboBox>::createGrid(void)
{
	// TODO: Make sure the grid wasn't already created.

	// Create the grid labels.
	createGridLabels();

	// NOTE: These should match v_sysData.
	const unsigned int sysCount = ImageTypesConfig::sysCount();
	const unsigned int imageTypeCount = ImageTypesConfig::imageTypeCount();
	assert(sysCount == static_cast<unsigned int>(v_sysData.size()));

	// Create the ComboBoxes.
	for (unsigned int sys = 0; sys < sysCount; sys++) {
		SysData_t &sysData = v_sysData[sys];

		// Get supported image types.
		uint32_t imgbf = ImageTypesConfig::supportedImageTypes(sys);
		assert(imgbf != 0);

		uint32_t validImageTypes = 0;
		for (unsigned int imageType = 0; imgbf != 0 && imageType < imageTypeCount; imageType++, imgbf >>= 1) {
			if (!(imgbf & 1)) {
				// Current image type is not supported.
				continue;
			}

			// Create the ComboBox.
			createComboBox(sysAndImageTypeToCbid(sys, imageType));
			// Increment the valid image types counter.
			validImageTypes++;
		}
		sysData.validImageTypes = validImageTypes;

		// Add strings to the ComboBoxes.
		assert(imageTypeCount == static_cast<unsigned int>(sysData.cboImageType.size()));
		for (unsigned int imageType = 0; imageType < imageTypeCount; imageType++) {
			if (sysData.cboImageType[imageType] != nullptr) {
				addComboBoxStrings(sysAndImageTypeToCbid(sys, imageType), validImageTypes);
			}
		}

		// ComboBox finalization, if necessary.
		finishComboBoxes();

		// Initial image types configuration is empty.
		std::fill(sysData.imageTypes.begin(), sysData.imageTypes.end(), 0xFF);
	}

	// Load the configuration.
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
	using LibRpBase::Config;

	bool hasChanged = false;

	// CBID map of ComboBoxes that have had a priority set.
	// NOTE: Will need expansion if either sysCount or imageTypeCount exceed 4 bits.
	std::array<bool, 256> cbid_needsReset;
	cbid_needsReset.fill(true);

	const Config *const config = Config::instance();
	Config::ImgTypePrio_t imgTypePrio;
	if (loadDefaults) {
		// Use the default image priority for all types.
		for (SysData_t &sysData : v_sysData) {
			sysData.sysIsDefault = true;
		}
		config->getDefImgTypePrio(&imgTypePrio);
	}

	// Keep track of image types set for each system.
	// Elements are set to true once an image type priority is read.
	// This vector is cleared to false after iterating over each system.
	// NOTE: Not using vector<bool>.
	const unsigned int imageTypeCount = ImageTypesConfig::imageTypeCount();
	std::vector<uint8_t> imageTypeSet;
	imageTypeSet.resize(imageTypeCount);

	const unsigned int sysCount = ImageTypesConfig::sysCount();
	for (unsigned int sys = 0; sys < sysCount; sys++) {
		SysData_t &sysData = v_sysData[sys];

		if (!loadDefaults) {
			// Get the image priority.
			const char *const className = ImageTypesConfig::className(sys);
			Config::ImgTypeResult res = config->getImgTypePrio(className, &imgTypePrio);
			bool no_thumbs = false;
			switch (res) {
				case Config::ImgTypeResult::Success:
					// Image type priority received successfully.
					sysData.sysIsDefault = false;
					break;
				case Config::ImgTypeResult::SuccessDefaults:
					// Image type priority received successfully.
					// ImgTypeResult::SuccessDefaults indicates the returned
					// data is the default priority, since a custom configuration
					// was not found for this class.
					sysData.sysIsDefault = true;
					break;
				case Config::ImgTypeResult::Disabled:
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
		std::fill(imageTypeSet.begin(), imageTypeSet.end(), false);

		for (unsigned int i = 0; i < imgTypePrio.length && nextPrio <= sysData.validImageTypes; i++) {
			uint8_t imageType = imgTypePrio.imgTypes[i];
			assert(imageType < imageTypeCount);
			if (imageType >= imageTypeCount) {
				// Invalid image type.
				// NOTE: 0xFF (no image) should not be encountered here.
				continue;
			}

			if (sysData.cboImageType[imageType] && !imageTypeSet[imageType]) {
				// Set the image type.
				imageTypeSet[imageType] = true;
				if (sysData.imageTypes[imageType] != nextPrio) {
					sysData.imageTypes[imageType] = nextPrio;
					hasChanged = true;

					// NOTE: Using the actual priority value, not the ComboBox index.
					cboImageType_setPriorityValue(sysAndImageTypeToCbid(sys, imageType), nextPrio);
				}
				const unsigned int cbid = sysAndImageTypeToCbid(sys, imageType);
				cbid_needsReset[cbid] = false;
				nextPrio++;
			}
		}
	}

	// Set ComboBoxes that don't have a priority to "No".
	for (unsigned int sys = 0; sys < sysCount; sys++) {
		const SysData_t &sysData = v_sysData[sys];
		unsigned int cbid = sysAndImageTypeToCbid(sys, 0);
		for (unsigned int imageType = 0; imageType < imageTypeCount; imageType++, cbid++) {
			if (cbid_needsReset[cbid] && sysData.cboImageType[imageType]) {
				if (sysData.imageTypes[imageType] != 0xFF) {
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

	const unsigned int sysCount = ImageTypesConfig::sysCount();
	const unsigned int imageTypeCount = ImageTypesConfig::imageTypeCount();

	// Format of imageTypes[]:
	// - Index: Image type.
	// - Value: Priority.
	// We need to swap index and value.
	std::vector<uint8_t> imgTypePrio;
	imgTypePrio.resize(imageTypeCount);

	std::string imageTypeList;
	imageTypeList.reserve(128);	// TODO: Optimal reservation?
	for (unsigned int sys = 0; sys < sysCount; sys++) {
		const SysData_t &sysData = v_sysData[sys];
		const char *const className = ImageTypesConfig::className(sys);

		// Is this system using the default configuration?
		if (sysData.sysIsDefault) {
			// Default configuration. Write an empty string.
			ret = saveWriteEntry(className, "");
			if (ret != 0) {
				// Error...
				saveFinish();
				return ret;
			}
			continue;
		}

		// Clear the imageTypeList.
		imageTypeList.clear();
		std::fill(imgTypePrio.begin(), imgTypePrio.end(), 0xFF);
		assert(imageTypeCount == static_cast<unsigned int>(sysData.imageTypes.size()));
		for (unsigned int imageType = 0; imageType < imageTypeCount; imageType++) {
			const unsigned int imgt = sysData.imageTypes[imageType];
			if (imgt >= imageTypeCount) {
				// Image type is either not valid for this system
				// or is set to "No".
				continue;
			}
			imgTypePrio[imgt] = imageType;
		}

		// Convert the image type priority to strings.
		// TODO: Export the string data from Config.
		static constexpr std::array<const char*, 10> conf_imageTypeNames = {{
			"IntIcon",
			"IntBanner",
			"IntMedia",
			"IntImage",
			"ExtMedia",
			"ExtCover",
			"ExtCover3D",
			"ExtCoverFull",
			"ExtBox",
			"ExtTitleScreen",
		}};
		static_assert(conf_imageTypeNames.size() == LibRpBase::RomData::IMG_EXT_MAX+1, "conf_imageTypeNames[] is out of sync!");

		bool hasOne = false;
		for (uint8_t imageType : imgTypePrio) {
			if (imageType < imageTypeCount) {
				if (hasOne)
					imageTypeList += ',';
				hasOne = true;
				imageTypeList += conf_imageTypeNames[imageType];
			}
		}

		if (hasOne) {
			// At least one image type is enabled.
			ret = saveWriteEntry(className, imageTypeList.c_str());
		} else {
			// All image types are disabled.
			ret = saveWriteEntry(className, "No");
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
 * @return True if changed; false if not.
 */
template<typename ComboBox>
bool TImageTypesConfig<ComboBox>::cboImageType_priorityValueChanged(unsigned int cbid, unsigned int prio)
{
	const unsigned int sys = sysFromCbid(cbid);
	const unsigned int imageType = imageTypeFromCbid(cbid);
	if (!validateSysImageType(sys, imageType))
		return false;

	SysData_t &sysData = v_sysData[sys];
	const uint8_t prev_prio = sysData.imageTypes[imageType];
	if (prev_prio == prio) {
		// No change.
		return false;
	}

	if (prio >= 0 && prio != 0xFF) {
		// Check for any image types that have the new priority.
		const unsigned int imageTypeCount = ImageTypesConfig::imageTypeCount();
		for (unsigned int i = 0; i < imageTypeCount; i++) {
			if (i == imageType)
				continue;
			if (sysData.cboImageType[i] != nullptr && sysData.imageTypes[i] == static_cast<uint8_t>(prio)) {
				// Found a match! Swap the priority.
				sysData.imageTypes[i] = prev_prio;
				cboImageType_setPriorityValue(sysAndImageTypeToCbid(sys, i), prev_prio);
				break;
			}
		}
	}

	// Save the image priority value.
	sysData.imageTypes[imageType] = prio;
	// Mark this configuration as no longer being default.
	sysData.sysIsDefault = false;
	// Configuration has been changed.
	changed = true;
	return true;
}

}
