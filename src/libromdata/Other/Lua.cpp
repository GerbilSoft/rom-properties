/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Lua.cpp: Lua binary chunk reader.                                       *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * Copyright (c) 2016-2022 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Lua.hpp"

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;

// C++ STL classes.
using std::string;
using std::vector;

namespace LibRomData {

/* Actual header sizes:
 * 2.4: 11
 * 2.5: 14
 * 3.1: 7+Number
 * 3.2: 6+Number
 * 4.0: 13+Number
 * 5.0: 14+Number
 * 5.1: 12
 * 5.2: 18
 * 5.3: 17+Integer+Number (the biggest one)
 * 5.4: 15+Integer+Number
 */
#define LUA_MAGIC "\033Lua"
#define LUA_TAIL "\x19\x93\r\n\x1a\n"
#define LUA_HEADERSIZE (17+8+8)

class LuaPrivate final : public RomDataPrivate
{
	public:
		LuaPrivate(Lua *q, IRpFile *file);

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(LuaPrivate)

	public:
		enum class Version {
			Unknown = -1,
			Lua2_4 = 0,
			Lua2_5 = 1,
			Lua3_1 = 2,
			Lua3_2 = 3,
			Lua4_0 = 4,
			Lua5_0 = 5,
			Lua5_1 = 6,
			Lua5_2 = 7,
			Lua5_3 = 8,
			Lua5_4 = 9,
			Max
		};

		Version version;

		/**
		 * Converts version byte to Version enum
		 */
		static Version to_version(uint8_t version);

	private:
		/**
		 * Compares two byte ranges.
		 * @param lhs buffer to be compared
		 * @param rhs buffer to compare with
		 * @param len size of the buffers
		 * @param endianness when set to 1, one of the buffers is reversed (used for comparing LE number to BE constant)
		 * @return true if equal
		 */
		static bool compare(const uint8_t *lhs, const uint8_t *rhs, size_t len, int endianness);

		/**
		 * Figures out endianness by comparing an integer with a magic constant
		 * @param test_int64 int64 big endian representation of magic
		 * @param p number to check
		 * @param len length of the number
		 * @return the endianness
		 */
		static int detect_endianness_int(const char *test_int64, const uint8_t *p, size_t len);

		/**
		 * Figures out endianness and type by comparing a number with a magic constant
		 * @param test_int64 int64 big endian representation of magic
		 * @param test_float32 float32 big endian representation of magic
		 * @param test_float64 float64 big endian representation of magic
		 * @param p number to check
		 * @param len length of the number
		 * @param is_integral (out) whether number is int or float
		 * @return the endianness
		 */
		static int detect_endianness(const char *test_int64, const char *test_float32, const char *test_float64, const uint8_t *p, size_t len, int *is_integral);
	public:
		/**
		 * Parses lua header into individual fields.
		 */
		void parse();

	private:
		/**
		 * Parses lua 2.x header into individual fields.
		 */
		void parse2(uint8_t version, uint8_t *p);

		/**
		 * Parses lua 3.x header into individual fields.
		 */
		void parse3(uint8_t version, uint8_t *p);

		/**
		 * Parses lua 4.x/5.x header into individual fields.
		 */
		void parse4(uint8_t version, uint8_t *p);

	public:
		// Lua header.
		uint8_t header[LUA_HEADERSIZE];

		int endianness = -1; // -1 - Unknown, 0 - BE, 1 - LE
		int int_size = -1; // sizeof(int)
		int size_t_size = -1; // sizeof(size_t)
		int Instruction_size = -1; // sizeof(lua_Instruction)
		bool weird_layout = false; // weird layout of the bits within lua_Instruction
		int Integer_size = -1; // sizeof(lua_Integer)
		int Number_size = -1; // sizeof(lua_Number), has a slightly different meaning for 3.x
		int is_integral = -1; // lua_Number is: -1 - Unknown, 0 - float, 1 - integer, 2 - string (3.2)
		bool is_float_swapped = false; // float endianness is swapped compared to integer
		bool corrupted = false; // the LUA_TAIL is corrupted
};

ROMDATA_IMPL(Lua)

/** LuaPrivate **/

/* RomDataInfo */
const char *const LuaPrivate::exts[] = {
	// NOTE: These extensions may cause conflicts on
	// Windows if fallback handling isn't working.
	".lua",	// Lua source code.
	".out", // from luac.out, the default output filename of luac.
	".lub", // Lua binary
	// TODO: Others?
	nullptr
};
const char *const LuaPrivate::mimeTypes[] = {
	// Unofficial MIME types from FreeDesktop.org.
	// FIXME: these are the MIME types for Lua source code
	"application/x-lua",
	"text/x-lua",
	nullptr
};
const RomDataInfo LuaPrivate::romDataInfo = {
	"Lua", exts, mimeTypes
};

LuaPrivate::LuaPrivate(Lua *q, IRpFile *file)
	: super(q, file, &romDataInfo)
{
	// Clear the header struct.
	memset(&header, 0, sizeof(header));
}

/**
 * Converts version byte to Version enum
 */
LuaPrivate::Version LuaPrivate::to_version(uint8_t version) {
	switch (version) {
	// Bytecode dumping was introduced in 2.3, which was never publicly released.
	// 2.4 kept the same format, so we refer to the 0x23 format as "2.4".
	case 0x23: return Version::Lua2_4;
	case 0x25: return Version::Lua2_5; // Also used by 3.0
	case 0x31: return Version::Lua3_1;
	case 0x32: return Version::Lua3_2;
	case 0x40: return Version::Lua4_0;
	case 0x50: return Version::Lua5_0;
	case 0x51: return Version::Lua5_1;
	case 0x52: return Version::Lua5_2;
	case 0x53: return Version::Lua5_3;
	case 0x54: return Version::Lua5_4;
	default:   return Version::Unknown;
	}
}

/**
 * Compares two byte ranges.
 * @param lhs buffer to be compared
 * @param rhs buffer to compare with
 * @param len size of the buffers
 * @param endianness when set to 1, one of the buffers is reversed (used for comparing LE number to BE constant)
 * @return true if equal
 */
bool LuaPrivate::compare(const uint8_t *lhs, const uint8_t *rhs, size_t len, int endianness)
{
	assert(endianness == 0 || endianness == 1);
	if (endianness == 0) {
		return !memcmp(lhs, rhs, len);
	} else if (endianness == 1) {
		for (size_t i = 0; i < len ; i++)
			if (lhs[i] != rhs[len-1-i])
				return false;
		return true;
	}
	return false;
}

/**
 * Figures out endianness by comparing an integer with a magic constant
 * @param test_int64 int64 big endian representation of magic
 * @param p number to check
 * @param len length of the number
 * @return the endianness
 */
int LuaPrivate::detect_endianness_int(const char *test_int64, const uint8_t *p, size_t len) {
	const uint8_t *test_int = nullptr;
	if (len == 8)
		test_int = (const uint8_t*)test_int64;
	else if (len == 4)
		test_int = (const uint8_t*)test_int64 + 4;
	else
		return -1;

	if (compare(p, test_int, len, 0))
		return 0;
	else if (compare(p, test_int, len, 1))
		return 1;
	else
		return -1;
}

/**
 * Figures out endianness and type by comparing a number with a magic constant
 * @param test_int64 int64 big endian representation of magic
 * @param test_float32 float32 big endian representation of magic
 * @param test_float64 float64 big endian representation of magic
 * @param p number to check
 * @param len length of the number
 * @param is_integral (out) whether number is int or float
 * @return the endianness
 */
int LuaPrivate::detect_endianness(const char *test_int64, const char *test_float32, const char *test_float64, const uint8_t *p, size_t len, int *is_integral) {
	const uint8_t *test_int = nullptr;
	const uint8_t *test_float = nullptr;
	if (len == 8) {
		test_int = (const uint8_t*)test_int64;
		test_float = (const uint8_t*)test_float64;
	} else if (len == 4) {
		test_int = (const uint8_t*)test_int64 + 4;
		test_float = (const uint8_t*)test_float32;
	} else {
		*is_integral = -1;
		return -1;
	}
	int endianness = -1;
	if (compare(p, test_float, len, 0))
		endianness = 0, *is_integral = 1;
	else if (compare(p, test_float, len, 1))
		endianness = 1, *is_integral = 1;
	else if (compare(p, test_int, len, 0))
		endianness = 0, *is_integral = 1;
	else if (compare(p, test_int, len, 1))
		endianness = 1, *is_integral = 1;
	else
		endianness = -1, *is_integral = -1;
	return endianness;
}

/**
 * Parses lua header into individual fields.
 */
void LuaPrivate::parse()
{
	endianness = -1;
	int_size = -1;
	size_t_size = -1;
	Instruction_size = -1;
	weird_layout = false;
	Integer_size = -1;
	Number_size = -1;
	is_integral = -1;
	is_float_swapped = false;
	corrupted = false;

	uint8_t *p = header + 4;
	uint8_t version = *p++;

	if (version < 0x31)
		parse2(version, p);
	else if (version < 0x40)
		parse3(version, p);
	else
		parse4(version, p);
}

/*
 * Parses lua 2.x header into individual fields.
 */
void LuaPrivate::parse2(uint8_t version, uint8_t *p) {
	if (version == 0x25) {
		// these two are hardcoded to be 2 and 4
		p++; // word size
		p++; // float size
		size_t_size = *p++; // pointer size
	}

	const uint8_t *test_word = (const uint8_t*)"\x12\x34"; // 0x1234
	if (compare(p, test_word, 2, 0))
		endianness = 0;
	else if (compare(p, test_word, 2, 1))
		endianness = 1;

	const uint8_t *test_float = (const uint8_t*)"\x17\xBF\x0A\x46"; // 0.123456789e-23
	if (endianness != -1 && compare(p + 2, test_float, 4, !endianness))
		is_float_swapped = true;
}

/**
 * Parses lua 3.x header into individual fields.
 */
void LuaPrivate::parse3(uint8_t version, uint8_t *p) {
	if (version == 0x31) {
		uint8_t Number_type = *p++;
		switch (Number_type) {
			case 'l': Number_size = 4; endianness = 0; is_integral = 1; return;
			case 'f': Number_size = 4; endianness = 0; is_integral = 0; return;
			case 'd': Number_size = 8; endianness = 0; is_integral = 0; return;
			case '?': break;
			default: return;
		}
	}

	Number_size = *p++;
	if (version == 0x32 && !Number_size) {
		Number_size = -1;
		is_integral = 2;
		return;
	}

	// This is supposed to be 3.14159265358979323846e8 cast to lua_Number
	endianness = detect_endianness("\x00\x00\x00\x00\x12\xB9\xB0\xA1", "\x4D\x95\xCD\x85", "\x41\xB2\xB9\xB0\xA1\x5B\xE6\x12",
		p, Number_size, &is_integral);
}

/**
 * Parses lua 4.x/5.x header into individual fields.
 */
void LuaPrivate::parse4(uint8_t version, uint8_t *p) {
	// Format byte. 0 means official format. Apparently it's meant to be used by forks(?)
	if (version >= 0x51)
		if (*p++ != 0)
			return;

	// Some magic bytes for detecting transmission failures. Very similar to PNG magic.
	// 5.2 had this at the end of the header.
	if (version >= 0x53) {
		if (memcmp(p, LUA_TAIL, sizeof(LUA_TAIL)-1)) {
			corrupted = true;
			return;
		}
		p += sizeof(LUA_TAIL)-1;
	}

	if (version < 0x53) {
		endianness = *p > 1 ? -1 : *p;
		p++;
	}

	// Lua 5.4 encodes int/size_t as varints, so it doesn't need to know their size.
	if (version < 0x54) {
		int_size = *p++;
		size_t_size = *p++;
	}

	Instruction_size = *p++;

	if (version == 0x40) {
		uint8_t INSTRUCTION_bits = *p++;
		uint8_t OP_bits = *p++;
		uint8_t B_bits = *p++;
		if (INSTRUCTION_bits != 32 || OP_bits != 6 || B_bits != 9)
			weird_layout = true;
	} else if (version == 0x50) {
		uint8_t OP_bits = *p++;
		uint8_t A_bits = *p++;
		uint8_t B_bits = *p++;
		uint8_t C_bits = *p++;
		if (OP_bits != 6 || A_bits != 8 || B_bits != 9 || C_bits != 9)
			weird_layout = true;
	}

	// Lua 5.3 introduced support for a separate integer type.
	if (version >= 0x53)
		Integer_size = *p++;

	Number_size = *p++;
	
	if (version >= 0x53) {
		// A test number for lua_Integer (0x5678)
		endianness = detect_endianness_int("\x00\x00\x00\x00\x00\x00\x56\x78", p, Number_size);
		if (Integer_size != 8 && Integer_size != 4) // a check to avoid overflows
			return;
		p += Integer_size;
		// Note that if this fails, we end up with endianness == -1, and so the test
		// for lua_Number gets skipped.
	}

	if (version == 0x51 || version == 0x52) {
		// Lua 5.1 and 5.2 just have a flag to specify whether lua_Number is int or float.
		is_integral = *p > 1 ? -1 : *p;
		p++;
		// End of header for 5.1
	} else if (endianness != -1) {
		// 4.0, 5.0 and 5.3+ have a test number, from which we can tell the format of lua_Number.

		// NOTE: 5.0 and earlier don't compare the fractional part of the test number.

		// Pick the right set of constants based on version
		int ed = -1;
		if (version == 0x40) {
			// This is supposed to be 3.14159265358979323846e8 cast to lua_Number
			ed = detect_endianness("\x00\x00\x00\x00\x12\xB9\xB0\xA1", "\x4D\x95\xCD\x85", "\x41\xB2\xB9\xB0\xA1\x5B\xE6\x12",
				p, Number_size, &is_integral);
		} else if (version == 0x50) {
			// This is supposed to be 3.14159265358979323846e7 cast to lua_Number
			ed = detect_endianness("\x00\x00\x00\x00\x01\xDF\x5E\x76", "\x4B\xEF\xAF\x3B", "\x41\x7D\xF5\xE7\x68\x93\x09\xB6",
				p, Number_size, &is_integral);
		} else {
			// This is supposed to be 370.5 cast to lua_Number
			ed = detect_endianness("\x00\x00\x00\x00\x00\x00\x01\x72", "\x43\xB9\x40\x00", "\x40\x77\x28\x00\x00\x00\x00\x00",
				p, Number_size, &is_integral);
		}
		if (is_integral == 0 && ed != endianness)
			is_float_swapped = true;
		// End of header for 4.0, 5.0, 5.3, 5.4
	}

	if (version == 0x52) {
		if (memcmp(p, LUA_TAIL, sizeof(LUA_TAIL)-1))
			corrupted = true;
		// End of header for 5.2
	}
}

/** Lua **/

/**
 * Read a Lua binary chunk.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM file.
 */
Lua::Lua(IRpFile *file)
	: super(new LuaPrivate(this, file))
{
	RP_D(Lua);
	d->mimeType = "text/x-lua";	// unofficial
	d->fileType = FileType::Executable; // FIXME: maybe another type should be introduced?

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Seek to the beginning of the header.
	d->file->rewind();

	// Read the ROM header.
	size_t size = d->file->read(&d->header, sizeof(d->header));
	if (size != sizeof(d->header)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->header);
	info.header.pData = d->header;
	info.ext = nullptr;	// Not needed for Lua.
	info.szFile = 0;	// Not needed for Lua.
	d->version = static_cast<LuaPrivate::Version>(isRomSupported_static(&info));
	d->isValid = ((int)d->version >= 0);

	if (!d->isValid) {
		UNREF_AND_NULL_NOCHK(d->file);
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Lua::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < LUA_HEADERSIZE)
	{
		return static_cast<int>(LuaPrivate::Version::Unknown);
	}

	const uint8_t *header = info->header.pData;
	if (!memcmp(header, LUA_MAGIC, sizeof(LUA_MAGIC)-1)) {
		uint8_t version = header[4];
		uint8_t format = version >= 0x51 ? header[5] : 0;
		if (format == 0)
			return static_cast<int>(LuaPrivate::to_version(version));
	}
		
	return static_cast<int>(LuaPrivate::Version::Unknown);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @return System name, or nullptr if not supported.
 */
const char *Lua::systemName(unsigned int type) const
{
	RP_D(const Lua);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	static_assert(SYSNAME_TYPE_MASK == 3,
		"Lua::systemName() array index optimization needs to be updated.");
	static_assert((int)LuaPrivate::Version::Max == 10,
		"Lua::systemName() array index optimization needs to be updated.");
	
	static const char *const sysNames[10][4] = {
		{"PUC Lua 2.4", "Lua 2.4", "Lua", nullptr},
		{"PUC Lua 2.5/3.0", "Lua 2.5/3.0", "Lua", nullptr},
		{"PUC Lua 3.1", "Lua 3.1", "Lua", nullptr},
		{"PUC Lua 3.2", "Lua 3.2", "Lua", nullptr},
		{"PUC Lua 4.0", "Lua 4.0", "Lua", nullptr},
		{"PUC Lua 5.0", "Lua 5.0", "Lua", nullptr},
		{"PUC Lua 5.1", "Lua 5.1", "Lua", nullptr},
		{"PUC Lua 5.2", "Lua 5.2", "Lua", nullptr},
		{"PUC Lua 5.3", "Lua 5.3", "Lua", nullptr},
		{"PUC Lua 5.4", "Lua 5.4", "Lua", nullptr},
	};

	return sysNames[(int)d->version][type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Lua::loadFieldData(void)
{
	RP_D(Lua);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	d->parse();

	d->fields->reserve(10);	// Maximum of 10 fields.

	if (d->endianness != -1)
		d->fields->addField_string(C_("Lua", "Endianness"),
			d->endianness ? C_("Lua", "Little-endian") : C_("Lua", "Big-endian"));
	if (d->int_size != -1)
		d->fields->addField_string_numeric(C_("Lua", "int size"), d->int_size);
	if (d->size_t_size != -1)
		d->fields->addField_string_numeric(C_("Lua", "size_t size"), d->size_t_size);
	if (d->Instruction_size != -1)
		d->fields->addField_string_numeric(C_("Lua", "lua_Instruction size"), d->Instruction_size);
	if (d->Integer_size != -1)
		d->fields->addField_string_numeric(C_("Lua", "lua_Integer size"), d->Integer_size);
	if (d->Number_size != -1)
		d->fields->addField_string_numeric(C_("Lua", "lua_Number size"), d->Number_size);
	if (d->is_integral != -1)
		d->fields->addField_string(C_("Lua", "lua_Number type"),
			d->is_integral == 2 ? C_("Lua", "String") :
			d->is_integral == 1 ? C_("Lua", "Integer") : C_("Lua", "Floating-point"));
	if (d->is_float_swapped)
		d->fields->addField_string(C_("RomData", "Warning"),
			C_("Lua", "Floating-point values are byte-swapped"), RomFields::STRF_WARNING);
	if (d->weird_layout)
		d->fields->addField_string(C_("RomData", "Warning"),
			C_("Lua", "Unusual instruction layout"), RomFields::STRF_WARNING);
	if (d->corrupted)
		d->fields->addField_string(C_("RomData", "Warning"),
			C_("Lua", "File corrupted"), RomFields::STRF_WARNING);

	return static_cast<int>(d->fields->count());
}

}
