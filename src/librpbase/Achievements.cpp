/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * Achievements.cpp: Achievements class.                                   *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Achievements.hpp"
#include "libi18n/i18n.h"

// librpcpu, librpfile
#include "librpcpu/byteswap.h"
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
# include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

#ifdef _WIN32
// Win32 needed for GetCurrentProcessId().
# include "libwin32common/RpWin32_sdk.h"
#endif /* _WIN32 */

// DEBUG: Uncomment this to force obfuscation in debug builds.
// This will use ach.bin and "RPACH10R" magic.
//#define FORCE_OBFUSCATE 1

namespace LibRpBase {

#ifdef _MSC_VER
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL0(zlibVersion);
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

		// Have achievements been loaded from disk?
		bool loaded;

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
			const char *desc;	// Description (NOP_C_, translatable)
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
		// Two maps: One for boolean/count, one for bitfield.
		// TODO: Map vs. unordered_map for performance?
		unordered_map<Achievements::ID, uint8_t, EnumClassHash> mapAchData_count;
		unordered_map<Achievements::ID, uint64_t, EnumClassHash> mapAchData_bitfield;

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
			uint32_t length;	// [0x00C] Length of remainder of file, in bytes. [includes count]
			uint32_t crc32;		// [0x010] CRC32 of remainder of file. [includes count]
			uint32_t count;		// [0x008] Number of achievements.
		};

		// The header is followed by achievement data: (1-byte alignment, little-endian)
		// - uint16_t: Achievement ID
		// - uint8_t: Achievement type
		// - Data (uint8_t for AT_COUNT, uint64_t for AT_BITFIELD)

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
		// tr: ViewedDebugCryptedFile (name)
		NOP_C_("Achievements", "You are now a developer!"),
		// tr: ViewedDebugCryptedFile (description)
		NOP_C_("Achievements", "Viewed a debug-encrypted file."),
		AT_COUNT, 1
	},
	{
		// tr: ViewedNonX86PE (name)
		NOP_C_("Achievements", "Now you're playing with POWER!"),
		// tr: ViewedNonX86PE (description)
		NOP_C_("Achievements", "Viewed a non-x86/x64 Windows PE executable."),
		AT_COUNT, 1
	},
	{
		// tr: ViewedBroadOnWADFile (name)
		NOP_C_("Achievements", "Insert Startup Disc"),
		// tr: ViewedBroadOnWADFile (description)
		NOP_C_("Achievements", "Viewed a BroadOn format Wii WAD file."),
		AT_COUNT, 1
	},
	{
		// tr: ViewedMegaDriveSKwithSK (name)
		NOP_C_("Achievements", "Knuckles & Knuckles"),
		// tr: ViewedMegaDriveSKwithSK (description)
		NOP_C_("Achievements", "Viewed a copy of Sonic & Knuckles locked on to Sonic & Knuckles."),
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
		unsigned int n = ((lfsr & 0x08) >> 3) ^ (lfsr & 0x01);
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
		unsigned int n = ((lfsr & 0x08) >> 3) ^ (lfsr & 0x01);
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
	if (DelayLoad_test_zlibVersion() != 0) {
		// Delay load failed.
		// Can't calculate the CRC32.
		return -ENOTSUP;
	}
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// Create the achievements file in memory.
	ao::uvector<uint8_t> buf;
	buf.reserve(sizeof(AchBinHeader) + ((int)Achievements::ID::Max * 12));
	buf.resize(sizeof(AchBinHeader));

	// Header.
	AchBinHeader *const header = reinterpret_cast<AchBinHeader*>(buf.data());
	memcpy(header->magic, ACH_BIN_MAGIC, sizeof(header->magic));
	header->length = 0;
	header->crc32 = 0;
	header->count = cpu_to_le32(static_cast<uint32_t>(mapAchData_count.size() + mapAchData_bitfield.size()));

	// Process all achievements in order of ID.
	for (unsigned int i = 0; i < (int)Achievements::ID::Max; i++) {
		switch (achInfo[i].type) {
			case AT_COUNT: {
				auto iter = mapAchData_count.find((Achievements::ID)i);
				if (iter == mapAchData_count.end())
					break;
				// Found achievement data. Write it out.
				buf.resize(buf.size()+4);
				uint8_t *const p = &buf.data()[buf.size()-4];
				// uint16_t: Achievement ID
				p[0] = (i & 0xFF);
				p[1] = (i >> 8) & 0xFF;
				// uint8_t: Achievement type
				p[2] = AT_COUNT;
				// uint8_t: Count
				p[3] = iter->second;
				break;
			}

			case AT_BITFIELD: {
				// TODO: NEEDS TESTING!!!
				auto iter = mapAchData_bitfield.find((Achievements::ID)i);
				if (iter == mapAchData_bitfield.end())
					break;
				// Found achievement data. Write it out.
				buf.resize(buf.size()+4);
				uint8_t *p = &buf.data()[buf.size()-4];
				// uint16_t: Achievement ID
				p[0] = (i & 0xFF);
				p[1] = (i >> 8) & 0xFF;
				// uint8_t: Achievement type
				p[2] = AT_COUNT;
				// uint64_t: Bitfield
				// NOTE: Writing a byte at a time to prevent alignment issues.
				p += 3;
				uint64_t val = iter->second;
				for (unsigned int n = 8; n > 0; n--, val >>= 8, p++) {
					*p = (val & 0xFF);
				}
				break;
			}

			default:
				assert(!"Unsupported achievement.");
				break;
		}
	}

	static const size_t HeaderSizeMinusCount = sizeof(AchBinHeader) - sizeof(header->count);

	// Length of achievement data.
	// Includes count, but not crc32.
	header->length = buf.size() - HeaderSizeMinusCount;

	// CRC32 of achievement data.
	// Includes count.
	if (buf.size() > HeaderSizeMinusCount) {
		header->crc32 = cpu_to_le32(crc32(0, &buf.data()[HeaderSizeMinusCount], buf.size() - HeaderSizeMinusCount));
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
#ifdef _WIN32
	iv.u16[0] = (GetCurrentProcessId() ^ time(nullptr)) & 0xFFFF;
#else /* !_WIN32 */
	iv.u16[0] = (getpid() ^ time(nullptr)) & 0xFFFF;
#endif /* _WIN32 */
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
	string filename = getFilename();
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
		return -EIO;
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
	if (DelayLoad_test_zlibVersion() != 0) {
		// Delay load failed.
		// Can't calculate the CRC32.
		return -ENOTSUP;
	}
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// Clear loaded achievements.
	loaded = false;
	mapAchData_count.clear();
	mapAchData_bitfield.clear();

	// Load the achievements file in memory.
	string filename = getFilename();
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

	const int64_t fileSize = file->size();
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
		printf("IV1 ERR\n");
		return -EIO;
	}

	buf.resize(buf.size()-4);
	doObfuscate(iv.u16[0], buf.data(), buf.size());
#endif /* NDEBUG || FORCE_OBFUSCATE */

	// Check the header.
	AchBinHeader *const header = reinterpret_cast<AchBinHeader*>(buf.data());
	if (memcmp(header->magic, ACH_BIN_MAGIC, sizeof(header->magic)) != 0) {
		// Incorrect header.
		return -EBADF;
	}

	// Length should be >= HeaderSizeMinusCount, and less than 1 MB.
	static const size_t HeaderSizeMinusCount = sizeof(AchBinHeader) - sizeof(header->count);
	const uint32_t data_len = le32_to_cpu(header->length);
	if (data_len < sizeof(header->count) ||
	    data_len >= ((1*1024*1024)-HeaderSizeMinusCount) ||
	    data_len + HeaderSizeMinusCount < buf.size())
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

	// Process all achievements.
	bool ok = true;
	const uint8_t *p = &buf.data()[sizeof(AchBinHeader)];
	const uint8_t *const p_end = &buf.data()[buf.size()];
	uint32_t ach_count = le32_to_cpu(header->count);
	for (; ok && p+3 < p_end && ach_count > 0; ach_count--) {
		// Achievement ID.
		const uint16_t id = p[0] | (p[1] << 8);
		if (id >= (int)Achievements::ID::Max) {
			// Invalid ID.
			// NOTE: Allowing this in case the user is downgrading
			// from a newer version that has more achievements.
			// NOTE: We can't validate the type if the achievement ID is out of range.
			assert(!"Achievement ID is out of range.");
		} else {
			// Make sure the type is correct.
			if (achInfo[id].type != p[2]) {
				// Incorrect type.
				assert(!"Achievement type is incorrect for specified ID.");
				ok = false;
				break;
			}
		}

		// Check the type byte.
		switch (p[2]) {
			case AT_COUNT: {
				// Check for duplicates.
				// If found, ignore this.
				auto iter = mapAchData_count.find((Achievements::ID)id);
				if (iter != mapAchData_count.end()) {
					// Found a duplicate.
					assert(!"Duplicate AT_COUNT achievement found!");
					p += 4;
					break;
				}
				// Save the achievement count.
				mapAchData_count[(Achievements::ID)id] = p[3];
				p += 4;
				break;
			}

			case AT_BITFIELD: {
				// TODO: NEEDS TESTING!!!
				// Check for duplicates.
				// If found, ignore this.
				auto iter = mapAchData_bitfield.find((Achievements::ID)id);
				if (iter != mapAchData_bitfield.end()) {
					// Found a duplicate.
					assert(!"Duplicate AT_BITFIELD achievement found!");
					p += 8;
					break;
				}
				// Make sure we have 8 bytes available.
				if (p+3+8 >= p_end) {
					// Out of range.
					assert(!"Not enough data bytes for AT_BITFIELD.");
					ok = false;
					break;
				}

				// Convert the uint64_t.
				uint64_t val = 0;
				p += 3;
				for (unsigned int n = 8; n > 0; n--, p++) {
					val <<= 8;
					val |= *p;
				}

				// Save the achievement bitfield.
				mapAchData_bitfield[(Achievements::ID)id] = val;
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
	d->notifyFunc = func;
	d->user_data = user_data;
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
	if (d->notifyFunc == func && d->user_data == user_data) {
		d->notifyFunc = nullptr;
		d->user_data = 0;
	}
}

/**
 * Unlock an achievement.
 * @param id Achievement ID.
 * @param bit Bitfield index for AT_BITFIELD achievements.
 */
void Achievements::unlock(ID id, int bit)
{
	RP_D(Achievements);

#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_zlibVersion() != 0) {
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
		return;
	}

	// Make sure achievements have been loaded.
	if (!d->loaded) {
		d->load();
	}

	// Check the type.
	bool unlocked = false;
	const AchievementsPrivate::AchInfo_t *const achInfo = &d->achInfo[(int)id];
	switch (achInfo->type) {
		default:
			assert(!"Achievement type not supported.");
			return;

		case AchievementsPrivate::AT_COUNT: {
			// Check if we've already reached the required count.
			uint8_t count = d->mapAchData_count[id];
			if (count >= achInfo->count) {
				// Count has been reached.
				// Achievement is already unlocked.
				return;
			}

			// Increment the count.
			count++;
			d->mapAchData_count[id] = count;
			if (count >= achInfo->count) {
				// Achievement unlocked!
				unlocked = true;
			}
			break;
		}

		case AchievementsPrivate::AT_BITFIELD: {
			// Bitfield value.
			assert(bit >= 0);
			assert(bit < achInfo->count);
			if (bit < 0 || bit >= achInfo->count) {
				// Invalid bit index.
				return;
			}

			// Check if we've already filled the bitfield.
			// TODO: Verify 32-bit and 64-bit operation for values 32 and 64.
			const uint64_t bf_filled = (1ULL << achInfo->count) - 1;
			uint64_t bf_value = d->mapAchData_bitfield[id];
			if (bf_value == bf_filled) {
				// Bitfield is already filled.
				// Achievement is already unlocked.
				return;
			}

			// Set the bit.
			uint64_t bf_new = bf_value | (1 << (unsigned int)bit);
			if (bf_new == bf_value) {
				// No change.
				return;
			}

			d->mapAchData_bitfield[id] = bf_new;
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
			d->notifyFunc(d->user_data,
				dpgettext_expr(RP_I18N_DOMAIN, "Achievements", achInfo->name),
				dpgettext_expr(RP_I18N_DOMAIN, "Achievements", achInfo->desc));
		}
	}
}

}
