/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * VkEnumStrings.cpp: Vulkan string tables.                                *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "VkEnumStrings.hpp"
#include "fileformat/vk_defs.h"

// C++ STL classes
#include <algorithm>
#include <array>
using std::array;

namespace LibRpTexture { namespace VkEnumStrings {

#include "vkEnum_base_data.h"
#include "vkEnum_1000156xxx_data.h"
#include "vkEnum_1000330xxx_data.h"
#include "vkEnum_1000340xxx_data.h"
#include "vkEnum_1000470xxx_data.h"
#include "vkEnum_PVRTC_data.h"
#include "vkEnum_ASTC_data.h"
#include "vkEnum_ASTC_3D_data.h"
#include "vkEnum_1000460xxx_data.h"
#include "vkEnum_1000609xxx_data.h"

/** Public functions **/

/**
 * Look up a Vulkan VkFormat enum string.
 * @param vkFormat	[in] vkFormat enum
 * @return String, or nullptr if not found.
 */
const char *lookup_vkFormat(unsigned int vkFormat)
{
	struct VkFormatTbl_t {
		unsigned int vk_min;	// Minimum Vulkan format (inclusive)
		unsigned int vk_max;	// Maximum Vulkan format (inclusive)
		const char *strtbl;	// String table pointer
		const uint16_t *offtbl;	// Offset table pointer
	};

	static const array<VkFormatTbl_t, 10> vkFormatTbl = {{
		{VK_FORMAT_UNDEFINED, VK_FORMAT_ASTC_12x12_SRGB_BLOCK, vkEnum_base_strtbl, vkEnum_base_offtbl},
		{VK_FORMAT_G8B8G8R8_422_UNORM, VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM, vkEnum_1000156xxx_strtbl, vkEnum_1000156xxx_offtbl},
		{VK_FORMAT_G8_B8R8_2PLANE_444_UNORM, VK_FORMAT_G16_B16R16_2PLANE_444_UNORM, vkEnum_1000330xxx_strtbl, vkEnum_1000330xxx_offtbl},
		{VK_FORMAT_A4R4G4B4_UNORM_PACK16, VK_FORMAT_A4B4G4R4_UNORM_PACK16, vkEnum_1000340xxx_strtbl, vkEnum_1000340xxx_offtbl},
		{VK_FORMAT_A1B5G5R5_UNORM_PACK16, VK_FORMAT_A8_UNORM, vkEnum_1000470xxx_strtbl, vkEnum_1000470xxx_offtbl},
		{VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG, VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG, vkEnum_PVRTC_strtbl, vkEnum_PVRTC_offtbl},
		{VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK, VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK, vkEnum_ASTC_strtbl, vkEnum_ASTC_offtbl},
		{VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT, VK_FORMAT_ASTC_6x6x6_SFLOAT_BLOCK_EXT, vkEnum_ASTC_3D_strtbl, vkEnum_ASTC_3D_offtbl},
		{VK_FORMAT_R8_BOOL_ARM, VK_FORMAT_R8_SFLOAT_FPENCODING_FLOAT8E5M2_ARM, vkEnum_1000460xxx_strtbl, vkEnum_1000460xxx_offtbl},
		{VK_FORMAT_R10X6_UINT_PACK16_ARM, VK_FORMAT_G14X2_B14X2R14X2_2PLANE_422_UNORM_3PACK16_ARM, vkEnum_1000609xxx_strtbl, vkEnum_1000609xxx_offtbl},
	}};

	// TODO: Binary search? (Need to ensure vkFormatTbl is sorted...)
	// NOTE: If switching to binary search, check the base table first regardless.
	// Alternatively, make a single giant binary search string table instead of
	// the several individual tables...
	auto iter = std::find_if(vkFormatTbl.cbegin(), vkFormatTbl.cend(),
		[vkFormat](const VkFormatTbl_t &p) {
			return (p.vk_min <= vkFormat && vkFormat <= p.vk_max);
		});
	if (iter != vkFormatTbl.cend()) {
		return &iter->strtbl[iter->offtbl[vkFormat - iter->vk_min]];
	}

	// Not in the table. Check for special cases.
	if (vkFormat == VK_FORMAT_R16G16_SFIXED5_NV) {
		return "R16G16_SFIXED5_NV";
	}

	// Unknown format.
	return nullptr;
}

} }
