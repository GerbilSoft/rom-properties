/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GM.cpp: GameMaker IFF/"data.win" header reader                          *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * Copyright (c) 2026 by Emma / InvoxiPlayGames.                           *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "GMS.hpp"
#include "RomData_p.hpp"

#include "gms_structs.h"

#include <cstdint>

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

class GMSPrivate final : public RomDataPrivate
{
public:
	explicit GMSPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(GMSPrivate)

	int readNullTerminatedString(uint32_t offset, std::string &str);

public:
	/** RomDataInfo **/
	static const array<const char*, 6+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

    // ROM header
    bool isBigEndian;
    int dataVersion;
    union {
        YYHeader v9;
        YYHeader_000A v10;
        YYHeader_000B v11;
        YYHeader_000C v12;
        YYHeader_000D v13;
        YYHeader_000E v14;
    } header;
    vector<int> roomOrder;

	bool hasGms2Header;
    uint8_t gms2License[0x28];
    YYGMS2HeaderData gms2Header;

	// parsed information
    std::string projectName;
    std::string projectConfig;
    std::string gameName;
    std::string displayName;
};

ROMDATA_IMPL(GMS)

const array<const char*, 6+1> GMSPrivate::exts = {{
	".win",
	".ios",
    ".unx",
    ".droid",
    ".3ds",
    ".symbian",
	nullptr
}};

const array<const char*, 1+1> GMSPrivate::mimeTypes = {{
	"application/x-iff",
	nullptr
}};

const RomDataInfo GMSPrivate::romDataInfo = {
	"GameMaker", exts.data(), mimeTypes.data()
};

int GMSPrivate::readNullTerminatedString(uint32_t offset, std::string &str)
{
	if (offset == 0)
		return 0;

	if (file->seek(offset) != 0)
		return -1; // failed to seek
	// really bad, loop a read of a single byte til we reach a NULL
	char c = 0;
	do {
		if (file->read(&c, sizeof(c)) != sizeof(c))
			return -2; // failed to read
		if (c != 0)
			str.push_back(c);
	} while (c != 0);
	return 0;
}

GMSPrivate::GMSPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	isBigEndian = false;
	dataVersion = 0;
	memset(&header, 0, sizeof(header));
	hasGms2Header = false;
	memset(&gms2License, 0, sizeof(gms2License));
	memset(&gms2Header, 0, sizeof(gms2Header));
}

typedef enum _GMSFileType {
    GMSFileType_Invalid = -1,
    GMSFileType_LE = 0,
    GMSFileType_BE = 1
} GMSFileType;

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GMS::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < (sizeof(iff_sect_hdr_t) * 2))
	{
		return GMSFileType_Invalid;
	}

    bool isBigEndian = false;

    // Check the IFF container header to make sure it's a FORM header
	const iff_sect_hdr_t *const formheader =
		reinterpret_cast<const iff_sect_hdr_t*>(info->header.pData);
    if (be32_to_cpu(formheader->magic) == FORM_HDR) {
        isBigEndian = true;
    } else if (le32_to_cpu(formheader->magic) != FORM_HDR) {
        return GMSFileType_Invalid; // not a valid file
    }
    // Validate the length
    uint32_t formLength = isBigEndian ? be32_to_cpu(formheader->length) : le32_to_cpu(formheader->length);
    if (formLength < (sizeof(iff_sect_hdr_t) + sizeof(YYHeader))) {
        return GMSFileType_Invalid; // not a valid length
    }

    // Check the data after the FORM header to make sure the first value is the GEN8 info header
	const iff_sect_hdr_t *const infoheader =
		reinterpret_cast<const iff_sect_hdr_t*>(info->header.pData + sizeof(iff_sect_hdr_t));
	if (isBigEndian == false &&
        le32_to_cpu(infoheader->magic) == GEN8_HDR &&
        le32_to_cpu(infoheader->length) >= sizeof(YYHeader)) {
        return GMSFileType_LE; // little endian
    } else if (isBigEndian &&
                be32_to_cpu(infoheader->magic) == GEN8_HDR &&
                be32_to_cpu(infoheader->length) >= sizeof(YYHeader)) {
        return GMSFileType_BE;
	} else {
		return GMSFileType_Invalid;
    }
}

/**
 * Read a GameMaker asset file.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
GMS::GMS(const IRpFilePtr &file)
	: super(new GMSPrivate(file))
{
	RP_D(GMS);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the IFF header (2 to get FORM container and GEN8 header)
	uint8_t iffheader_bytes[sizeof(iff_sect_hdr_t) * 2];
	d->file->rewind();
	size_t size = d->file->read(iffheader_bytes, sizeof(iffheader_bytes));
	if (size != sizeof(iffheader_bytes)) {
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(iffheader_bytes), iffheader_bytes},
		nullptr,
		0
	};
    int romSupport = isRomSupported_static(&info);
	if (romSupport >= 0) {
        d->isValid = true;
        d->isBigEndian = romSupport == GMSFileType_BE;
    } else {
        d->isValid = false;
    }

    // Read out the start of the info header
    int infoHeaderInt = 0;
    size = d->file->read(&infoHeaderInt, sizeof(infoHeaderInt));
    if (size != sizeof(infoHeaderInt)) {
	    d->isValid = false;
	    d->file.reset();
	    return;
    }
    infoHeaderInt = d->isBigEndian ? be32_to_cpu(infoHeaderInt) : le32_to_cpu(infoHeaderInt);
	// Get the version of the file
	d->dataVersion = (infoHeaderInt >> 8) & 0xFF;
    d->header.v9.debug = infoHeaderInt;
	// TODO: clean this up! remove duplicated code
	if (d->dataVersion < 0xA) {
	    size = d->file->read(&d->header.v9.pName, sizeof(YYHeader) - 4);
		if (size != sizeof(YYHeader) - 4) {
			d->isValid = false;
			d->file.reset();
			return;
		}
	} else if (d->dataVersion == 0xA) {
		size = d->file->read(&d->header.v10.pName, sizeof(YYHeader_000A) - 4);
		if (size != sizeof(YYHeader_000A) - 4) {
			d->isValid = false;
			d->file.reset();
			return;
		}
	} else if (d->dataVersion == 0xB) {
		size = d->file->read(&d->header.v11.pName, sizeof(YYHeader_000B) - 4);
		if (size != sizeof(YYHeader_000B) - 4) {
			d->isValid = false;
			d->file.reset();
			return;
		}
	} else if (d->dataVersion == 0xC) {
		size = d->file->read(&d->header.v12.pName, sizeof(YYHeader_000C) - 4);
		if (size != sizeof(YYHeader_000C) - 4) {
			d->isValid = false;
			d->file.reset();
			return;
		}
	} else if (d->dataVersion == 0xD) {
		size = d->file->read(&d->header.v13.pName, sizeof(YYHeader_000D) - 4);
		if (size != sizeof(YYHeader_000D) - 4) {
			d->isValid = false;
			d->file.reset();
			return;
		}
	} else if (d->dataVersion >= 0xE) {
		size = d->file->read(&d->header.v14.pName, sizeof(YYHeader_000E) - 4);
		if (size != sizeof(YYHeader_000E) - 4) {
			d->isValid = false;
			d->file.reset();
			return;
		}
	}

	// Byteswap the header if necessary
	if (d->isBigEndian) {
		//d->header.v9.debug = be32_to_cpu(d->header.v9.debug);
		d->header.v9.pName = be32_to_cpu(d->header.v9.pName);
		d->header.v9.pConfig = be32_to_cpu(d->header.v9.pConfig);
		d->header.v9.roomMaxId = be32_to_cpu(d->header.v9.roomMaxId);
		d->header.v9.roomMaxTileId = be32_to_cpu(d->header.v9.roomMaxTileId);
		d->header.v9.id = be32_to_cpu(d->header.v9.id);
		d->header.v9.buildNumber = be32_to_cpu(d->header.v9.buildNumber);
		d->header.v9.revisionNumber = be32_to_cpu(d->header.v9.revisionNumber);
		d->header.v9.guid3 = be32_to_cpu(d->header.v9.guid3);
		d->header.v9.guid4 = be32_to_cpu(d->header.v9.guid4);
		d->header.v9.pGameName = be32_to_cpu(d->header.v9.pGameName);
		d->header.v9.MajorVersion = be32_to_cpu(d->header.v9.MajorVersion);
		d->header.v9.MinorVersion = be32_to_cpu(d->header.v9.MinorVersion);
		d->header.v9.ReleaseVersion = be32_to_cpu(d->header.v9.ReleaseVersion);
		d->header.v9.BuildVersion = be32_to_cpu(d->header.v9.BuildVersion);
		d->header.v9.xscreensize = be32_to_cpu(d->header.v9.xscreensize);
		d->header.v9.yscreensize = be32_to_cpu(d->header.v9.yscreensize);
		d->header.v9.screenflags = be32_to_cpu(d->header.v9.screenflags);
		d->header.v9.crc = be32_to_cpu(d->header.v9.crc);
		d->header.v9.datetimeUTC = be64_to_cpu(d->header.v9.datetimeUTC);
		if (d->dataVersion >= 0xA)
			d->header.v10.pDisplayName = be32_to_cpu(d->header.v10.pDisplayName);
		if (d->dataVersion >= 0xB)
			d->header.v11.Licensed = be64_to_cpu(d->header.v11.Licensed);
		if (d->dataVersion >= 0xC)
			d->header.v12.functionClasses = be64_to_cpu(d->header.v12.functionClasses);
		if (d->dataVersion >= 0xD)
			d->header.v13.steamAppId = be32_to_cpu(d->header.v13.steamAppId);
		if (d->dataVersion >= 0xE)
			d->header.v14.debuggerServerPort = be32_to_cpu(d->header.v14.debuggerServerPort);
	} else {
		//d->header.v9.debug = be32_to_cpu(d->header.v9.debug);
		d->header.v9.pName = le32_to_cpu(d->header.v9.pName);
		d->header.v9.pConfig = le32_to_cpu(d->header.v9.pConfig);
		d->header.v9.roomMaxId = le32_to_cpu(d->header.v9.roomMaxId);
		d->header.v9.roomMaxTileId = le32_to_cpu(d->header.v9.roomMaxTileId);
		d->header.v9.id = le32_to_cpu(d->header.v9.id);
		d->header.v9.buildNumber = le32_to_cpu(d->header.v9.buildNumber);
		d->header.v9.revisionNumber = le32_to_cpu(d->header.v9.revisionNumber);
		d->header.v9.guid3 = le32_to_cpu(d->header.v9.guid3);
		d->header.v9.guid4 = le32_to_cpu(d->header.v9.guid4);
		d->header.v9.pGameName = le32_to_cpu(d->header.v9.pGameName);
		d->header.v9.MajorVersion = le32_to_cpu(d->header.v9.MajorVersion);
		d->header.v9.MinorVersion = le32_to_cpu(d->header.v9.MinorVersion);
		d->header.v9.ReleaseVersion = le32_to_cpu(d->header.v9.ReleaseVersion);
		d->header.v9.BuildVersion = le32_to_cpu(d->header.v9.BuildVersion);
		d->header.v9.xscreensize = le32_to_cpu(d->header.v9.xscreensize);
		d->header.v9.yscreensize = le32_to_cpu(d->header.v9.yscreensize);
		d->header.v9.screenflags = le32_to_cpu(d->header.v9.screenflags);
		d->header.v9.crc = le32_to_cpu(d->header.v9.crc);
		d->header.v9.datetimeUTC = le64_to_cpu(d->header.v9.datetimeUTC);
		if (d->dataVersion >= 0xA)
			d->header.v10.pDisplayName = le32_to_cpu(d->header.v10.pDisplayName);
		if (d->dataVersion >= 0xB)
			d->header.v11.Licensed = le64_to_cpu(d->header.v11.Licensed);
		if (d->dataVersion >= 0xC)
			d->header.v12.functionClasses = le64_to_cpu(d->header.v12.functionClasses);
		if (d->dataVersion >= 0xD)
			d->header.v13.steamAppId = le32_to_cpu(d->header.v13.steamAppId);
		if (d->dataVersion >= 0xE)
			d->header.v14.debuggerServerPort = le32_to_cpu(d->header.v14.debuggerServerPort);
	}
	
	// load room order from the file
	int roomOrderLength = 0;
	size = d->file->read(&roomOrderLength, sizeof(roomOrderLength));
	if (size != sizeof(roomOrderLength)) {
		d->isValid = false;
		d->file.reset();
		return;
	}
	roomOrderLength = d->isBigEndian ? be32_to_cpu(roomOrderLength) : le32_to_cpu(roomOrderLength);

	for (int i = 0; i < roomOrderLength; i++) {
		int roomId = 0;
		size = d->file->read(&roomId, sizeof(roomId));
		if (size != sizeof(roomId)) {
			d->isValid = false;
			d->file.reset();
			return;
		}
		roomId = d->isBigEndian ? be32_to_cpu(roomId) : le32_to_cpu(roomId);
		d->roomOrder.push_back(roomId);
	}

	// load the GMS2 header information
	if (d->dataVersion >= 0xE && d->header.v9.MajorVersion >= 2) {
		d->hasGms2Header = true;
		// license blob
		size = d->file->read(d->gms2License, sizeof(d->gms2License));
		if (size != sizeof(d->gms2License)) {
			d->isValid = false;
			d->file.reset();
			return;
		}
		// header info
		size = d->file->read(&d->gms2Header, sizeof(YYGMS2HeaderData));
		if (size != sizeof(YYGMS2HeaderData)) {
			d->isValid = false;
			d->file.reset();
			return;
		}
		// endian swap the GMS2 header
		if (d->isBigEndian) {
			d->gms2Header.AllowStatistics = be32_to_cpu(d->gms2Header.AllowStatistics);
			d->gms2Header.GameSpeed = bef32_to_cpu(d->gms2Header.GameSpeed);
		} else {
			d->gms2Header.AllowStatistics = le32_to_cpu(d->gms2Header.AllowStatistics);
			d->gms2Header.GameSpeed = lef32_to_cpu(d->gms2Header.GameSpeed);
		}
		// TODO: endian swap the GUID
	}

	// read the strings, really badly
	if (d->readNullTerminatedString(d->header.v9.pName, d->projectName) != 0) {
		d->isValid = false;
		d->file.reset();
		return;
	}
	if (d->readNullTerminatedString(d->header.v9.pConfig, d->projectConfig) != 0) {
		d->isValid = false;
		d->file.reset();
		return;
	}
	if (d->readNullTerminatedString(d->header.v9.pGameName, d->gameName) != 0) {
		d->isValid = false;
		d->file.reset();
		return;
	}
	if (d->dataVersion >= 0xA) {
		if (d->readNullTerminatedString(d->header.v10.pDisplayName, d->displayName) != 0) {
			d->isValid = false;
			d->file.reset();
			return;
		}
	}


    if (!d->isValid) {
        d->file.reset();
        return;
    }

    d->fileType = FileType::ResourceFile;
    d->mimeType = "application/x-iff";
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *GMS::systemName(unsigned int type) const
{
	RP_D(const GMS);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GameMaker has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"GM::systemName() array index optimization needs to be updated.");

	static const char *const sysNames_studio1[4] = {
		"GameMaker:Studio",
        "GameMaker",
        "GMS",
        nullptr
	};
	static const char *const sysNames_studio2[4] = {
		"GameMaker",
        "GameMaker",
        "GM",
        nullptr
	};

	if (d->hasGms2Header)
		return sysNames_studio2[type & SYSNAME_TYPE_MASK];
	else
		return sysNames_studio1[type & SYSNAME_TYPE_MASK];
}

int GMS::loadFieldData(void)
{
	RP_D(GMS);
    if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// general info fields
    d->fields.addTab("Info");
	{
	    static const char *const endianness_flags[] = {
			NOP_C_("GameMaker", "Little Endian"),
			NOP_C_("GameMaker", "Big Endian")
		};
		std::vector<std::string> *const v_endianness_flags =
		    RomFields::strArrayToVector_i18n("GameMaker", endianness_flags, ARRAY_SIZE(endianness_flags));
	    d->fields.addField_bitfield(C_("GameMaker", "Endianness"), v_endianness_flags, 3, d->isBigEndian ? 2 : 1);
		// TODO: better way to say we're big endian

	    d->fields.addField_string_numeric(C_("GameMaker", "File Version"), d->dataVersion);

		if (d->projectName.length() != 0)
		    d->fields.addField_string(
			    C_("GameMaker", "Project Name"), d->projectName.c_str(), RomFields::STRF_TRIM_END);

	    if (d->projectConfig.length() != 0)
		    d->fields.addField_string(
			    C_("GameMaker", "Project Config"), d->projectConfig.c_str(), RomFields::STRF_TRIM_END);

	    if (d->gameName.length() != 0)
		    d->fields.addField_string(
			    C_("GameMaker", "Game Name"), d->gameName.c_str(), RomFields::STRF_TRIM_END);

	    char versionNum[32];
	    snprintf(versionNum, sizeof(versionNum), "%i.%i.%i.%i", d->header.v9.MajorVersion,
		    d->header.v9.MinorVersion, d->header.v9.ReleaseVersion, d->header.v9.BuildVersion);
	    d->fields.addField_string(C_("GameMaker", "IDE Version"), versionNum, RomFields::STRF_TRIM_END);

		char resolution[32];
	    snprintf(resolution, sizeof(resolution), "%i x %i", d->header.v9.xscreensize, d->header.v9.yscreensize);
		d->fields.addField_string(C_("GameMaker", "Resolution"), resolution, RomFields::STRF_TRIM_END);

		// TODO: change these to match the specific IDE version as it can change
		static const char *const screen_flags[] = {
			NOP_C_("GameMaker", "Fullscreen"),
			NOP_C_("GameMaker", "VSync"),
			NOP_C_("GameMaker", "Software Vertex Processing"),
			NOP_C_("GameMaker", "Interpolate Pixels"),
			NOP_C_("GameMaker", "Keep Aspect Ratio"),
			NOP_C_("GameMaker", "Show Cursor"),
			NOP_C_("GameMaker", "Resizable Window"),
			NOP_C_("GameMaker", "Allow Fullscreen Switch"),
			NOP_C_("GameMaker", "Allow Dock in Fullscreen (Mac)"),
			NOP_C_("GameMaker", "Free IDE"),
			NOP_C_("GameMaker", "Standard IDE"),
			NOP_C_("GameMaker", "Pro IDE"),
			NOP_C_("GameMaker", "Steam Workshop"),
			NOP_C_("GameMaker", "Use Roaming AppData"),
			NOP_C_("GameMaker", "Borderless Fullscreen"),
			NOP_C_("GameMaker", "JavaScript Enabled"),
			NOP_C_("GameMaker", "Show Hobby Splash"),
			NOP_C_("GameMaker", "Is IDE Build"),
		};
		std::vector<std::string> *const v_screen_flags =
			RomFields::strArrayToVector_i18n("GameMaker", screen_flags, ARRAY_SIZE(screen_flags));
		d->fields.addField_bitfield(
			C_("GameMaker", "Screen Flags"), v_screen_flags, 3, d->header.v9.screenflags);

		d->fields.addField_dateTime(C_("GameMaker", "Build Timestamp"), d->header.v9.datetimeUTC,
		    RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_HAS_TIME |
			    RomFields::RFT_DATETIME_IS_UTC);

		if (d->dataVersion >= 0xA) {
		    if (d->displayName.length() != 0)
			    d->fields.addField_string(
				    C_("GameMaker", "Display Name"), d->displayName.c_str(), RomFields::STRF_TRIM_END);
		}
		if (d->dataVersion >= 0xB) {
			//licensed?
		}
		if (d->dataVersion >= 0xC) {
			// reference: https://github.com/UnderminersTeam/UndertaleModTool/blob/1e1991722f509292c6f58bf9ce7f9748968cf647/UndertaleModLib/Models/UndertaleGeneralInfo.cs#L118

			static const char *const function_classes_1[] = {
				NOP_C_("GameMaker", "Internet"),
				NOP_C_("GameMaker", "Joystick"),
				NOP_C_("GameMaker", "Gamepad"),
				NOP_C_("GameMaker", "Immersion"),
				NOP_C_("GameMaker", "Screengrab"),
				NOP_C_("GameMaker", "Math"),
				NOP_C_("GameMaker", "Action"),
				NOP_C_("GameMaker", "MatrixD3D"),
				NOP_C_("GameMaker", "D3DModel"),
				NOP_C_("GameMaker", "DataStructures"),
				NOP_C_("GameMaker", "File"),
				NOP_C_("GameMaker", "INI"),
				NOP_C_("GameMaker", "Filename"),
				NOP_C_("GameMaker", "Directory"),
				NOP_C_("GameMaker", "Environment"),
				nullptr,
				NOP_C_("GameMaker", "HTTP"),
				NOP_C_("GameMaker", "Encoding"),
				NOP_C_("GameMaker", "UIDialog"),
				NOP_C_("GameMaker", "MotionPlanning"),
				NOP_C_("GameMaker", "ShapeCollision"),
				NOP_C_("GameMaker", "Instance"),
				NOP_C_("GameMaker", "Room"),
				NOP_C_("GameMaker", "Game"),
				NOP_C_("GameMaker", "Display"),
				NOP_C_("GameMaker", "Device"),
				NOP_C_("GameMaker", "Window"),
				NOP_C_("GameMaker", "DrawColor"),
				NOP_C_("GameMaker", "Texture"),
				NOP_C_("GameMaker", "Layer"),
				NOP_C_("GameMaker", "String"),
				NOP_C_("GameMaker", "Tiles")
			};
			std::vector<std::string> *const v_function_classes_1 = RomFields::strArrayToVector_i18n(
				"GameMaker", function_classes_1, ARRAY_SIZE(function_classes_1));

			static const char *const function_classes_2[] = {
				NOP_C_("GameMaker", "Surface"),
				NOP_C_("GameMaker", "Skeleton"),
				NOP_C_("GameMaker", "IO"),
				NOP_C_("GameMaker", "Variables"),
				NOP_C_("GameMaker", "Array"),
				NOP_C_("GameMaker", "ExternalCall"),
				NOP_C_("GameMaker", "Notification"),
				NOP_C_("GameMaker", "Date"),
				NOP_C_("GameMaker", "Particle"),
				NOP_C_("GameMaker", "Sprite"),
				NOP_C_("GameMaker", "Clickable"),
				NOP_C_("GameMaker", "LegacySound"),
				NOP_C_("GameMaker", "Audio"),
				NOP_C_("GameMaker", "Event"),
				nullptr,
				NOP_C_("GameMaker", "FreeType"),
				NOP_C_("GameMaker", "Analytics"),
				nullptr,
				nullptr,
				NOP_C_("GameMaker", "Achievement"),
				NOP_C_("GameMaker", "CloudSaving"),
				NOP_C_("GameMaker", "Ads"),
				NOP_C_("GameMaker", "OS"),
				NOP_C_("GameMaker", "IAP"),
				NOP_C_("GameMaker", "Facebook"),
				NOP_C_("GameMaker", "Physics"),
				NOP_C_("GameMaker", "FlashAA"),
				NOP_C_("GameMaker", "Console"),
				NOP_C_("GameMaker", "Buffer"),
				NOP_C_("GameMaker", "Steam"),
				NOP_C_("GameMaker", "Shaders")
			};
			std::vector<std::string> *const v_function_classes_2 = RomFields::strArrayToVector_i18n(
				"GameMaker", function_classes_2, ARRAY_SIZE(function_classes_2));

			if (d->isBigEndian) {
				// TODO: is this correct?
				d->fields.addField_bitfield(C_("GameMaker", "Function Classes (1)"),
					v_function_classes_1, 3, d->header.v12.functionClasses >> 32);
				d->fields.addField_bitfield(C_("GameMaker", "Function Classes (2)"),
					v_function_classes_2, 3, d->header.v12.functionClasses & 0xFFFFFFFF);
			} else {
				d->fields.addField_bitfield(C_("GameMaker", "Function Classes (1)"),
					v_function_classes_1, 3, d->header.v12.functionClasses & 0xFFFFFFFF);
				d->fields.addField_bitfield(C_("GameMaker", "Function Classes (2)"),
					v_function_classes_2, 3, d->header.v12.functionClasses >> 32);
			}
		}
		if (d->dataVersion >= 0xD) {
			if (d->header.v13.steamAppId != 0 && d->header.v13.steamAppId != 0xFFFFFFFF)
				d->fields.addField_string_numeric(
					C_("GameMaker", "Steam App ID"), -d->header.v13.steamAppId);
		}
		if (d->dataVersion >= 0xE) {
			if (d->header.v14.debuggerServerPort != 0)
				d->fields.addField_string_numeric(
					C_("GameMaker", "Debugger Port"), d->header.v14.debuggerServerPort);
		}

		if (d->hasGms2Header) {
			d->fields.addField_string_numeric(C_("GameMaker", "Game Speed"), d->gms2Header.GameSpeed);

			static const char *const stats_flags[] = {NOP_C_("GameMaker", "Allow Statistics")};
			std::vector<std::string> *const v_stats_flags =
				RomFields::strArrayToVector_i18n("GameMaker", stats_flags, ARRAY_SIZE(stats_flags));

			d->fields.addField_bitfield(C_("GameMaker", "Statistics"), v_stats_flags, 3, d->gms2Header.AllowStatistics);
		}

	    d->fields.addField_string_numeric(C_("GameMaker", "Room Count"), (uint32_t)d->roomOrder.size());
	}

    return 0;
}

}
