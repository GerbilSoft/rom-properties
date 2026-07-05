/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameMaker.cpp: GameMaker IFF/"data.win" header reader                   *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * Copyright (c) 2026 by Emma / InvoxiPlayGames.                           *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "GameMaker.hpp"
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

class GameMakerPrivate final : public RomDataPrivate
{
public:
	explicit GameMakerPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(GameMakerPrivate)

public:
	// Maximum string length (TODO: Better maximum?)
	static constexpr int MAX_STRING_LENGTH = 1024;

	/**
	 * Read a NULL-terminated string.
	 * @param offset	[in] Offset in the file
	 * @param str 		[out] NULL-terminated string
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int readNullTerminatedString(uint32_t offset, string &str);

public:
	/** RomDataInfo **/
	static const array<const char*, 6+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// GMS file type
	enum class GMSFileType {
		Unknown = -1,

		LE = 0,
		BE = 1,

		Max
	};
	GMSFileType gmsFileType;

	inline bool isBigEndian(void) const
	{
		return (gmsFileType == GMSFileType::BE);
	}

	// ROM header
	int dataVersion;
	YYHeader header;
	vector<int> roomOrder;

	bool hasGms2Header;
	bool hasCodeSegment;

	uint8_t gms2License[0x28];
	YYGMS2HeaderData gms2Header;

	// parsed information
	string projectName;
	string projectConfig;
	string gameName;
	string displayName;
};

ROMDATA_IMPL(GameMaker)

const array<const char*, 6+1> GameMakerPrivate::exts = {{
	".win",
	".ios",
	".unx",
	".droid",
	".3ds",
	".symbian",
	nullptr
}};

const array<const char*, 1+1> GameMakerPrivate::mimeTypes = {{
	"application/x-iff",
	nullptr
}};

const RomDataInfo GameMakerPrivate::romDataInfo = {
	"GameMaker", exts.data(), mimeTypes.data()
};

GameMakerPrivate::GameMakerPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, dataVersion(0)
	, hasGms2Header(false)
	, hasCodeSegment(false)
{
	// Clear the various structs.
	memset(&header, 0, sizeof(header));
	memset(&gms2License, 0, sizeof(gms2License));
	memset(&gms2Header, 0, sizeof(gms2Header));
}

/**
 * Read a NULL-terminated string.
 * @param offset	[in] Offset in the file
 * @param str 		[out] NULL-terminated string
 * @return 0 on success; negative POSIX error code on error.
 */
int GameMakerPrivate::readNullTerminatedString(uint32_t offset, string &str)
{
	str.clear();
	if (offset == 0) {
		return -ENOENT;
	}

	// Read a string.
	char buf[MAX_STRING_LENGTH];
	size_t size = file->seekAndRead(offset, buf, sizeof(buf));
	if (size == 0) {
		// Seek and/or read error.
		return -EIO;
	} else if (size > sizeof(buf)) {
		size = sizeof(buf);
	}

	// Ensure NULL termination.
	buf[size-1] = '\0';

	// Find the first NULL terminator.
	const char *pNull = static_cast<const char*>(memchr(buf, '\0', sizeof(buf)));
	if (pNull) {
		str.assign(buf, pNull - buf);
	} else {
		// Shouldn't happen...
		str.assign(buf, sizeof(buf));
	}

	return 0;
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameMaker::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < (sizeof(iff_sect_hdr_t) * 2))
	{
		return static_cast<int>(GameMakerPrivate::GMSFileType::Unknown);
	}

	// NOTE: Little-endian files have "big-endian" fourCCs.
	bool isBigEndian = false;

	// Check the IFF container header to make sure it's a FORM header
	const iff_sect_hdr_t *const formheader =
		reinterpret_cast<const iff_sect_hdr_t*>(info->header.pData);
	if (formheader->magic == cpu_to_le32(FORM_HDR)) {
		isBigEndian = true;
	} else if (formheader->magic == cpu_to_be32(FORM_HDR)) {
		isBigEndian = false;
	} else {
		return static_cast<int>(GameMakerPrivate::GMSFileType::Unknown); // not a valid file
	}
	// Validate the length
	uint32_t formLength = isBigEndian ? be32_to_cpu(formheader->length) : le32_to_cpu(formheader->length);
	if (formLength < (sizeof(iff_sect_hdr_t) + sizeof(YYHeader))) {
		// Length is too small.
		return static_cast<int>(GameMakerPrivate::GMSFileType::Unknown);
	}

	// Check the data after the FORM header to make sure the first value is the GEN8 info header
	const iff_sect_hdr_t *const infoheader =
		reinterpret_cast<const iff_sect_hdr_t*>(info->header.pData + sizeof(iff_sect_hdr_t));
	if (isBigEndian == false &&
	    infoheader->magic == cpu_to_be32(GEN8_HDR) &&
	    le32_to_cpu(infoheader->length) >= sizeof(YYHeader))
	{
		// Little-endian
		return static_cast<int>(GameMakerPrivate::GMSFileType::LE);
	} else if (isBigEndian &&
		   infoheader->magic == cpu_to_le32(GEN8_HDR) &&
		   be32_to_cpu(infoheader->length) >= sizeof(YYHeader))
	{
		// Big-endian
		return static_cast<int>(GameMakerPrivate::GMSFileType::BE);
	}

	// Not valid...
	return static_cast<int>(GameMakerPrivate::GMSFileType::Unknown);
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
GameMaker::GameMaker(const IRpFilePtr &file)
	: super(new GameMakerPrivate(file))
{
	RP_D(GameMaker);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the IFF header.
	array<iff_sect_hdr_t, 2> iffheader;
	d->file->rewind();
	size_t size = d->file->read(&iffheader, sizeof(iffheader));
	if (size != sizeof(iffheader)) {
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(iffheader), reinterpret_cast<const uint8_t*>(&iffheader)},
		nullptr,
		0
	};
	d->gmsFileType = static_cast<GameMakerPrivate::GMSFileType>(isRomSupported_static(&info));
	d->isValid = ((int)d->gmsFileType >= 0);
	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// Read the header, assuming the entire header is present.
	// File pointer will be adjusted afterwards, if necessary.
	size = d->file->read(&d->header, sizeof(d->header));
	if (size != sizeof(d->header)) {
		d->isValid = false;
		d->file.reset();
		return;
	}

	const bool isBigEndian = d->isBigEndian();

	// Get the file format version.
	d->header.infoHeader = isBigEndian ? be32_to_cpu(d->header.infoHeader) : le32_to_cpu(d->header.infoHeader);
	d->dataVersion = (d->header.infoHeader >> 8) & 0xFF;

	// TODO: clean this up! remove duplicated code
	off64_t file_pos_adj = 0;
	if (d->dataVersion < 10) {
		file_pos_adj = -(static_cast<off64_t>(sizeof(d->header)) - YYHEADER_SIZE_COMMON);
	} else {
		switch (d->dataVersion) {
			case 10:
				file_pos_adj = -(static_cast<off64_t>(sizeof(d->header)) - YYHEADER_SIZE_V10);
				break;
			case 11:
				file_pos_adj = -(static_cast<off64_t>(sizeof(d->header)) - YYHEADER_SIZE_V11);
				break;
			case 12:
				file_pos_adj = -(static_cast<off64_t>(sizeof(d->header)) - YYHEADER_SIZE_V12);
				break;
			case 13:
				file_pos_adj = -(static_cast<off64_t>(sizeof(d->header)) - YYHEADER_SIZE_V13);
				break;
			case 14:
			default:
				file_pos_adj = -(static_cast<off64_t>(sizeof(d->header)) - YYHEADER_SIZE_V14);
				static_assert(static_cast<off64_t>(sizeof(d->header)) - YYHEADER_SIZE_V14 == 0, "header size needs to be adjusted!");
				break;
		}
	}

	if (file_pos_adj != 0) {
		int ret = d->file->seek(file_pos_adj, IRpFile::SeekWhence::Cur);
		if (ret != 0) {
			d->isValid = false;
			d->file.reset();
			return;
		}
	}

	// Byteswap the header, if necessary.
	// NOTE: d->header.infoHeader has already been byteswapped.
	if (( isBigEndian && SYS_BYTEORDER == SYS_LIL_ENDIAN) ||
	    (!isBigEndian && SYS_BYTEORDER == SYS_BIG_ENDIAN))
	{
		d->header.pName			= __swab32(d->header.pName);
		d->header.pConfig		= __swab32(d->header.pConfig);
		d->header.roomMaxId		= __swab32(d->header.roomMaxId);
		d->header.roomMaxTileId		= __swab32(d->header.roomMaxTileId);
		d->header.id			= __swab32(d->header.id);
		d->header.buildNumber		= __swab32(d->header.buildNumber);
		d->header.revisionNumber	= __swab32(d->header.revisionNumber);
		d->header.guid3			= __swab32(d->header.guid3);
		d->header.guid4			= __swab32(d->header.guid4);
		d->header.pGameName		= __swab32(d->header.pGameName);
		d->header.Version.Major		= __swab32(d->header.Version.Major);
		d->header.Version.Minor		= __swab32(d->header.Version.Minor);
		d->header.Version.Release	= __swab32(d->header.Version.Release);
		d->header.Version.Build		= __swab32(d->header.Version.Build);
		d->header.xscreensize		= __swab32(d->header.xscreensize);
		d->header.yscreensize		= __swab32(d->header.yscreensize);
		d->header.screenflags		= __swab32(d->header.screenflags);
		d->header.crc			= __swab32(d->header.crc);
		d->header.datetimeUTC		= __swab64(d->header.datetimeUTC);
		if (d->dataVersion >= 10) {
			d->header.v10.pDisplayName		= __swab32(d->header.v10.pDisplayName);
		}
		if (d->dataVersion >= 11) {
			d->header.v11.Licensed			= __swab64(d->header.v11.Licensed);
		}
		if (d->dataVersion >= 12) {
			d->header.v12.functionClasses		= __swab64(d->header.v12.functionClasses);
		}
		if (d->dataVersion >= 13) {
			d->header.v13.steamAppId		= __swab32(d->header.v13.steamAppId);
		}
		if (d->dataVersion >= 14) {
			d->header.v14.debuggerServerPort	= __swab32(d->header.v14.debuggerServerPort);
		}
	}
	
	// load room order from the file
	int roomOrderLength = 0;
	size = d->file->read(&roomOrderLength, sizeof(roomOrderLength));
	if (size != sizeof(roomOrderLength)) {
		d->isValid = false;
		d->file.reset();
		return;
	}
	roomOrderLength = isBigEndian ? be32_to_cpu(roomOrderLength) : le32_to_cpu(roomOrderLength);

	d->roomOrder.resize(roomOrderLength);
	const size_t roomOrderTotalSize = roomOrderLength * sizeof(int);
	size = d->file->read(d->roomOrder.data(), roomOrderLength * sizeof(int));
	if (size != roomOrderTotalSize) {
		d->roomOrder.clear();
		d->isValid = false;
		d->file.reset();
	}

	// Byteswap the room IDs, if necessary.
	if (( isBigEndian && SYS_BYTEORDER == SYS_LIL_ENDIAN) ||
	    (!isBigEndian && SYS_BYTEORDER == SYS_BIG_ENDIAN))
	{
		for (int &roomId : d->roomOrder) {
			roomId = __swab32(roomId);
		}
	}

	// load the GMS2 header information
	if (d->dataVersion >= 14 && d->header.Version.Major >= 2) {
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
		if (isBigEndian) {
#if SYS_BYTEORDER != SYS_BIG_ENDIAN
			d->gms2Header.AllowStatistics	= be32_to_cpu(d->gms2Header.AllowStatistics);
			d->gms2Header.GameSpeed		= bef32_to_cpu(d->gms2Header.GameSpeed);
#endif /* SYS_BYTEORDER != SYS_BIG_ENDIAN */
		} else {
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
			d->gms2Header.AllowStatistics	= le32_to_cpu(d->gms2Header.AllowStatistics);
			d->gms2Header.GameSpeed		= lef32_to_cpu(d->gms2Header.GameSpeed);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */
		}
		// TODO: endian swap the GUID
	}

	// read the strings, really badly
	if (d->readNullTerminatedString(d->header.pName, d->projectName) != 0) {
		d->isValid = false;
		d->file.reset();
		return;
	}
	if (d->readNullTerminatedString(d->header.pConfig, d->projectConfig) != 0) {
		d->isValid = false;
		d->file.reset();
		return;
	}
	if (d->readNullTerminatedString(d->header.pGameName, d->gameName) != 0) {
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

	// run over every file segment
	if (d->file->seek(sizeof(iffheader), IRpFile::SeekWhence::Set) != 0) {
		d->isValid = false;
		d->file.reset();
		return;
	}
	uint32_t file_len = isBigEndian ? be32_to_cpu(iffheader[0].length) : le32_to_cpu(iffheader[0].length);
	file_len += sizeof(iff_sect_hdr_t);
	// skip over the GEN8 header
	uint32_t post_gen8_offset = isBigEndian ? be32_to_cpu(iffheader[1].length) : le32_to_cpu(iffheader[1].length);
	if (d->file->seek(post_gen8_offset, IRpFile::SeekWhence::Cur) != 0) {
		d->isValid = false;
		d->file.reset();
		return;
	}

	// TODO: Limit this?
	while (d->file->tell() < file_len) {
		iff_sect_hdr_t sect_hdr;
		// doubles as making sure every section of the file is telling the truth about its size?
		if (d->file->read(&sect_hdr, sizeof(iff_sect_hdr_t)) != sizeof(iff_sect_hdr_t)) {
			d->isValid = false;
			d->file.reset();
			return;
		}
		// NOTE: Little-endian files have "big-endian" fourCCs.
		uint32_t host_magic_expected = isBigEndian ? cpu_to_le32(CODE_HDR) : cpu_to_be32(CODE_HDR);
		uint32_t host_len = isBigEndian ? be32_to_cpu(sect_hdr.length) : le32_to_cpu(sect_hdr.length);

		// has a non-zero length code segment - not YYC
		if (sect_hdr.magic == host_magic_expected && host_len > 0) {
			d->hasCodeSegment = true;
		}

		d->file->seek(host_len, IRpFile::SeekWhence::Cur);
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
const char *GameMaker::systemName(unsigned int type) const
{
	RP_D(const GameMaker);
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

	if (d->hasGms2Header) {
		return sysNames_studio2[type & SYSNAME_TYPE_MASK];
	} else {
		return sysNames_studio1[type & SYSNAME_TYPE_MASK];
	}
}

int GameMaker::loadFieldData(void)
{
	RP_D(GameMaker);
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
		if (d->displayName.length() != 0) {
			d->fields.addField_string(C_("GameMaker", "Display Name"), d->displayName.c_str(),
				RomFields::STRF_TRIM_END);
		}

		if (d->gameName.length() != 0) {
			d->fields.addField_string(
				C_("GameMaker", "Game Name"), d->gameName.c_str(), RomFields::STRF_TRIM_END);
		}

		d->fields.addField_string_numeric(C_("GameMaker", "Format Version"), d->dataVersion);

		d->fields.addField_string(C_("GameMaker", "Runtime"), d->hasCodeSegment ? "VM" : "YYC", 0);

		if (d->projectName.length() != 0) {
		    d->fields.addField_string(
			    C_("GameMaker", "Project Name"), d->projectName.c_str(), RomFields::STRF_TRIM_END);
		}

		if (d->projectConfig.length() != 0) {
			d->fields.addField_string(
				C_("GameMaker", "Project Config"), d->projectConfig.c_str(), RomFields::STRF_TRIM_END);
		}
		
		// HACK: to make GCC happy with the packed structures, make a copy of the Version struct
		const YYVersion version = d->header.Version;
		string versionNum =
			fmt::format(FSTR("{}.{}.{}.{}"), version.Major, version.Minor, version.Release, version.Build);
		d->fields.addField_string(C_("GameMaker", "IDE Version"), versionNum, 0);

		d->fields.addField_dimensions(
			C_("GameMaker", "Resolution"), d->header.xscreensize, d->header.yscreensize);

		d->fields.addField_dateTime(C_("GameMaker", "Build Timestamp"), d->header.datetimeUTC,
			RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_HAS_TIME |
				RomFields::RFT_DATETIME_IS_UTC);

		static const array<const char*, 19> screen_flags = {{
			NOP_C_("GameMaker|ScreenFlags", "Fullscreen"),
			NOP_C_("GameMaker|ScreenFlags", "VSync"),
			NOP_C_("GameMaker|ScreenFlags", "SW Vertexes"),
			NOP_C_("GameMaker|ScreenFlags", "Anti-Aliasing"),
			NOP_C_("GameMaker|ScreenFlags", "Keep Aspect Ratio"),
			NOP_C_("GameMaker|ScreenFlags", "Show Cursor"),
			NOP_C_("GameMaker|ScreenFlags", "Resizable"),
			NOP_C_("GameMaker|ScreenFlags", "Allow FS Switch"),
			NOP_C_("GameMaker|ScreenFlags", "Allow Dock in FS"),
			NOP_C_("GameMaker|ScreenFlags", "Free IDE"),
			NOP_C_("GameMaker|ScreenFlags", "Standard IDE"),
			NOP_C_("GameMaker|ScreenFlags", "Pro IDE"),
			NOP_C_("GameMaker|ScreenFlags", "Steam Workshop"),
			NOP_C_("GameMaker|ScreenFlags", "Roaming AppData"),
			NOP_C_("GameMaker|ScreenFlags", "Borderless FS"),
			NOP_C_("GameMaker|ScreenFlags", "JS Enabled"),
			// TODO: change these to match the specific IDE version as it can change
			NOP_C_("GameMaker|ScreenFlags", "Is IDE Build"), // prev "Show Hobby Splash"
			NOP_C_("GameMaker|ScreenFlags", "Transparent BG"), // prev "Is IDE Build"
			NOP_C_("GameMaker|ScreenFlags", "D3D Swap Discard"),
		}};
		vector<string> *const v_screen_flags =
			RomFields::strArrayToVector_i18n("GameMaker", screen_flags);
		d->fields.addField_bitfield(
			C_("GameMaker", "Screen Flags"), v_screen_flags, 3, d->header.screenflags);

		if (d->dataVersion >= 11) {
			// license data? never referenced by the runner, always seen as 0
		}

		if (d->dataVersion >= 12) {
			// reference: https://github.com/UnderminersTeam/UndertaleModTool/blob/1e1991722f509292c6f58bf9ce7f9748968cf647/UndertaleModLib/Models/UndertaleGeneralInfo.cs#L118

			static const array<const char*, 63> function_classes = {{
				NOP_C_("GameMaker|FunctionClass", "Internet"),
				NOP_C_("GameMaker|FunctionClass", "Joystick"),
				NOP_C_("GameMaker|FunctionClass", "Gamepad"),
				NOP_C_("GameMaker|FunctionClass", "Immersion"),
				NOP_C_("GameMaker|FunctionClass", "Screengrab"),
				NOP_C_("GameMaker|FunctionClass", "Math"),
				NOP_C_("GameMaker|FunctionClass", "Action"),
				NOP_C_("GameMaker|FunctionClass", "MatrixD3D"),
				NOP_C_("GameMaker|FunctionClass", "D3DModel"),
				NOP_C_("GameMaker|FunctionClass", "DataStructures"),
				NOP_C_("GameMaker|FunctionClass", "File"),
				NOP_C_("GameMaker|FunctionClass", "INI"),
				NOP_C_("GameMaker|FunctionClass", "Filename"),
				NOP_C_("GameMaker|FunctionClass", "Directory"),
				NOP_C_("GameMaker|FunctionClass", "Environment"),
				nullptr,
				NOP_C_("GameMaker|FunctionClass", "HTTP"),
				NOP_C_("GameMaker|FunctionClass", "Encoding"),
				NOP_C_("GameMaker|FunctionClass", "UIDialog"),
				NOP_C_("GameMaker|FunctionClass", "MotionPlanning"),
				NOP_C_("GameMaker|FunctionClass", "ShapeCollision"),
				NOP_C_("GameMaker|FunctionClass", "Instance"),
				NOP_C_("GameMaker|FunctionClass", "Room"),
				NOP_C_("GameMaker|FunctionClass", "Game"),
				NOP_C_("GameMaker|FunctionClass", "Display"),
				NOP_C_("GameMaker|FunctionClass", "Device"),
				NOP_C_("GameMaker|FunctionClass", "Window"),
				NOP_C_("GameMaker|FunctionClass", "DrawColor"),
				NOP_C_("GameMaker|FunctionClass", "Texture"),
				NOP_C_("GameMaker|FunctionClass", "Layer"),
				NOP_C_("GameMaker|FunctionClass", "String"),
				NOP_C_("GameMaker|FunctionClass", "Tiles"),
				NOP_C_("GameMaker|FunctionClass", "Surface"),
				NOP_C_("GameMaker|FunctionClass", "Skeleton"),
				NOP_C_("GameMaker|FunctionClass", "IO"),
				NOP_C_("GameMaker|FunctionClass", "Variables"),
				NOP_C_("GameMaker|FunctionClass", "Array"),
				NOP_C_("GameMaker|FunctionClass", "ExternalCall"),
				NOP_C_("GameMaker|FunctionClass", "Notification"),
				NOP_C_("GameMaker|FunctionClass", "Date"),
				NOP_C_("GameMaker|FunctionClass", "Particle"),
				NOP_C_("GameMaker|FunctionClass", "Sprite"),
				NOP_C_("GameMaker|FunctionClass", "Clickable"),
				NOP_C_("GameMaker|FunctionClass", "LegacySound"),
				NOP_C_("GameMaker|FunctionClass", "Audio"),
				NOP_C_("GameMaker|FunctionClass", "Event"),
				nullptr,
				NOP_C_("GameMaker|FunctionClass", "FreeType"),
				NOP_C_("GameMaker|FunctionClass", "Analytics"),
				nullptr,
				nullptr,
				NOP_C_("GameMaker|FunctionClass", "Achievement"),
				NOP_C_("GameMaker|FunctionClass", "CloudSaving"),
				NOP_C_("GameMaker|FunctionClass", "Ads"),
				NOP_C_("GameMaker|FunctionClass", "OS"),
				NOP_C_("GameMaker|FunctionClass", "IAP"),
				NOP_C_("GameMaker|FunctionClass", "Facebook"),
				NOP_C_("GameMaker|FunctionClass", "Physics"),
				NOP_C_("GameMaker|FunctionClass", "FlashAA"),
				NOP_C_("GameMaker|FunctionClass", "Console"),
				NOP_C_("GameMaker|FunctionClass", "Buffer"),
				NOP_C_("GameMaker|FunctionClass", "Steam"),
				NOP_C_("GameMaker|FunctionClass", "Shaders")
			}};

			// Copied from Nintendo3DS. (TODO: Centralize it?)
#ifdef _WIN32
			// Windows: 6 visible rows per RFT_LISTDATA.
			static constexpr int rows_visible = 6;
#else  /* !_WIN32 */
			// Linux: 4 visible rows per RFT_LISTDATA.
			static constexpr int rows_visible = 4;
#endif /* _WIN32 */

			// FIXME: Use checkboxes. RFT_LISTDATA has a 32-bit checkbox field,
			// so this will need to be extended at some point.
			auto *const vv_fn_classes = new RomFields::ListData_t;
			for (size_t i = 0; i < function_classes.size(); i++) {
				if (function_classes[i] != nullptr && d->header.v12.functionClasses & ((uint64_t)1 << i)) {
					vector<string> v_fn_class;
					v_fn_class.push_back(function_classes[i]);
					vv_fn_classes->push_back(std::move(v_fn_class));
				}
			}

			if (!vv_fn_classes->empty()) {
				RomFields::AFLD_PARAMS params(0, rows_visible);
				params.headers = nullptr;
				params.data.single = vv_fn_classes;
				d->fields.addField_listData(C_("GameMaker", "Function Classes"), &params);
			} else {
				delete vv_fn_classes;
			}
		}

		if (d->dataVersion >= 13) {
			if (d->header.v13.steamAppId != 0 && d->header.v13.steamAppId != -1) {
				// NOTE: GMS1 and early GMS2 store the Steam app ID as a negative number.
				d->fields.addField_string_numeric(C_("GameMaker", "Steam App ID"), abs(d->header.v13.steamAppId));
			}
		}
		if (d->dataVersion >= 14) {
			if (d->header.v14.debuggerServerPort > 0 && d->header.v14.debuggerServerPort <= 65535) {
				d->fields.addField_string_numeric(
					C_("GameMaker", "Debugger Port"), d->header.v14.debuggerServerPort);
			}
		}

		if (d->hasGms2Header) {
			// loss of precision on the float - but the float will never usually be anything that isn't a full number anyway
			d->fields.addField_string_numeric(C_("GameMaker", "Game Speed"), (uint32_t)d->gms2Header.GameSpeed);

			static const array<const char*, 1> stats_flags = {{
				NOP_C_("GameMaker", "Allow Statistics")
			}};
			vector<string> *const v_stats_flags =
				RomFields::strArrayToVector_i18n("GameMaker", stats_flags);
			d->fields.addField_bitfield(C_("GameMaker", "Statistics"), v_stats_flags, 3, d->gms2Header.AllowStatistics);
		}

		d->fields.addField_string_numeric(C_("GameMaker", "Room Count"), (uint32_t)d->roomOrder.size());
	}

	// Finished reading the field data.
	return d->fields.count();
}

int GameMaker::loadMetaData(void)
{
	RP_D(GameMaker);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		return -EBADF;
	} else if (!d->isValid) {
		return -EIO;
	}

	d->metaData.reserve(3); // Maximum of 3 metadata properties.

	if (d->displayName.length() != 0) {
		d->metaData.addMetaData_string(Property::Title, d->displayName, RomMetaData::STRF_TRIM_END);
	} else if (d->gameName.length() != 0) {
		d->metaData.addMetaData_string(Property::Title, d->gameName, RomMetaData::STRF_TRIM_END);
	}

	d->metaData.addMetaData_timestamp(Property::CreationDate, d->header.datetimeUTC);

	d->metaData.addMetaData_string(Property::Generator, d->hasGms2Header ? "GameMaker" : "GameMaker:Studio");

	return d->metaData.count();
}

} // namespace LibRomData
