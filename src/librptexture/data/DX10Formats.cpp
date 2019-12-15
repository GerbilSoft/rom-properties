/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * DX10Formats.cpp: DirectX 10 formats.                                    *
 *                                                                         *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "DX10Formats.hpp"
#include "../fileformat/dds_structs.h"

namespace LibRpTexture {

class DX10FormatsPrivate
{
	private:
		// Static class.
		DX10FormatsPrivate();
		~DX10FormatsPrivate();
		RP_DISABLE_COPY(DX10FormatsPrivate)

	public:
		/**
		 * DirectX 10 format table.
		 * NOTE: Only contains the contiguous section.
		 * Other values have to be checked afterwards.
		 */
		static const char *const dxgiFormat_tbl[];
};

/**
 * DirectX 10 format table.
 * NOTE: Only contains the contiguous section.
 * Other values have to be checked afterwards.
 */
const char *const DX10FormatsPrivate::dxgiFormat_tbl[] = {
	nullptr, "R32G32B32A32_TYPELESS",			// 0,1
	"R32G32B32A32_FLOAT", "R32G32B32A32_UINT",		// 2,3
	"R32G32B32A32_SINT", "R32G32B32_TYPELESS",		// 4,5
	"R32G32B32_FLOAT", "R32G32B32_UINT",			// 6,7
	"R32G32B32_SINT", "R16G16B16A16_TYPELESS",		// 8,9
	"R16G16B16A16_FLOAT", "R16G16B16A16_UNORM",		// 10,11
	"R16G16B16A16_UINT", "R16G16B16A16_SNORM",		// 12,13
	"R16G16B16A16_SINT", "R32G32_TYPELESS",			// 14,15
	"R32G32_FLOAT", "R32G32_UINT",				// 16,17
	"R32G32_SINT", "R32G8X24_TYPELESS",			// 18,19
	"D32_FLOAT_S8X24_UINT", "R32_FLOAT_X8X24_TYPELESS",	// 20,21
	"X32_TYPELESS_G8X24_UINT", "R10G10B10A2_TYPELESS",	// 22,23
	"R10G10B10A2_UNORM", "R10G10B10A2_UINT",		// 24,25
	"R11G11B10_FLOAT", "R8G8B8A8_TYPELESS",			// 26,27
	"R8G8B8A8_UNORM", "R8G8B8A8_UNORM_SRGB",		// 28,29
	"R8G8B8A8_UINT", "R8G8B8A8_SNORM",			// 30,31
	"R8G8B8A8_SINT", "R16G16_TYPELESS",			// 32,33
	"R16G16_FLOAT", "R16G16_UNORM",				// 34,35
	"R16G16_UINT", "R16G16_SNORM",				// 36,37
	"R16G16_SINT", "R32_TYPELESS",				// 38,39
	"D32_FLOAT", "R32_FLOAT",				// 40,41
	"R32_UINT", "R32_SINT",					// 42,43
	"R24G8_TYPELESS", "D24_UNORM_S8_UINT",			// 44,45
	"R24_UNORM_X8_TYPELESS", "X24_TYPELESS_G8_UINT",	// 46,47
	"R8G8_TYPELESS", "R8G8_UNORM",				// 48,49
	"R8G8_UINT", "R8G8_SNORM",				// 50,51
	"R8G8_SINT", "R16_TYPELESS",				// 52,53
	"R16_FLOAT", "D16_UNORM",				// 54,55
	"R16_UNORM", "R16_UINT",				// 56,57
	"R16_SNORM", "R16_SINT",				// 58,59
	"R8_TYPELESS", "R8_UNORM",				// 60,61
	"R8_UINT", "R8_SNORM",					// 62,63
	"R8_SINT", "A8_UNORM",					// 64,65
	"R1_UNORM", "R9G9B9E5_SHAREDEXP",			// 66,67
	"R8G8_B8G8_UNORM", "G8R8_G8B8_UNORM",			// 68,69
	"BC1_TYPELESS", "BC1_UNORM",				// 70,71
	"BC1_UNORM_SRGB", "BC2_TYPELESS",			// 72,73
	"BC2_UNORM", "BC2_UNORM_SRGB",				// 74,75
	"BC3_TYPELESS", "BC3_UNORM",				// 76,77
	"BC3_UNORM_SRGB", "BC4_TYPELESS",			// 78,79
	"BC4_UNORM", "BC4_SNORM",				// 80,81
	"BC5_TYPELESS", "BC5_UNORM",				// 82,83
	"BC5_SNORM", "B5G6R5_UNORM",				// 84,85
	"B5G5R5A1_UNORM", "B8G8R8A8_UNORM",			// 86,87
	"B8G8R8X8_UNORM", "R10G10B10_XR_BIAS_A2_UNORM",		// 88,89
	"B8G8R8A8_TYPELESS", "B8G8R8A8_UNORM_SRGB",		// 90,91
	"B8G8R8X8_TYPELESS", "B8G8R8X8_UNORM_SRGB",		// 92,93
	"BC6H_TYPELESS", "BC6H_UF16",				// 94,95
	"BC6H_SF16", "BC7_TYPELESS",				// 96,97
	"BC7_UNORM", "BC7_UNORM_SRGB",				// 98,99
	"AYUV", "Y410", "Y416", "NV12",				// 100-103
	"P010", "P016", "420_OPAQUE", "YUY2",			// 104-107
	"Y210", "Y216", "NV11", "AI44",				// 108-111
	"IA44", "P8", "A8P8", "B4G4R4A4_UNORM",			// 112-115
	"XBOX_R10G10B10_7E2_A2_FLOAT",				// 116
	"XBOX_R10G10B10_6E4_A2_FLOAT",				// 117
	"XBOX_D16_UNORM_S8_UINT", "XBOX_R6_UNORM_X8_TYPELESS",	// 118-119
	"XBOX_DXGI_FORMAT_X16_TYPELESS_G8_UINT",		// 120
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,	// 121-126
	nullptr, nullptr, nullptr,				// 127-129
	"P208", "V208", "V408",					// 130-132
};

/** DX10Formats **/

/**
 * Look up a DirectX 10 format value.
 * @param dxgiFormat	[in] DXGI_FORMAT
 * @return String, or nullptr if not found.
 */
const char *DX10Formats::lookup_dxgiFormat(unsigned int dxgiFormat)
{
	static_assert(ARRAY_SIZE(DX10FormatsPrivate::dxgiFormat_tbl) == DXGI_FORMAT_V408+1,
		"DX10FormatsPrivate::dxgiFormat_tbl[] size is incorrect.");

	const char *texFormat = nullptr;
	if (/*dxgiFormat >= 0 &&*/ dxgiFormat < ARRAY_SIZE(DX10FormatsPrivate::dxgiFormat_tbl)) {
		texFormat = DX10FormatsPrivate::dxgiFormat_tbl[dxgiFormat];
	} else {
		switch (dxgiFormat) {
			case XBOX_DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM:
				texFormat = "XBOX_R10G10B10_SNORM_A2_UNORM";
				break;
			case XBOX_DXGI_FORMAT_R4G4_UNORM:
				texFormat = "XBOX_R4G4_UNORM";
				break;
			case DXGI_FORMAT_FORCE_UINT:
				texFormat = "FORCE_UINT";
				break;

			// FAKE formats.
			// These aren't used by actual DX10 DDSes, but *are* used
			// internally by rom-properties for some FourCCs that don't
			// have corresponding DXGI_FORMAT values.
			case DXGI_FORMAT_FAKE_PVRTC_2bpp:
				texFormat = "PVRTC 2bpp RGBA";
				break;
			case DXGI_FORMAT_FAKE_PVRTC_4bpp:
				texFormat = "PVRTC 4bpp RGBA";
				break;

			default:
				break;
		}
	}

	return texFormat;
}

}
