/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * DX10Formats.cpp: DirectX 10 formats.                                    *
 *                                                                         *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
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
		 * DirectX 10 format string table.
		 */
		static const char dxgiFormat_strtbl[];

		/**
		 * DirectX 10 format offset table.
		 * 0 == no string
		 */
		static const uint16_t dxgiFormat_offtbl[];
};

/**
 * DirectX 10 format table.
 * NOTE: Only contains the contiguous section.
 * Other values have to be checked afterwards.
 */
const char DX10FormatsPrivate::dxgiFormat_strtbl[] = {
	// 0
	"\0" "R32G32B32A32_TYPELESS\0"
	"R32G32B32A32_FLOAT\0" "R32G32B32A32_UINT\0"
	"R32G32B32A32_SINT\0" "R32G32B32_TYPELESS\0"
	"R32G32B32_FLOAT\0" "R32G32B32_UINT\0"
	"R32G32B32_SINT\0" "R16G16B16A16_TYPELESS\0"

	// 10
	"R16G16B16A16_FLOAT\0" "R16G16B16A16_UNORM\0"
	"R16G16B16A16_UINT\0" "R16G16B16A16_SNORM\0"
	"R16G16B16A16_SINT\0" "R32G32_TYPELESS\0"
	"R32G32_FLOAT\0" "R32G32_UINT\0"
	"R32G32_SINT\0" "R32G8X24_TYPELESS\0"

	// 20
	"D32_FLOAT_S8X24_UINT\0" "R32_FLOAT_X8X24_TYPELESS\0"
	"X32_TYPELESS_G8X24_UINT\0" "R10G10B10A2_TYPELESS\0"
	"R10G10B10A2_UNORM\0" "R10G10B10A2_UINT\0"
	"R11G11B10_FLOAT\0" "R8G8B8A8_TYPELESS\0"
	"R8G8B8A8_UNORM\0" "R8G8B8A8_UNORM_SRGB\0"

	// 30
	"R8G8B8A8_UINT\0" "R8G8B8A8_SNORM\0"
	"R8G8B8A8_SINT\0" "R16G16_TYPELESS\0"
	"R16G16_FLOAT\0" "R16G16_UNORM\0"
	"R16G16_UINT\0" "R16G16_SNORM\0"
	"R16G16_SINT\0" "R32_TYPELESS\0"

	// 40
	"D32_FLOAT\0" "R32_FLOAT\0"
	"R32_UINT\0" "R32_SINT\0"
	"R24G8_TYPELESS\0" "D24_UNORM_S8_UINT\0"
	"R24_UNORM_X8_TYPELESS\0" "X24_TYPELESS_G8_UINT\0"
	"R8G8_TYPELESS\0" "R8G8_UNORM\0"

	// 50
	"R8G8_UINT\0" "R8G8_SNORM\0"
	"R8G8_SINT\0" "R16_TYPELESS\0"
	"R16_FLOAT\0" "D16_UNORM\0"
	"R16_UNORM\0" "R16_UINT\0"
	"R16_SNORM\0" "R16_SINT\0"

	// 60
	"R8_TYPELESS\0" "R8_UNORM\0"
	"R8_UINT\0" "R8_SNORM\0"
	"R8_SINT\0" "A8_UNORM\0"
	"R1_UNORM\0" "R9G9B9E5_SHAREDEXP\0"
	"R8G8_B8G8_UNORM\0" "G8R8_G8B8_UNORM\0"

	// 70
	"BC1_TYPELESS\0" "BC1_UNORM\0"
	"BC1_UNORM_SRGB\0" "BC2_TYPELESS\0"
	"BC2_UNORM\0" "BC2_UNORM_SRGB\0"
	"BC3_TYPELESS\0" "BC3_UNORM\0"
	"BC3_UNORM_SRGB\0" "BC4_TYPELESS\0"

	// 80
	"BC4_UNORM\0" "BC4_SNORM\0"
	"BC5_TYPELESS\0" "BC5_UNORM\0"
	"BC5_SNORM\0" "B5G6R5_UNORM\0"
	"B5G5R5A1_UNORM\0" "B8G8R8A8_UNORM\0"
	"B8G8R8X8_UNORM\0" "R10G10B10_XR_BIAS_A2_UNORM\0"

	// 90
	"B8G8R8A8_TYPELESS\0" "B8G8R8A8_UNORM_SRGB\0"
	"B8G8R8X8_TYPELESS\0" "B8G8R8X8_UNORM_SRGB\0"
	"BC6H_TYPELESS\0" "BC6H_UF16\0"
	"BC6H_SF16\0" "BC7_TYPELESS\0"
	"BC7_UNORM\0" "BC7_UNORM_SRGB\0"

	// 100
	"AYUV\0" "Y410\0" "Y416\0" "NV12\0"
	"P010\0" "P016\0" "420_OPAQUE\0" "YUY2\0"
	"Y210\0" "Y216\0" "NV11\0" "AI44\0"
	"IA44\0" "P8\0" "A8P8\0" "B4G4R4A4_UNORM\0"

	// 116
	"XBOX_R10G10B10_7E2_A2_FLOAT\0"
	"XBOX_R10G10B10_6E4_A2_FLOAT\0"
	"XBOX_D16_UNORM_S8_UINT\0" "XBOX_R6_UNORM_X8_TYPELESS\0"

	// 120
	"XBOX_X16_TYPELESS_G8_UINT\0"

	// 130
	"P208\0" "V208\0" "V408\0"

	// ASTC (133-188)
	#define DXGI_ASTC(x,y) \
		"ASTC_" #x "X" #x "_TYPELESS\0" \
		"ASTC_" #x "X" #x "_UNORM\0" \
		"ASTC_" #x "X" #x "_UNORM_SRGB\0"
	DXGI_ASTC(4, 4)
	DXGI_ASTC(5, 4)
	DXGI_ASTC(5, 5)
	DXGI_ASTC(6, 5)
	DXGI_ASTC(6, 6)
	DXGI_ASTC(8, 5)
	DXGI_ASTC(8, 6)
	DXGI_ASTC(8, 8)
	DXGI_ASTC(10, 5)
	DXGI_ASTC(10, 6)
	DXGI_ASTC(10, 8)
	DXGI_ASTC(10, 10)
	DXGI_ASTC(12, 10)
	DXGI_ASTC(12, 12)

	// Additional Xbox One formats
	// 189
	"XBOX_R10G10B10_SNORM_A2_UNORM\0"
	"XBOX_R4G4_UNORM\0"
};

/**
 * DirectX 10 format offset table.
 * 0 == no string
 */
const uint16_t DX10FormatsPrivate::dxgiFormat_offtbl[] = {
	// 0
	0, 1, 23, 42, 60, 78, 97, 113, 128, 143,
	// 10
	165, 184, 203, 221, 240, 258, 274, 287, 299, 311,
	// 20
	329, 350, 375, 399, 420, 438, 455, 471, 489, 504,
	// 30
	524, 538, 553, 567, 583, 596, 609, 621, 634, 646,
	// 40
	659, 669, 679, 688, 697, 712, 730, 752, 773, 787,
	// 50
	798, 808, 819, 829, 842, 852, 862, 872, 881, 891,
	// 60
	900, 912, 921, 929, 938, 946, 955, 964, 983, 999,
	// 70
	1015, 1028, 1038, 1053, 1066, 1076, 1091, 1104, 1114, 1129,
	// 80
	1142, 1152, 1162, 1175, 1185, 1195, 1208, 1223, 1238, 1253,
	// 90
	1280, 1298, 1318, 1336, 1356, 1370, 1380, 1390, 1403, 1413,
	// 100
	1428, 1433, 1438, 1443, 1448, 1453, 1458, 1469, 1474, 1479,
	// 110
	1484, 1489, 1494, 1499, 1502, 1507, 1522, 1550, 1578, 1601,
	// 120
	1627, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	// 130
	1653, 0, 0, 1668, 1691, 1711, 0, 1736, 1759, 1779,
	// 140
	0, 1804, 1827, 1847, 0, 1872, 1895, 1915, 0, 1940,
	// 150
	1963, 1983, 0, 2008, 2031, 2051, 0, 2076, 2099, 2119,
	// 160
	0, 2144, 2167, 2187, 0, 2212, 2236, 2257, 0, 2283,
	// 170
	2307, 2328, 0, 2354, 2378, 2399, 0, 2425, 2450, 2472,
	// 180
	0, 2499, 2524, 2546, 0, 2573, 2598, 2620, 0, 2647,
	// 190
	2677,
};

/** DX10Formats **/

/**
 * Look up a DirectX 10 format value.
 * @param dxgiFormat	[in] DXGI_FORMAT
 * @return String, or nullptr if not found.
 */
const char *DX10Formats::lookup_dxgiFormat(unsigned int dxgiFormat)
{
	static_assert(ARRAY_SIZE(DX10FormatsPrivate::dxgiFormat_offtbl) == XBOX_DXGI_FORMAT_R4G4_UNORM+1,
		"DX10FormatsPrivate::dxgiFormat_tbl[] size is incorrect.");

	const char *texFormat = nullptr;
	if (/*dxgiFormat >= 0 &&*/ dxgiFormat < ARRAY_SIZE(DX10FormatsPrivate::dxgiFormat_offtbl)) {
		unsigned int offset = DX10FormatsPrivate::dxgiFormat_offtbl[dxgiFormat];
		return (likely(offset != 0))
			? &DX10FormatsPrivate::dxgiFormat_strtbl[offset]
			: nullptr;
	} else {
		switch (dxgiFormat) {
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
