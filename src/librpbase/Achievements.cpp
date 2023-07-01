/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * Achievements.cpp: Achievements class.                                   *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Achievements.hpp"
#include "libi18n/i18n.h"

// librpfile, librptexture
#include "librpfile/FileSystem.hpp"
#include "librpfile/RpFile.hpp"
using namespace LibRpFile;

// C++ STL classes.
using std::string;
using std::unordered_map;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

// zlib for CRC32.
#include <zlib.h>
#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

#ifdef _WIN32
// Win32 is needed for GetCurrentProcessId().
#  include "libwin32common/RpWin32_sdk.h"
#  ifdef getpid
#    undef getpid
#  endif
#  define getpid() GetCurrentProcessId()
#endif /* _WIN32 */

// DEBUG: Uncomment this to force obfuscation in debug builds.
// This will use ach.bin and "RPACH10R" magic.
//#define FORCE_OBFUSCATE 1

namespace LibRpBase {

#ifdef _MSC_VER
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#endif /* _MSC_VER */

class AchievementsPrivate
{
	public:
		AchievementsPrivate();

	private:
		RP_DISABLE_COPY(AchievementsPrivate)

	public:
		// Static Achievements instance.
		// TODO: Q_GLOBAL_STATIC() equivalent, though we
		// may need special initialization if the compiler
		// doesn't support thread-safe statics.
		static Achievements instance;

	public:
		// Notification function.
		Achievements::NotifyFunc notifyFunc;
		intptr_t user_data;

	public:
		// Achievement types
		enum AchType : uint8_t {
			AT_COUNT = 0,	// Count (requires the same action X number of times)
					// For BOOLEAN achievements, set count to 1.
			AT_BITFIELD,	// Bitfield (multiple actions)

			AT_MAX
		};

		// Achievement information.
		// Array index is the ID.
		struct AchInfo_t {
			const char *name;	// Name (NOP_C_, translatable)
			const char *desc_unlk;	// Unlocked description (NOP_C_, translatable)
			AchType type;		// Achievement type
			uint8_t count;		// AT_COUNT: Number of times needed to unlock.
						// AT_BITFIELD: Number of bits. (up to 64)
						//              All bits must be 1 to unlock.
		};
		static const AchInfo_t achInfo[];

		// C++14 adds support for enum classes as unordered_map keys.
		// C++11 needs an explicit hash functor.
		struct EnumClassHash {
			inline std::size_t operator()(Achievements::ID t) const
			{
				return std::hash<int>()(static_cast<int>(t));
			}
		};

		// Active achievement data.
		struct AchData_t {
			union {
				uint8_t count;		// AT_COUNT
				uint64_t bitfield;	// AT_BITFIELD
			};
			time_t timestamp;		// Time this achievement was last updated.
		};

		// Achievement map.
		// TODO: Map vs. unordered_map for performance?
		unordered_map<Achievements::ID, AchData_t, EnumClassHash> mapAchData;
		bool loaded;	// Have achievements been loaded from disk?

		// Achievements filename and magic number.
#if defined(NDEBUG) || defined(FORCE_OBFUSCATE)
		// Release version is obfuscated.
		#define ACH_BIN_MAGIC    "RPACH10R"
		#define ACH_BIN_FILENAME "ach.bin"
#else /* !NDEBUG && !FORCE_OBFUSCATE */
		// Debug version is not obfuscated.
		#define ACH_BIN_MAGIC    "RPACH10D"
		#define ACH_BIN_FILENAME "achd.bin"
#endif /* NDEBUG || FORCE_OBFUSCATE*/

		// Serialized achievement header.
		// All fields are in little-endian.
		struct AchBinHeader {
			char magic[8];		// [0x000] "RPACH10R" or "RPACH10D"
			uint32_t length;	// [0x008] Length of remainder of file, in bytes. [excludes CRC32; includes count]
			uint32_t crc32;		// [0x00C] CRC32 of remainder of file. [includes count]
			uint32_t count;		// [0x010] Number of achievements.
		};

		// The header is followed by achievement data: (1-byte alignment, little-endian)
		// - uint16_t: Achievement ID
		// - uint8_t: Achievement type
		// - varlenint: Timestamp the achievement was last updated
		// - Data (uint8_t for AT_COUNT, varlenint for AT_BITFIELD)

		// varlenint is a variable-length value using an encoding
		// similar to MIDI variable-length values:
		// - 7 bits per byte, starting with the least-significant bits.
		// - Last byte has bit 7 clear.
		// - All other bytes have bit 7 set.
		// Examples:
		// -       0x10 -> 10
		// -       0x80 -> 80 01
		// -      0x100 -> 80 02
		// - 0x0FFFFFFF -> FF FF FF 7F

		/**
		 * Append a uint64_t to an ao::uvector<> using varlenint format.
		 * @param vec ao::uvector<>
		 * @param val Value
		 */
		static void appendVarlenInt(ao::uvector<uint8_t> &vec, uint64_t val);

		/**
		 * Parse a varlenint value.
		 * @param val	[out] Output value.
		 * @param p	[in] Data pointer.
		 * @param p_end	[in] End of data.
		 * @return Number of bytes processed, or 0 on error.
		 */
		template<typename T>
		static int parseVarlenInt(T &val, const uint8_t *p, const uint8_t *const p_end);

		/**
		 * Symmetric obfuscation function.
		 * @param iv Initialization vector.
		 * @param buf Data buffer.
		 * @param size Size. (must be a multiple of 2)
		 */
		static void doObfuscate(uint16_t iv, uint8_t *buf, size_t size);

		/**
		 * Get the achievements filename.
		 * @return Achievements filename.
		 */
		string getFilename(void) const
		{
			string filename = FileSystem::getConfigDirectory();
			if (filename.empty())
				return filename;

			if (filename.at(filename.size()-1) != DIR_SEP_CHR) {
				filename += DIR_SEP_CHR;
			}
			filename += ACH_BIN_FILENAME;
			return filename;
		}

		/**
		 * Save the achievements data.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int save(void) const;

		/**
		 * Load the achievements data.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int load(void);
};

/** AchievementsPrivate **/

// Achievement information.
const struct AchievementsPrivate::AchInfo_t AchievementsPrivate::achInfo[] =
{
	{
		NOP_C_("Achievements", "You are now a developer!"),
		NOP_C_("Achievements", "Viewed a debug-encrypted file."),
		AT_COUNT, 1
	},
	{
		NOP_C_("Achievements", "Now you're playing with POWER!"),
		NOP_C_("Achievements", "Viewed a non-x86/x64 Windows PE executable."),
		AT_COUNT, 1
	},
	{
		NOP_C_("Achievements", "Insert Startup Disc"),
		NOP_C_("Achievements", "Viewed a BroadOn format Wii WAD file."),
		AT_COUNT, 1
	},
	{
		NOP_C_("Achievements", "Knuckles & Knuckles"),
		NOP_C_("Achievements", "Viewed a copy of Sonic & Knuckles locked on to Sonic & Knuckles."),
		AT_COUNT, 1
	},
	{
		NOP_C_("Achievements", "Link, mah boi..."),
		NOP_C_("Achievements", "Viewed a CD-i disc image."),
		AT_COUNT, 1
	},
};

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
Achievements AchievementsPrivate::instance;

AchievementsPrivate::AchievementsPrivate()
	: notifyFunc(nullptr)
	, user_data(0)
	, loaded(false)
{ }

/**
 * Append a uint64_t to an ao::uvector<> using varlenint format.
 * @param vec ao::uvector<>
 * @param val Value
 */
void AchievementsPrivate::appendVarlenInt(ao::uvector<uint8_t> &vec, uint64_t val)
{
	vec.reserve(vec.size() + 8);

	while (val >= 0x80) {
		vec.push_back(0x80U | (val & 0x7FU));
		val >>= 7;
	}

	// Last byte.
	// NOTE: Masking probably isn't needed here...
	vec.push_back(val & 0x7FU);
}

/**
 * Parse a varlenint value.
 * @param val	[out] Output value.
 * @param p	[in] Data pointer.
 * @param p_end	[in] End of data.
 * @return Number of bytes processed, or 0 on error.
 */
template<typename T>
int AchievementsPrivate::parseVarlenInt(T &val, const uint8_t *p, const uint8_t *const p_end)
{
	int bytesParsed = 0;	// maximum: 5 for 32-bit, 10 for 64-bit
	uint8_t shamt = 0;	// next shift amount
	val = 0;

	bool foundLastByte = false;
	for (; p < p_end; p++, shamt += 7) {
		if (shamt >= sizeof(T)*8) {
			// Shift amount is out of range.
			return 0;
		}

		val |= static_cast<T>(*p & 0x7F) << shamt;
		bytesParsed++;

		if (!(*p & 0x80)) {
			// Last byte.
			foundLastByte = true;
			break;
		}
	}

	// If we found the last byte, this function succeeded.
	// Otherwise, it failed.
	return (foundLastByte ? bytesParsed : 0);
}

/**
 * Symmetric obfuscation function.
 * @param iv Initialization vector.
 * @param buf Data buffer.
 * @param size Size. (must be a multiple of 2)
 */
void AchievementsPrivate::doObfuscate(uint16_t iv, uint8_t *buf, size_t size)
{
	assert(size % 2 == 0);
	uint16_t *buf16 = reinterpret_cast<uint16_t*>(buf);

	// Based on 16-stage LFSR, similar to SMS SN76489.
	uint16_t lfsr = 0x8000;
	// Run for 32 cycles to initialize the LFSR.
	for (unsigned int i = 32; i > 0; i--) {
		// Bits 3 and 0 are XOR'd to form the next input.
		const unsigned int n = ((lfsr & 0x08) >> 3) ^ (lfsr & 0x01);
		lfsr >>= 1;
		lfsr |= (n << 15);
	}

	for (; size > 1; buf16++, size -= 2, iv++) {
		uint16_t data = ~(*buf16);
		data ^= lfsr;
		if (size & 4) {
			data ^= 0x5A5A;
		} else {
			data ^= 0xA5A5;
		}
		data ^= iv;
		*buf16 = data;

		// Bits 3 and 0 are XOR'd to form the next input.
		const unsigned int n = ((lfsr & 0x08) >> 3) ^ (lfsr & 0x01);
		lfsr >>= 1;
		lfsr |= (n << 15);
	}
}

/**
 * Save the achievements data.
 * @return 0 on success; negative POSIX error code on error.
 */
int AchievementsPrivate::save(void) const
{
#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		// Can't calculate the CRC32.
		return -ENOTSUP;
	}
#else /* !defined(_MSC_VER) || !defined(ZLIB_IS_DLL) */
	// zlib isn't in a DLL, but we need to ensure that the
	// CRC table is initialized anyway.
	get_crc_table();
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// Create the achievements file in memory.
	ao::uvector<uint8_t> buf;
	buf.reserve(sizeof(AchBinHeader) + ((int)Achievements::ID::Max * 12));
	buf.resize(sizeof(AchBinHeader));

	// Header
	AchBinHeader *header = reinterpret_cast<AchBinHeader*>(buf.data());
	memcpy(header->magic, ACH_BIN_MAGIC, sizeof(header->magic));
	header->length = 0;
	header->crc32 = 0;
	header->count = cpu_to_le32(static_cast<uint32_t>(mapAchData.size()));

	// Process all achievements in order of ID.
	for (unsigned int i = 0; i < (int)Achievements::ID::Max; i++) {
		auto iter = mapAchData.find((Achievements::ID)i);
		if (iter == mapAchData.end())
			continue;

		switch (achInfo[i].type) {
			case AT_COUNT: {
				buf.reserve(buf.size()+3+8+1);
				buf.resize(buf.size()+3);
				uint8_t *const p = &buf.data()[buf.size()-3];
				// uint16_t: Achievement ID
				p[0] = (i & 0xFF);
				p[1] = (i >> 8) & 0xFF;
				// uint8_t: Achievement type
				p[2] = AT_COUNT;
				// varlenint: Timestamp
				appendVarlenInt(buf, iter->second.timestamp);
				// uint8_t: Count
				buf.push_back(iter->second.count);
				break;
			}

			case AT_BITFIELD: {
				// TODO: NEEDS TESTING!!!
				buf.reserve(buf.size()+3+8+8);
				buf.resize(buf.size()+3);
				uint8_t *p = &buf.data()[buf.size()-3];
				// uint16_t: Achievement ID
				p[0] = (i & 0xFF);
				p[1] = (i >> 8) & 0xFF;
				// uint8_t: Achievement type
				p[2] = AT_BITFIELD;
				// varlenint: Timestamp
				appendVarlenInt(buf, iter->second.timestamp);
				// varlenint: Bitfield
				appendVarlenInt(buf, iter->second.bitfield);
				break;
			}

			default:
				assert(!"Unsupported achievement.");
				break;
		}
	}

	// NOTE: buf may have been reallocated, so we need to
	// get the header pointer again.
	header = reinterpret_cast<AchBinHeader*>(buf.data());

	static const size_t HeaderSizeMinusCount = sizeof(AchBinHeader) - sizeof(header->count);

	// Length of achievement data.
	// Includes count, but not crc32.
	header->length = static_cast<uint32_t>(buf.size() - HeaderSizeMinusCount);

	// CRC32 of achievement data.
	// Includes count.
	if (buf.size() > HeaderSizeMinusCount) {
		header->crc32 = cpu_to_le32(
			crc32(0, &buf.data()[HeaderSizeMinusCount],
			      static_cast<uInt>(buf.size() - HeaderSizeMinusCount)));
	}

#if defined(NDEBUG) || defined(FORCE_OBFUSCATE)
	// Obfuscate the data.
	if (buf.size() % 2 != 0) {
		buf.push_back(0);
	};
	union {
		uint16_t u16[2];
		uint8_t u8[4];
	} iv;
	iv.u16[0] = (getpid() ^ time(nullptr)) & 0xFFFF;
	iv.u16[1] = 0xFFFF - iv.u16[0];
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
	iv.u16[0] = cpu_to_le16(iv.u16[0]);
	iv.u16[1] = cpu_to_le16(iv.u16[1]);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */
	const size_t ivpos = buf.size();
	buf.resize(ivpos+4);
	memcpy(&buf.data()[ivpos], iv.u8, sizeof(iv.u8));

	doObfuscate(iv.u16[0], buf.data(), ivpos);
#endif /* NDEBUG || FORCE_OBFUSCATE */

	// Write the achievements file.
	const string filename = getFilename();
	if (filename.empty()) {
		// Unable to get the filename.
		return -EIO;
	}

	RpFile *const file = new RpFile(filename, RpFile::FM_CREATE_WRITE);
	if (!file->isOpen()) {
		int ret = -file->lastError();
		if (ret == 0) {
			ret = -EIO;
		}
		file->unref();
		return ret;
	}
	size_t size = file->write(buf.data(), buf.size());
	if (size != buf.size()) {
		// Short write.
		// TODO: Delete the file?
		int ret = -file->lastError();
		if (ret == 0) {
			ret = -EIO;
		}
		file->unref();
		return ret;
	}
	file->unref();

	// Achievements written.
	const_cast<AchievementsPrivate*>(this)->loaded = true;
	return 0;
}

/**
 * Load the achievements data.
 * @return 0 on success; negative POSIX error code on error.
 */
int AchievementsPrivate::load(void)
{
#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		// Can't calculate the CRC32.
		return -ENOTSUP;
	}
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// Clear loaded achievements.
	loaded = false;
	mapAchData.clear();

	// Load the achievements file in memory.
	const string filename = getFilename();
	if (filename.empty()) {
		// Unable to get the filename.
		return -EIO;
	}

	RpFile *const file = new RpFile(filename, RpFile::FM_OPEN_READ);
	if (!file->isOpen()) {
		int ret = -file->lastError();
		if (ret == 0) {
			ret = -EIO;
		}
		file->unref();
		return ret;
	}

	const off64_t fileSize = file->size();
	if (fileSize > 1*1024*1024) {
		// 1 MB is probably way too much...
		file->unref();
		return -ENOMEM;
	}

	ao::uvector<uint8_t> buf;
	buf.resize(static_cast<size_t>(fileSize));
	size_t size = file->read(buf.data(), buf.size());
	if (size != buf.size()) {
		int ret = -file->lastError();
		if (ret == 0) {
			ret = -EIO;
		}
		file->unref();
		return ret;
	}
	file->unref();

#if defined(NDEBUG) || defined(FORCE_OBFUSCATE)
	// Deobfuscate the data.
	if (buf.size() < 4) {
		// IV is missing.
		return -EIO;
	} else if (buf.size() % 2 != 0) {
		// IV should've padded it.
		return -EIO;
	};

	union {
		uint16_t u16[2];
		uint8_t u8[4];
	} iv;
	memcpy(iv.u8, &buf.data()[buf.size()-4], sizeof(iv.u8));
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
	iv.u16[0] = le16_to_cpu(iv.u16[0]);
	iv.u16[1] = le16_to_cpu(iv.u16[1]);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */

	if (iv.u16[1] != (0xFFFF - iv.u16[0])) {
		// Incorrect iv1.
		return -EIO;
	}

	buf.resize(buf.size()-4);
	doObfuscate(iv.u16[0], buf.data(), buf.size());
#endif /* NDEBUG || FORCE_OBFUSCATE */

	// Check the header.
	const AchBinHeader *header = reinterpret_cast<const AchBinHeader*>(buf.data());
	if (memcmp(header->magic, ACH_BIN_MAGIC, sizeof(header->magic)) != 0) {
		// Incorrect header.
		return -EBADF;
	}

	// Length should be >= HeaderSizeMinusCount, and less than 1 MB.
	static const size_t HeaderSizeMinusCount = sizeof(AchBinHeader) - sizeof(header->count);
	const uint32_t data_len = le32_to_cpu(header->length);
	if (data_len < sizeof(header->count) ||
	    data_len >= ((1*1024*1024)-HeaderSizeMinusCount) ||
	    data_len + HeaderSizeMinusCount > buf.size())
	{
		// Incorrect length.
		return -EBADF;
	}

	// Resize the buffer to get rid of any extra data.
	buf.resize(data_len + HeaderSizeMinusCount);

	// Verify the CRC32.
	const uint32_t crc = crc32(0, &buf.data()[HeaderSizeMinusCount], data_len);
	if (crc != le32_to_cpu(header->crc32)) {
		// Incorrect CRC32.
		return -EBADF;
	}

	// NOTE: buf may have been reallocated, so we need to
	// get the header pointer again.
	header = reinterpret_cast<const AchBinHeader*>(buf.data());

	// Process all achievements.
	bool ok = true;
	const uint8_t *p = &buf.data()[sizeof(AchBinHeader)];
	const uint8_t *const p_end = &buf.data()[buf.size()];
	uint32_t ach_count = le32_to_cpu(header->count);
	for (; ok && p < p_end-3 && ach_count > 0; ach_count--) {
		// Achievement ID.
		const uint16_t id = p[0] | (p[1] << 8);
		bool isIDok = true;
		if (id >= (int)Achievements::ID::Max) {
			// Invalid ID.
			// NOTE: Allowing this in case the user is downgrading
			// from a newer version that has more achievements.
			// NOTE: We can't validate the type if the achievement ID is out of range.
			assert(!"Achievement ID is out of range.");
			isIDok = false;
		} else {
			// Make sure the type is correct.
			if (achInfo[id].type != p[2]) {
				// Incorrect type.
				assert(!"Achievement type is incorrect for specified ID.");
				ok = false;
				break;
			}
		}

		// Check for duplicates.
		// If found, ignore this.
		auto iter = mapAchData.find((Achievements::ID)id);
		if (iter != mapAchData.end()) {
			// Found a duplicate.
			assert(!"Duplicate achievement found!");
			ok = false;
			break;
		}

		// Type byte.
		const uint8_t type = p[2];

		// Timestamp.
		int64_t timestamp;
		int bytesParsed = parseVarlenInt(timestamp, &p[3], p_end);
		if (bytesParsed == 0) {
			assert(!"Not enough bytes for the timestamp.");
			ok = false;
			break;
		}

		p += 3 + bytesParsed;
		if (p >= p_end) {
			assert(!"Not enough bytes for the achievement data.");
			ok = false;
			break;
		}

		// Check the type byte.
		switch (type) {
			case AT_COUNT: {
				if (isIDok) {
					mapAchData[(Achievements::ID)id].timestamp = static_cast<time_t>(timestamp);
					mapAchData[(Achievements::ID)id].count = *p;
				}
				p++;
				break;
			}

			case AT_BITFIELD: {
				// TODO: NEEDS TESTING!!!

				// Parse the bitfield as a varlenint.
				uint64_t bitfield;
				bytesParsed = parseVarlenInt(bitfield, p, p_end);
				if (bytesParsed == 0) {
					// Out of bytes.
					assert(!"Not enough data bytes for AT_BITFIELD.");
					ok = false;
					break;
				}
				p += bytesParsed;

				if (isIDok) {
					mapAchData[(Achievements::ID)id].timestamp = static_cast<time_t>(timestamp);
					mapAchData[(Achievements::ID)id].bitfield = bitfield;
				}
				break;
			}

			default:
				// Unhandled achievement type.
				// This is an error!
				assert(!"Unhandled achievement type.");
				ok = false;
				break;
		}
	}

	if (!ok) {
		// An error occurred while loading achievements.
		loaded = false;
		mapAchData.clear();
		return -EIO;
	}

	// Achievements loaded.
	loaded = true;
	return 0;
}

/** Achievements **/

Achievements::Achievements()
	: d_ptr(new AchievementsPrivate())
{
	// TODO: Load achievements.
}

Achievements::~Achievements()
{
	delete d_ptr;
	d_ptr = nullptr;
}

/**
 * Get the Achievements instance.
 *
 * This automatically initializes the object and
 * reloads the achievements data if it has been modified.
 *
 * @return Achievements instance.
 */
Achievements *Achievements::instance(void)
{
	// Initialize the singleton instance.
	Achievements *const q = &AchievementsPrivate::instance;

	// TODO: Load achievements.
	RP_UNUSED(q);

	// Return the singleton instance.
	return q;
}

/**
 * Set the notification function.
 * This is used for the UI frontends.
 * @param func Notification function.
 * @param user_data User data.
 */
void Achievements::setNotifyFunction(NotifyFunc func, intptr_t user_data)
{
	RP_D(Achievements);
	if (d) {
		d->notifyFunc = func;
		d->user_data = user_data;
	}
}

/**
 * Unregister a notification function if set.
 *
 * If both func and user_data match the existing values,
 * then both are cleared.
 *
 * @param func Notification function.
 * @param user_data User data.
 */
void Achievements::clearNotifyFunction(NotifyFunc func, intptr_t user_data)
{
	RP_D(Achievements);
	if (d && d->notifyFunc == func && d->user_data == user_data) {
		d->notifyFunc = nullptr;
		d->user_data = 0;
	}
}

/**
 * Unlock an achievement.
 * @param id Achievement ID.
 * @param bit Bitfield index for AT_BITFIELD achievements.
 * @return 0 on success; negative POSIX error code on error.
 */
int Achievements::unlock(ID id, int bit)
{
#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		// We won't be able to calculate CRC32s, so don't
		// enable achievements at all.
		return -ENOTSUP;
	}
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// If this achievement is bool/count, increment the value.
	// If the value has hit the maximum, achievement is unlocked.
	assert((int)id >= 0);
	assert(id < ID::Max);
	if ((int)id < 0 || id >= ID::Max) {
		// Invalid achievement ID.
		return -EINVAL;
	}

	// Make sure achievements have been loaded.
	RP_D(Achievements);
	if (!d->loaded) {
		d->load();
	}

	// Check the type.
	bool unlocked = false;
	const AchievementsPrivate::AchInfo_t &achInfo = d->achInfo[(int)id];
	AchievementsPrivate::AchData_t &achData = d->mapAchData[id];
	switch (achInfo.type) {
		default:
			assert(!"Achievement type not supported.");
			return -EINVAL;

		case AchievementsPrivate::AT_COUNT: {
			// Check if we've already reached the required count.
			uint8_t count = achData.count;
			if (count >= achInfo.count) {
				// Count has been reached.
				// Achievement is already unlocked.
				return 0;
			}

			// Increment the count.
			count++;
			achData.count = count;
			achData.timestamp = time(nullptr);
			if (count >= achInfo.count) {
				// Achievement unlocked!
				unlocked = true;
			}
			break;
		}

		case AchievementsPrivate::AT_BITFIELD: {
			// Bitfield value.
			assert(bit >= 0);
			assert(bit < achInfo.count);
			if (bit < 0 || bit >= achInfo.count) {
				// Invalid bit index.
				return -EINVAL;
			}

			// Check if we've already filled the bitfield.
			// TODO: Verify 32-bit and 64-bit operation for values 32 and 64.
			const uint64_t bf_filled = (1ULL << achInfo.count) - 1;
			uint64_t bf_value = achData.bitfield;
			if (bf_value == bf_filled) {
				// Bitfield is already filled.
				// Achievement is already unlocked.
				return 0;
			}

			// Set the bit.
			const uint64_t bf_new = bf_value | (1ULL << (unsigned int)bit);
			if (bf_new == bf_value) {
				// No change.
				return 0;
			}

			achData.bitfield = bf_new;
			achData.timestamp = time(nullptr);
			if (bf_new == bf_filled) {
				// Achievement unlocked!
				unlocked = true;
			}
			break;
		}
	}

	// Save the achievement data.
	d->save();

	if (unlocked) {
		// Achievement unlocked!
		if (d->notifyFunc) {
			d->notifyFunc(d->user_data, id);
		}
	}

	return 0;
}

/**
 * Check if an achievement is unlocked.
 * @param id Achievement ID.
 * @return UNIX time value if unlocked; -1 if not.
 */
time_t Achievements::isUnlocked(ID id) const
{
#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		// We won't be able to calculate CRC32s, so don't
		// enable achievements at all.
		return -1;
	}
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// If this achievement is bool/count, increment the value.
	// If the value has hit the maximum, achievement is unlocked.
	assert((int)id >= 0);
	assert(id < ID::Max);
	if ((int)id < 0 || id >= ID::Max) {
		// Invalid achievement ID.
		return -1;
	}

	// Make sure achievements have been loaded.
	RP_D(const Achievements);
	if (!d->loaded) {
		const_cast<AchievementsPrivate*>(d)->load();
	}

	// Check the type.
	time_t timestamp = -1;
	const AchievementsPrivate::AchInfo_t *const achInfo = &d->achInfo[(int)id];
	switch (achInfo->type) {
		default:
			assert(!"Achievement type not supported.");
			return -EINVAL;

		case AchievementsPrivate::AT_COUNT: {
			// Check if we've already reached the required count.
			auto iter = d->mapAchData.find(id);
			if (iter != d->mapAchData.end()) {
				const auto &ach = iter->second;
				if (ach.count >= achInfo->count) {
					timestamp = ach.timestamp;
				}
			}
			break;
		}

		case AchievementsPrivate::AT_BITFIELD: {
			// Check if we've already filled the bitfield.
			// TODO: Verify 32-bit and 64-bit operation for values 32 and 64.
			auto iter = d->mapAchData.find(id);
			if (iter != d->mapAchData.end()) {
				const auto &ach = iter->second;
				const uint64_t bf_filled = (1ULL << achInfo->count) - 1;
				if (bf_filled == ach.bitfield) {
					timestamp = ach.timestamp;
				}
			}
			break;
		}
	}

	return timestamp;
}

/**
 * Get an achievement name. (localized)
 * @param id Achievement ID.
 * @return Achievement description, or nullptr on error.
 */
const char *Achievements::getName(ID id) const
{
	assert((int)id >= 0);
	assert(id < ID::Max);
	if ((int)id < 0 || id >= ID::Max) {
		// Invalid achievement ID.
		return nullptr;
	}

	RP_D(const Achievements);
	const AchievementsPrivate::AchInfo_t *const achInfo = &d->achInfo[(int)id];
	return dpgettext_expr(RP_I18N_DOMAIN, "Achievements", achInfo->name);
}

/**
 * Get an unlocked achievement description. (localized)
 * @param id Achievement ID.
 * @return Unlocked achievement description, or nullptr on error.
 */
const char *Achievements::getDescUnlocked(ID id) const
{
	assert((int)id >= 0);
	assert(id < ID::Max);
	if ((int)id < 0 || id >= ID::Max) {
		// Invalid achievement ID.
		return nullptr;
	}

	RP_D(const Achievements);
	const AchievementsPrivate::AchInfo_t *const achInfo = &d->achInfo[(int)id];
	return dpgettext_expr(RP_I18N_DOMAIN, "Achievements", achInfo->desc_unlk);
}

}
