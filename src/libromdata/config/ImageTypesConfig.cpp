/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ImageTypesConfig.cpp: Image Types non-templated common functions.       *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ImageTypesConfig.hpp"
#include "librpbase/config/Config.hpp"

// librpbase
#include "librpbase/RomData.hpp"	// for IMG_* constants
using namespace LibRpBase;

// RomData subclasses with images.
// Does not include texture files, since those are always
// thumbnailed using IMG_INT_IMAGE.
#include "Other/Amiibo.hpp"
#include "Console/DreamcastSave.hpp"
#include "Console/GameCube.hpp"
#include "Console/GameCubeSave.hpp"
#include "Other/NintendoBadge.hpp"
#include "Handheld/NintendoDS.hpp"
#include "Handheld/Nintendo3DS.hpp"
#include "Console/PlayStationDisc.hpp"
#include "Console/PlayStationSave.hpp"
#include "Console/WiiU.hpp"
#include "Console/WiiWAD.hpp"

// C++ STL classes
using std::array;

namespace LibRomData { namespace ImageTypesConfig {

// Number of image types. (columns)
static constexpr unsigned int IMG_TYPE_COUNT = LibRpBase::RomData::IMG_EXT_MAX+1;
// Number of systems. (rows)
static constexpr unsigned int SYS_COUNT = 11;

namespace Private {

// NOTE: SysData_t is defined here because it causes
// problems if it's embedded inside of a templated class.
typedef uint32_t (*pFnSupportedImageTypes)(void);
struct SysData_t {
	const char *className;			// Class name in Config. (ASCII)
	pFnSupportedImageTypes getTypes;	// Get supported image types.
};
#define SysDataEntry(klass) \
	{#klass, LibRomData::klass::supportedImageTypes_static}

// System data.
static const array<SysData_t, SYS_COUNT> sysData = {{
	SysDataEntry(Amiibo),
	SysDataEntry(NintendoBadge),
	SysDataEntry(DreamcastSave),
	SysDataEntry(GameCube),
	SysDataEntry(GameCubeSave),
	SysDataEntry(NintendoDS),
	SysDataEntry(Nintendo3DS),
	SysDataEntry(PlayStationDisc),
	SysDataEntry(PlayStationSave),
	SysDataEntry(WiiU),
	SysDataEntry(WiiWAD),
}};

}

/**
 * Get the number of image types that can be configured.
 * @return Image type count.
 */
unsigned int imageTypeCount(void)
{
	return IMG_TYPE_COUNT;
}

/**
 * Get an image type name.
 * @param imageType Image type ID.
 * @return Image type name, or nullptr if invalid.
 */
const char *imageTypeName(unsigned int imageType)
{
	// Image type names.
	static const array<const char*, IMG_TYPE_COUNT> imageType_names = {{
		/** Internal **/

		// tr: IMG_INT_ICON
		NOP_C_("ImageTypesConfig|ImageTypeDisp", "Internal\nIcon"),
		// tr: IMG_INT_BANNER
		NOP_C_("ImageTypesConfig|ImageTypeDisp", "Internal\nBanner"),
		// tr: IMG_INT_MEDIA
		NOP_C_("ImageTypesConfig|ImageTypeDisp", "Internal\nMedia"),
		// tr: IMG_INT_IMAGE
		NOP_C_("ImageTypesConfig|ImageTypeDisp", "Internal\nImage"),

		/** External **/

		// tr: IMG_EXT_MEDIA
		NOP_C_("ImageTypesConfig|ImageTypeDisp", "External\nMedia"),
		// tr: IMG_EXT_COVER
		NOP_C_("ImageTypesConfig|ImageTypeDisp", "External\nCover"),
		// tr: IMG_EXT_COVER_3D
		NOP_C_("ImageTypesConfig|ImageTypeDisp", "External\n3D Cover"),
		// tr: IMG_EXT_COVER_FULL
		NOP_C_("ImageTypesConfig|ImageTypeDisp", "External\nFull Cover"),
		// tr: IMG_EXT_BOX
		NOP_C_("ImageTypesConfig|ImageTypeDisp", "External\nBox"),
		// tr: IMG_EXT_TITLE_SCREEN
		NOP_C_("ImageTypesConfig|ImageTypeDisp", "External\nTitle Screen"),
	}};

	assert(imageType < imageType_names.size());
	if (imageType >= imageType_names.size())
		return nullptr;
	return pgettext_expr("ImageTypesConfig|ImageTypeDisp", imageType_names[imageType]);
}

/**
 * Get the number of systems that can be configured.
 * @return System count.
 */
unsigned int sysCount(void)
{
	return SYS_COUNT;
}

/**
 * Get a system name.
 * @param sys System ID.
 * @return System name, or nullptr if invalid.
 */
const char *sysName(unsigned int sys)
{
	// System names.
	static const array<const char*, SYS_COUNT> sysNames = {{
		// tr: amiibo
		NOP_C_("ImageTypesConfig|SysName", "amiibo"),
		// tr: NintendoBadge
		NOP_C_("ImageTypesConfig|SysName", "Badge Arcade"),
		// tr: DreamcastSave
		NOP_C_("ImageTypesConfig|SysName", "Dreamcast Saves"),
		// tr: GameCube
		NOP_C_("ImageTypesConfig|SysName", "GameCube / Wii"),
		// tr: GameCubeSave
		NOP_C_("ImageTypesConfig|SysName", "GameCube Saves"),
		// tr: NintendoDS
		NOP_C_("ImageTypesConfig|SysName", "Nintendo DS(i)"),
		// tr: Nintendo3DS
		NOP_C_("ImageTypesConfig|SysName", "Nintendo 3DS"),
		// tr: PlayStationDisc (PS1 and PS2)
		NOP_C_("ImageTypesConfig|SysName", "PlayStation 1 & 2"),
		// tr: PlayStationSave
		NOP_C_("ImageTypesConfig|SysName", "PlayStation Saves"),
		// tr: WiiU
		NOP_C_("ImageTypesConfig|SysName", "Wii U"),
		// tr: WiiWAD
		NOP_C_("ImageTypesConfig|SysName", "Wii WAD Files"),
	}};

	assert(sys < sysNames.size());
	if (sys >= sysNames.size())
		return nullptr;
	return pgettext_expr("ImageTypesConfig|SysName", sysNames[sys]);
}

/**
 * Get a class name.
 * @param sys System ID
 * @return Class name, or nullptr if invalid.
 */
const char *className(unsigned int sys)
{
	assert(sys < SYS_COUNT);
	if (sys >= SYS_COUNT)
		return nullptr;
	return Private::sysData[sys].className;
}

/**
 * Get the supported image types for the specified system.
 * @param sys System ID
 * @return Supported image types, or 0 if invalid.
 */
uint32_t supportedImageTypes(unsigned int sys)
{
	assert(sys < SYS_COUNT);
	if (sys >= SYS_COUNT)
		return 0;
	return Private::sysData[sys].getTypes();
}

} }
