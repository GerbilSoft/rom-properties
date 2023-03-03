/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Lua.cpp: Lua binary chunk reader.                                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * Copyright (c) 2016-2022 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Lua.hpp"
#include "lua_structs.h"

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;

// C++ STL classes.
using std::string;
using std::vector;

namespace LibRomData {

class LuaPrivate final : public RomDataPrivate
{
	public:
		LuaPrivate(IRpFile *file);

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(LuaPrivate)

	protected:
		/**
		 * Reset the Lua identification variables.
		 */
		void reset_lua(void);

	public:
		enum class LuaVersion {
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
		LuaVersion luaVersion;

		/**
		 * Converts version byte to Version enum
		 */
		static LuaVersion to_version(uint8_t version);

	public:
		// Lua header.
		uint8_t header[LUA_HEADERSIZE];

		enum class Endianness : int8_t {
			Unknown = -1,
			BE = 0,
			LE = 1,
		};
		Endianness endianness;

		/**
		 * Flip endianness from BE to LE or vice-versa.
		 * @param endianness Endianness
		 * @return Flipped endianness
		 */
		static inline Endianness FlipEndianness(Endianness endianness)
		{
			switch (endianness) {
				case Endianness::BE:	return Endianness::LE;
				case Endianness::LE:	return Endianness::BE;
				default:		assert(!"Invalid endianness."); break;
			}
			return Endianness::Unknown;
		}

		int8_t int_size = -1;		// sizeof(int)
		int8_t size_t_size = -1;	// sizeof(size_t)
		int8_t instruction_size = -1;	// sizeof(lua_Instruction)
		bool weird_layout = false;	// weird layout of the bits within lua_Instruction
		int8_t integer_size = -1;	// sizeof(lua_Integer)
		int8_t number_size = -1;	// sizeof(lua_Number), has a slightly different meaning for 3.x

		enum class IntegralType : int8_t {
			Unknown = -1,
			Float = 0,
			Integer = 1,
			String = 2,	// Lua 3.2
		};
		IntegralType is_integral;	// lua_Number

		bool is_float_swapped;		// float endianness is swapped compared to integer
		bool corrupted;			// the LUA_TAIL is corrupted

	private:
		/**
		 * Compares two byte ranges.
		 * @param lhs buffer to be compared
		 * @param rhs buffer to compare with
		 * @param len size of the buffers
		 * @param endianness when set to LE, one of the buffers is reversed (used for comparing LE number to BE constant)
		 * @return true if equal
		 */
		static bool compare(const uint8_t *lhs, const uint8_t *rhs, size_t len, Endianness endianness);

		/**
		 * Figures out endianness by comparing an integer with a magic constant
		 * @param test_int64 int64 big endian representation of magic
		 * @param p number to check
		 * @param len length of the number
		 * @return the endianness
		 */
		static Endianness detect_endianness_int(const char *test_int64, const uint8_t *p, size_t len);

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
		static Endianness detect_endianness(const char *test_int64,
			const char *test_float32, const char *test_float64,
			const uint8_t *p, size_t len, IntegralType *is_integral);

	public:
		/**
		 * Parses lua header into individual fields.
		 */
		void parse();

	private:
		/**
		 * Parses lua 2.x header into individual fields.
		 */
		void parse2(uint8_t version, const uint8_t *p);

		/**
		 * Parses lua 3.x header into individual fields.
		 */
		void parse3(uint8_t version, const uint8_t *p);

		/**
		 * Parses lua 4.x/5.x header into individual fields.
		 */
		void parse4(uint8_t version, const uint8_t *p);
};

ROMDATA_IMPL(Lua)

/** LuaPrivate **/

/* RomDataInfo */
const char *const LuaPrivate::exts[] = {
	// NOTE: These extensions may cause conflicts on
	// Windows if fallback handling isn't working.
	".lub", // Lua binary
	".out", // from luac.out, the default output filename of luac.

	// TODO: Others?
	nullptr
};
const char *const LuaPrivate::mimeTypes[] = {
	// Unofficial MIME types from FreeDesktop.org.
	// FIXME: Source MIME type is "text/x-lua"; using "application/x-lua" for binary.
	"application/x-lua",
	nullptr
};
const RomDataInfo LuaPrivate::romDataInfo = {
	"Lua", exts, mimeTypes
};

LuaPrivate::LuaPrivate(IRpFile *file)
	: super(file, &romDataInfo)
	, luaVersion(LuaVersion::Unknown)
{
	// Clear the header struct.
	memset(&header, 0, sizeof(header));

	// Reset the Lua identification variables.
	// NOTE: Does NOT reset luaVersion; that's set when
	// the file is loaded.
	reset_lua();
}

void LuaPrivate::reset_lua(void)
{
	endianness = Endianness::Unknown;
	int_size = -1;				// sizeof(int)
	size_t_size = -1;			// sizeof(size_t)
	instruction_size = -1;			// sizeof(lua_Instruction)
	weird_layout = false;			// weird layout of the bits within lua_Instruction
	integer_size = -1;			// sizeof(lua_Integer)
	number_size = -1;			// sizeof(lua_Number), has a slightly different meaning for 3.x
	is_integral = IntegralType::Unknown;	// lua_Number
	is_float_swapped = false;		// float endianness is swapped compared to integer
	corrupted = false;			// the LUA_TAIL is corrupted
}

/**
 * Converts version byte to Version enum
 */
LuaPrivate::LuaVersion LuaPrivate::to_version(uint8_t version)
{
	switch (version) {
		// Bytecode dumping was introduced in 2.3, which was never publicly released.
		// 2.4 kept the same format, so we refer to the 0x23 format as "2.4".
		case 0x23:	return LuaVersion::Lua2_4;
		case 0x25:	return LuaVersion::Lua2_5; // Also used by 3.0
		case 0x31:	return LuaVersion::Lua3_1;
		case 0x32:	return LuaVersion::Lua3_2;
		case 0x40:	return LuaVersion::Lua4_0;
		case 0x50:	return LuaVersion::Lua5_0;
		case 0x51:	return LuaVersion::Lua5_1;
		case 0x52:	return LuaVersion::Lua5_2;
		case 0x53:	return LuaVersion::Lua5_3;
		case 0x54:	return LuaVersion::Lua5_4;
		default:	return LuaVersion::Unknown;
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
bool LuaPrivate::compare(const uint8_t *lhs, const uint8_t *rhs, size_t len, Endianness endianness)
{
	switch (endianness) {
		case Endianness::BE:
			return !memcmp(lhs, rhs, len);

		case Endianness::LE:
			for (size_t i = 0; i < len; i++) {
				if (lhs[i] != rhs[len-1-i]) {
					return false;
				}
			}
			return true;

		default:
			assert(!"Invalid endianness value.");
			break;
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
LuaPrivate::Endianness LuaPrivate::detect_endianness_int(const char *test_int64, const uint8_t *p, size_t len)
{
	const uint8_t *test_int = nullptr;
	if (len == 8)
		test_int = (const uint8_t*)test_int64;
	else if (len == 4)
		test_int = (const uint8_t*)test_int64 + 4;
	else
		return Endianness::Unknown;

	if (compare(p, test_int, len, Endianness::BE))
		return Endianness::BE;
	else if (compare(p, test_int, len, Endianness::LE))
		return Endianness::LE;
	else
		return Endianness::Unknown;
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
LuaPrivate::Endianness LuaPrivate::detect_endianness(const char *test_int64,
	const char *test_float32, const char *test_float64,
	const uint8_t *p, size_t len, IntegralType *is_integral)
{
	const uint8_t *test_int = nullptr;
	const uint8_t *test_float = nullptr;
	if (len == 8) {
		test_int = (const uint8_t*)test_int64;
		test_float = (const uint8_t*)test_float64;
	} else if (len == 4) {
		test_int = (const uint8_t*)test_int64 + 4;
		test_float = (const uint8_t*)test_float32;
	} else {
		*is_integral = IntegralType::Unknown;
		return Endianness::Unknown;
	}

	Endianness endianness;
	if (compare(p, test_float, len, Endianness::BE)) {
		endianness = Endianness::BE;
		*is_integral = IntegralType::Float;
	} else if (compare(p, test_float, len, Endianness::LE)) {
		endianness = Endianness::LE;
		*is_integral = IntegralType::Float;
	} else if (compare(p, test_int, len, Endianness::BE)) {
		endianness = Endianness::BE;
		*is_integral = IntegralType::Integer;
	} else if (compare(p, test_int, len, Endianness::LE)) {
		endianness = Endianness::BE;
		*is_integral = IntegralType::Integer;
	} else {
		endianness = Endianness::Unknown;
		*is_integral = IntegralType::Unknown;
	}
	return endianness;
}

/**
 * Parses lua header into individual fields.
 */
void LuaPrivate::parse()
{
	reset_lua();

	const uint8_t *p = header + 4;
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
void LuaPrivate::parse2(uint8_t version, const uint8_t *p) {
	if (version == 0x25) {
		// these two are hardcoded to be 2 and 4
		p++; // word size
		p++; // float size
		size_t_size = *p++; // pointer size
	}

	const uint8_t *test_word = (const uint8_t*)"\x12\x34"; // 0x1234
	if (compare(p, test_word, 2, Endianness::BE)) {
		endianness = Endianness::BE;
	} else if (compare(p, test_word, 2, Endianness::LE)) {
		endianness = Endianness::LE;
	}

	const uint8_t *test_float = (const uint8_t*)"\x17\xBF\x0A\x46"; // 0.123456789e-23
	if (endianness != Endianness::Unknown &&
	    compare(p + 2, test_float, 4, FlipEndianness(endianness)))
	{
		is_float_swapped = true;
	}
}

/**
 * Parses lua 3.x header into individual fields.
 */
void LuaPrivate::parse3(uint8_t version, const uint8_t *p) {
	if (version == 0x31) {
		uint8_t Number_type = *p++;
		switch (Number_type) {
			case 'l':
				number_size = 4;
				endianness = Endianness::BE;
				is_integral = IntegralType::Integer;
				return;
			case 'f':
				number_size = 4;
				endianness = Endianness::BE;
				is_integral = IntegralType::Float;
				return;
			case 'd':
				number_size = 8;
				endianness = Endianness::BE;
				is_integral = IntegralType::Float;
				return;
			case '?': break;
			default: return;
		}
	}

	number_size = *p++;
	if (version == 0x32 && !number_size) {
		number_size = -1;
		is_integral = IntegralType::String;
		return;
	}

	// This is supposed to be 3.14159265358979323846e8 cast to lua_Number
	endianness = detect_endianness("\x00\x00\x00\x00\x12\xB9\xB0\xA1",
		"\x4D\x95\xCD\x85", "\x41\xB2\xB9\xB0\xA1\x5B\xE6\x12",
		p, number_size, &is_integral);
}

/**
 * Parses lua 4.x/5.x header into individual fields.
 */
void LuaPrivate::parse4(uint8_t version, const uint8_t *p) {
	// Format byte. 0 means official format. Apparently it's meant to be used by forks(?)
	if (version >= 0x51)
		if (*p++ != 0)
			return;

	// Some magic bytes for detecting transmission failures. Very similar to PNG magic.
	// 5.2 had this at the end of the header.
	if (version >= 0x53) {
		if (memcmp(p, LUA_TAIL, sizeof(LUA_TAIL)-1) != 0) {
			corrupted = true;
			return;
		}
		p += sizeof(LUA_TAIL)-1;
	}

	if (version < 0x53) {
		endianness = *p > 1 ? Endianness::Unknown : (Endianness)*p;
		p++;
	}

	// Lua 5.4 encodes int/size_t as varints, so it doesn't need to know their size.
	if (version < 0x54) {
		int_size = *p++;
		size_t_size = *p++;
	}

	   instruction_size = *p++;

	if (version == 0x40) {
		uint8_t INSTRUCTION_bits = *p++;
		uint8_t OP_bits = *p++;
		uint8_t B_bits = *p++;
		if (INSTRUCTION_bits != 32 || OP_bits != 6 || B_bits != 9) {
			weird_layout = true;
		}
	} else if (version == 0x50) {
		uint8_t OP_bits = *p++;
		uint8_t A_bits = *p++;
		uint8_t B_bits = *p++;
		uint8_t C_bits = *p++;
		if (OP_bits != 6 || A_bits != 8 || B_bits != 9 || C_bits != 9) {
			weird_layout = true;
		}
	}

	// Lua 5.3 introduced support for a separate integer type.
	if (version >= 0x53)
		      integer_size = *p++;

	number_size = *p++;

	if (version >= 0x53) {
		// A test number for lua_Integer (0x5678)
		endianness = detect_endianness_int("\x00\x00\x00\x00\x00\x00\x56\x78", p, number_size);
		if (integer_size != 8 && integer_size != 4) // a check to avoid overflows
			return;
		p += integer_size;
		// Note that if this fails, we end up with endianness == -1, and so the test
		// for lua_Number gets skipped.
	}

	if (version == 0x51 || version == 0x52) {
		// Lua 5.1 and 5.2 just have a flag to specify whether lua_Number is int or float.
		is_integral = *p > 1 ? IntegralType::Unknown : (IntegralType)*p;
		p++;
		// End of header for 5.1
	} else if (endianness != Endianness::Unknown) {
		// 4.0, 5.0 and 5.3+ have a test number, from which we can tell the format of lua_Number.

		// NOTE: 5.0 and earlier don't compare the fractional part of the test number.

		// Pick the right set of constants based on version
		Endianness ed = Endianness::Unknown;
		if (version == 0x40) {
			// This is supposed to be 3.14159265358979323846e8 cast to lua_Number
			ed = detect_endianness("\x00\x00\x00\x00\x12\xB9\xB0\xA1",
				"\x4D\x95\xCD\x85", "\x41\xB2\xB9\xB0\xA1\x5B\xE6\x12",
				p, number_size, &is_integral);
		} else if (version == 0x50) {
			// This is supposed to be 3.14159265358979323846e7 cast to lua_Number
			ed = detect_endianness("\x00\x00\x00\x00\x01\xDF\x5E\x76",
				"\x4B\xEF\xAF\x3B", "\x41\x7D\xF5\xE7\x68\x93\x09\xB6",
				p, number_size, &is_integral);
		} else {
			// This is supposed to be 370.5 cast to lua_Number
			ed = detect_endianness("\x00\x00\x00\x00\x00\x00\x01\x72",
				"\x43\xB9\x40\x00", "\x40\x77\x28\x00\x00\x00\x00\x00",
				p, number_size, &is_integral);
		}
		if (is_integral == IntegralType::Float && ed != endianness) {
			is_float_swapped = true;
		}
		// End of header for 4.0, 5.0, 5.3, 5.4
	}

	if (version == 0x52) {
		if (memcmp(p, LUA_TAIL, sizeof(LUA_TAIL)-1) != 0) {
			corrupted = true;
		}
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
	: super(new LuaPrivate(file))
{
	RP_D(Lua);
	d->mimeType = "application/x-lua";	// unofficial; binary files only
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
	const DetectInfo info = {
		{0, sizeof(d->header), d->header},
		nullptr,	// ext (not needed for Lua)
		0		// szFile (not needed for Lua)
	};
	d->luaVersion = static_cast<LuaPrivate::LuaVersion>(isRomSupported_static(&info));
	d->isValid = ((int)d->luaVersion >= 0);

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
		return static_cast<int>(LuaPrivate::LuaVersion::Unknown);
	}

	const uint8_t *header = info->header.pData;
	if (!memcmp(header, LUA_MAGIC, sizeof(LUA_MAGIC)-1)) {
		uint8_t version = header[4];
		uint8_t format = version >= 0x51 ? header[5] : 0;
		if (format == 0) {
			return static_cast<int>(LuaPrivate::to_version(version));
		}
	}

	return static_cast<int>(LuaPrivate::LuaVersion::Unknown);
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
	static_assert((int)LuaPrivate::LuaVersion::Max == 10,
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

	const int i = (int)d->luaVersion;
	if (i < 0 || i >= (int)LuaPrivate::LuaVersion::Max)
		return nullptr;
	return sysNames[i][type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Lua::loadFieldData(void)
{
	RP_D(Lua);
	if (!d->fields.empty()) {
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

	d->fields.reserve(10);	// Maximum of 10 fields.

	if (d->endianness != LuaPrivate::Endianness::Unknown) {
		const char *s_endianness = nullptr;
		switch (d->endianness) {
			case LuaPrivate::Endianness::BE:
				s_endianness = C_("RomData", "Big-Endian");
				break;
			case LuaPrivate::Endianness::LE:
				s_endianness = C_("RomData", "Little-Endian");
				break;
			default:
				assert(!"Invalid endianness.");
				break;
		}
		if (s_endianness) {
			d->fields.addField_string(C_("RomData", "Endianness"), s_endianness);
		}
	}
	if (d->int_size != -1)
		d->fields.addField_string_numeric(C_("Lua", "int size"), d->int_size);
	if (d->size_t_size != -1)
		d->fields.addField_string_numeric(C_("Lua", "size_t size"), d->size_t_size);
	if (d->instruction_size != -1)
		d->fields.addField_string_numeric(C_("Lua", "lua_Instruction size"), d->instruction_size);
	if (d->integer_size != -1)
		d->fields.addField_string_numeric(C_("Lua", "lua_Integer size"), d->integer_size);
	if (d->number_size != -1)
		d->fields.addField_string_numeric(C_("Lua", "lua_Number size"), d->number_size);
	if (d->is_integral != LuaPrivate::IntegralType::Unknown) {
		const char *s_integral_type = nullptr;
		switch (d->is_integral) {
			case LuaPrivate::IntegralType::Float:
				s_integral_type = C_("Lua", "Floating-point");
				break;
			case LuaPrivate::IntegralType::Integer:
				s_integral_type = C_("Lua", "Integer");
				break;
			case LuaPrivate::IntegralType::String:
				s_integral_type = C_("Lua", "String");
				break;
			default:
				assert(!"Invalid integral type.");
				break;
		}
		if (s_integral_type) {
			d->fields.addField_string(C_("Lua", "lua_Number type"), s_integral_type);
		}
	}
	if (d->is_float_swapped)
		d->fields.addField_string(C_("RomData", "Warning"),
			C_("Lua", "Floating-point values are byte-swapped"), RomFields::STRF_WARNING);
	if (d->weird_layout)
		d->fields.addField_string(C_("RomData", "Warning"),
			C_("Lua", "Unusual instruction layout"), RomFields::STRF_WARNING);
	if (d->corrupted)
		d->fields.addField_string(C_("RomData", "Warning"),
			C_("Lua", "File corrupted"), RomFields::STRF_WARNING);

	return static_cast<int>(d->fields.count());
}

}
