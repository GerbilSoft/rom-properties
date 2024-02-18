/***************************************************************************
 * ROM Properties Page shell extension. (amiibo-data)                      *
 * amiiboc.cpp: Nintendo amiibo binary data compiler.                      *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "amiibo_bin_structs.h"

// librpsecure
#include "librpsecure/os-secure.h"
#include "librpsecure/restrict-dll.h"

// Byteswapping
#include "byteswap_rp.h"
// Unsigned ctype
#include "ctypex.h"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
using std::map;
using std::string;
using std::unordered_map;
using std::vector;

static bool verbose = false;

// NOTE: Tables in memory are HOST-endian.
// They will be swapped to little-endian when written to disk.

// String table.
static vector<char> stringTable;
static unordered_map<string, uint32_t> stringTableMap;		// string table indexes

// amiibo data.
static AmiiboBinHeader binHeader;
static vector<uint32_t> charSeriesTable;			// index is char series ID; value is string table offset
static map<uint16_t, CharTableEntry> charTable;			// key is char ID

// Character Variant table:
// key: char ID
// value: map with key=variant ID
typedef map<uint8_t, CharVariantTableEntry> CharVariantMap;
static map<uint16_t, CharVariantMap> charVarTable;

static vector<uint32_t> amiiboSeriesTable;			// index is amiibo series ID; value is string table offset
static vector<AmiiboIDTableEntry> amiiboTable;			// index is amiibo ID

/**
 * Get the string table offset for the specified string.
 *
 * If the string is found in the string table, the existing offset
 * will be returned.
 *
 * If the string is not found in the string table, it will be added
 * and the new string offset will be returned.
 *
 * @param str String to look up.
 * @return String table offset.
 */
static uint32_t getStringTableOffset(const char *str)
{
	if (!str || str[0] == '\0') {
		// Empty string.
		return 0;
	}

	// Check if the string is already in the string table.
	auto iter = stringTableMap.find(str);
	if (iter != stringTableMap.end()) {
		// Found the string.
		return iter->second;
	}

	// Not found. Add the string
	const uint32_t offset = static_cast<uint32_t>(stringTable.size());
	string entry(str);
	stringTable.insert(stringTable.end(), entry.c_str(), entry.c_str() + entry.size() + 1);
	stringTableMap.emplace(std::move(entry), offset);
	return offset;
}

/**
 * Align a file to 16 bytes by writing NULL bytes.
 * @param f File.
 */
static void alignFileTo16Bytes(FILE *f)
{
	const unsigned int offset_mod16 = static_cast<unsigned int>(ftello(f) % 16);
	if (offset_mod16 == 0)
		return;

	uint8_t zerobytes[16];
	memset(zerobytes, 0, sizeof(zerobytes));
	const unsigned int count = 16 - offset_mod16;
	fwrite(zerobytes, 1, count, f);
}

/**
 * Set security options.
 * @return 0 on success; non-zero on error.
 */
static int set_security_options(void)
{
	// Restrict DLL lookups.
	// NOTE: Not checking the return value.
	rp_secure_restrict_dll_lookups();

	// Set OS-specific security options.
	rp_secure_param_t param;
#if defined(_WIN32)
	param.bHighSec = 1;	// Disable Win32k syscalls. (NTUser/GDI)
#elif defined(HAVE_SECCOMP)
	static const int syscall_wl[] = {
		// Syscalls used by amiiboc.
		SCMP_SYS(close),
		SCMP_SYS(fstat),     SCMP_SYS(fstat64),		// __GI___fxstat() [printf()]
		SCMP_SYS(fstatat64), SCMP_SYS(newfstatat),	// Ubuntu 19.10 (32-bit)
		SCMP_SYS(gettimeofday),	// 32-bit only?
		SCMP_SYS(lseek), SCMP_SYS(_llseek),
		SCMP_SYS(open),		// Ubuntu 16.04
		SCMP_SYS(openat),	// glibc-2.31
#if defined(__SNR_openat2)
		SCMP_SYS(openat2),	// Linux 5.6
#elif defined(__NR_openat2)
		__NR_openat2,		// Linux 5.6
#endif /* __SNR_openat2 || __NR_openat2 */

		-1	// End of whitelist
	};
	param.syscall_wl = syscall_wl;
	param.threading = false;
#elif defined(HAVE_PLEDGE)
	// Promises:
	// - stdio: General stdio functionality.
	// - rpath: Read from the specified file.
	// - wpath: Write to the specified file.
	param.promises = "stdio rpath wpath";
#elif defined(HAVE_TAME)
	param.tame_flags = TAME_STDIO | TAME_RPATH | TAME_WPATH;
#else
	param.dummy = 0;
#endif

	return rp_secure_enable(param);
}

int main(int argc, char *argv[])
{
	// Set security options.
	set_security_options();

	// TODO: Better command line parsing.
	if (argc < 3) {
		fprintf(stderr, "syntax: %s [-v] amiibo-data.txt amiibo.bin\n", argv[0]);
		return EXIT_FAILURE;
	}

	int optind = 1;
	if (!strcmp(argv[optind], "-v")) {
		// Verbose mode.
		verbose = true;
		optind++;
	}

	FILE *f_in = fopen(argv[optind++], "r");
	if (!f_in) {
		fprintf(stderr, "*** ERROR opening input file '%s': %s\n", argv[1], strerror(errno));
		return EXIT_FAILURE;
	}

	// Initialize the header and tables.
	memset(&binHeader, 0, sizeof(binHeader));
	charSeriesTable.reserve(0x3A4/4);
	amiiboSeriesTable.reserve(32);
	amiiboTable.reserve(0x1000);

	// Initialize the string table.
	// The string table always starts with a NULL byte. (empty string)
	stringTable.reserve(32768);
	stringTableMap.reserve(2048);
	stringTable.push_back('\0');
	stringTableMap.emplace("", 0);

	// Parse the file.
	bool foundHeader = false;	// true if we found the header line
	bool err = false;
	int line = 1;
	char buf[256];
	for (; fgets(buf, sizeof(buf), f_in) != nullptr; line++) {
		// Remove leading and trailing whitespace.
		const char *const buf_end = buf + sizeof(buf);
		char *p = buf;
		while (p < buf_end && *p != '\0' && ISSPACE(*p)) {
			p++;
		}
		if (*p == '#') {
			// Comment line.
			continue;
		}
		size_t plen = strlen(p);
		while (plen > 0) {
			if (!ISSPACE(p[plen-1]))
				break;

			p[plen-1] = '\0';
			plen--;
		}
		if (plen == 0)
			continue;

		// Tokenize the line.
		char *saveptr;
		const char *token = strtok_r(p, ":", &saveptr);
		assert(token != nullptr);
		if (!token)
			continue;

		if (!strcmp(token, AMIIBO_BIN_MAGIC)) {
			if (foundHeader) {
				// Duplicate file header.
				fprintf(stderr, "*** ERROR: Line %d: Duplicate %s header.\n", line, AMIIBO_BIN_MAGIC);
				err = true;
				break;
			} else {
				// Found the file header.
				memcpy(binHeader.magic, token, sizeof(binHeader.magic));
				foundHeader = true;
				continue;
			}
		}

		// For anything else, the header must have been found already.
		if (!foundHeader) {
			fprintf(stderr, "*** ERROR: Missing %s header.\n", AMIIBO_BIN_MAGIC);
			err = true;
			break;
		}

		// Check the command.
		if (!strcmp(token, "CS")) {
			// Character Series
			// Fields: ID, Name

			// ID
			token = strtok_r(nullptr, ":", &saveptr);
			if (!token) {
				fprintf(stderr, "*** ERROR: Line %d: 'CS' command is missing ID field.\n", line);
				err = true;
				break;
			}
			char *endptr;
			const unsigned int id = strtoul(token, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "*** ERROR: Line %d: 'CS' command: Invalid ID '%s'.\n", line, token);
				err = true;
				break;
			} else if (id > 16384) {
				fprintf(stderr, "*** ERROR: Line %d: 'CS' command: ID is out of range: %u (0x%04X)\n", line, id, id);
				err = true;
				break;
			}

			// Name
			token = strtok_r(nullptr, ":", &saveptr);
			if (!token || token[0] == '\0') {
				fprintf(stderr, "*** ERROR: Line %d: 'CS' command is missing Name field.\n", line);
				err = true;
				break;
			}

			// Check if the ID is a multiple of 4.
			if (id % 4 != 0) {
				fprintf(stderr, "*** ERROR: Line %d: 'CS' command has non-multiple-of-4 ID: %u (0x%04X)\n", line, id, id);
				err = true;
				break;
			}

			// Check if we already have this character series.
			const unsigned int idx = id / 4;
			if (idx < charSeriesTable.size()) {
				if (charSeriesTable[idx] != 0) {
					fprintf(stderr, "*** ERROR: Line %d: 'CS' command has duplicate ID: %u (0x%04X)\n", line, id, id);
					err = true;
					break;
				}
			} else {
				charSeriesTable.resize(idx+1);
			}

			// Add the name to the string table and save the series name.
			charSeriesTable[idx] = getStringTableOffset(token);
			if (verbose) {
				printf("CS: ID=%04X, name=%s, offset=%u\n", id, token, charSeriesTable[idx]);
			}
		} else if (!strcmp(token, "C")) {
			// Character
			// Fields: ID, Name

			// ID
			token = strtok_r(nullptr, ":", &saveptr);
			if (!token) {
				fprintf(stderr, "*** ERROR: Line %d: 'C' command is missing ID field.\n", line);
				err = true;
				break;
			}
			char *endptr;
			const unsigned int id = strtoul(token, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "*** ERROR: Line %d: 'C' command: Invalid ID '%s'.\n", line, token);
				err = true;
				break;
			} else if (id > 0xFFFF) {
				fprintf(stderr, "*** ERROR: Line %d: 'C' command: ID is out of range: %u (0x%04X)\n", line, id, id);
				err = true;
				break;
			}

			// Name
			token = strtok_r(nullptr, ":", &saveptr);
			if (!token || token[0] == '\0') {
				fprintf(stderr, "*** ERROR: Line %d: 'C' command is missing Name field.\n", line);
				err = true;
				break;
			}

			// Check if we already have this character.
			auto iter = charTable.find(id);
			if (iter != charTable.end()) {
				fprintf(stderr, "*** ERROR: Line %d: 'C' command has duplicate ID: %u (0x%04X)\n", line, id, id);
				err = true;
				break;
			}

			// Add the name to the string table and save the character.
			CharTableEntry charTableEntry;
			charTableEntry.char_id = id;
			charTableEntry.name = getStringTableOffset(token);
			if (verbose) {
				printf("C: ID=%04X, name=%s, offset=%u\n", id, token, charTableEntry.name);
			}
			charTable[id] = std::move(charTableEntry);
		} else if (!strcmp(token, "CV")) {
			// Character Variant
			// Fields: ID, VarID, Name

			// ID
			token = strtok_r(nullptr, ":", &saveptr);
			if (!token) {
				fprintf(stderr, "*** ERROR: Line %d: 'CV' command is missing ID field.\n", line);
				err = true;
				break;
			}
			char *endptr;
			const unsigned int id = strtoul(token, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "*** ERROR: Line %d: 'CV' command: Invalid ID '%s'.\n", line, token);
				err = true;
				break;
			} else if (id > 0xFFFF) {
				fprintf(stderr, "*** ERROR: Line %d: 'CV' command: ID is out of range: %u (0x%04X)\n", line, id, id);
				err = true;
				break;
			}

			// VarID
			token = strtok_r(nullptr, ":", &saveptr);
			if (!token) {
				fprintf(stderr, "*** ERROR: Line %d: 'CV' command is missing VarID field.\n", line);
				err = true;
				break;
			}
			const unsigned int var_id = strtoul(token, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "*** ERROR: Line %d: 'CV' command: Invalid VarID '%s'.\n", line, token);
				err = true;
				break;
			} else if (var_id > 0xFF) {
				fprintf(stderr, "*** ERROR: Line %d: 'CV' command: VarID is out of range: %u (0x%04X)\n", line, var_id, var_id);
				err = true;
				break;
			}

			// Name
			token = strtok_r(nullptr, ":", &saveptr);
			if (!token || token[0] == '\0') {
				fprintf(stderr, "*** ERROR: Line %d: 'CV' command is missing Name field.\n", line);
				err = true;
				break;
			}

			// Check if this character was set.
			auto iter = charTable.find(id);
			if (iter == charTable.end()) {
				fprintf(stderr, "*** ERROR: Line %d: 'CV' command has unassigned char ID: %u (0x%04X)\n", line, id, id);
				err = true;
				break;
			}
			// Set the high bit in the character ID.
			// This indicates character variants are present.
			iter->second.char_id |= CHARTABLE_VARIANT_FLAG;

			// Check if we already have this character variant.
			CharVariantMap *pMap;
			auto iterV = charVarTable.find(id);
			if (iterV != charVarTable.end()) {
				// We have at least one character variant.
				pMap = &(iterV->second);
				auto iterV2 = pMap->find(var_id);
				if (iterV2 != pMap->end()) {
					fprintf(stderr, "*** ERROR: Line %d: 'C' command has duplicate variant ID: %u:%u (0x%04X:0x%02X)\n", line, id, var_id, id, var_id);
					err = true;
					break;
				}
			} else {
				// No character variants yet.
				auto ret = charVarTable.emplace(id, CharVariantMap());
				pMap = &(ret.first->second);
			}

			// Add the variant ID.
			CharVariantTableEntry entry;
			entry.char_id = id;
			entry.var_id = var_id;
			entry.reserved = 0;
			entry.name = getStringTableOffset(token);
			if (verbose) {
				printf("CV: ID=%04X, VarID=%02X, name=%s, offset=%u\n", id, var_id, token, entry.name);
			}
			pMap->emplace(var_id, std::move(entry));
		} else if (!strcmp(token, "AS")) {
			// amiibo Series
			// Fields: ID, Name

			// ID
			token = strtok_r(nullptr, ":", &saveptr);
			if (!token) {
				fprintf(stderr, "*** ERROR: Line %d: 'AS' command is missing ID field.\n", line);
				err = true;
				break;
			}
			char *endptr;
			const unsigned int id = strtoul(token, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "*** ERROR: Line %d: 'AS' command: Invalid ID '%s'.\n", line, token);
				err = true;
				break;
			} else if (id > 0xFF) {
				fprintf(stderr, "*** ERROR: Line %d: 'AS' command: ID is out of range: %u (0x%02X)\n", line, id, id);
				err = true;
				break;
			}

			// Name
			token = strtok_r(nullptr, ":", &saveptr);
			if (!token || token[0] == '\0') {
				fprintf(stderr, "*** ERROR: Line %d: 'AS' command is missing Name field.\n", line);
				err = true;
				break;
			}

			// Check if we already have this amiibo series.
			if (id < amiiboSeriesTable.size()) {
				if (amiiboSeriesTable[id] != 0) {
					fprintf(stderr, "*** ERROR: Line %d: 'AS' command has duplicate ID: %u (0x%04X)\n", line, id, id);
					err = true;
					break;
				}
			} else {
				amiiboSeriesTable.resize(id+1);
			}

			// Add the name to the string table and save the series name.
			amiiboSeriesTable[id] = getStringTableOffset(token);
			if (verbose) {
				printf("AS: ID=%04X, name=%s, offset=%u\n", id, token, amiiboSeriesTable[id]);
			}
		} else if (!strcmp(token, "A")) {
			// amiibo
			// Fields: ID, Release No., Wave, Name

			// ID
			token = strtok_r(nullptr, ":", &saveptr);
			if (!token) {
				fprintf(stderr, "*** ERROR: Line %d: 'A' command is missing ID field.\n", line);
				err = true;
				break;
			}
			char *endptr;
			const unsigned int id = strtoul(token, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "*** ERROR: Line %d: 'A' command: Invalid ID '%s'.\n", line, token);
				err = true;
				break;
			} else if (id > 0xFFFF) {
				fprintf(stderr, "*** ERROR: Line %d: 'A' command: ID is out of range: %u (0x%04X)\n", line, id, id);
				err = true;
				break;
			}

			// Release No.
			token = strtok_r(nullptr, ":", &saveptr);
			if (!token) {
				fprintf(stderr, "*** ERROR: Line %d: 'A' command is missing Release No. field.\n", line);
				err = true;
				break;
			}
			const unsigned int release_no = strtoul(token, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "*** ERROR: Line %d: 'A' command: Invalid Release No. '%s'.\n", line, token);
				err = true;
				break;
			} else if (release_no > 0xFFFF) {
				fprintf(stderr, "*** ERROR: Line %d: 'A' command: Release No. is out of range: %u (0x%04X)\n", line, id, id);
				err = true;
				break;
			}

			// Wave
			token = strtok_r(nullptr, ":", &saveptr);
			if (!token) {
				fprintf(stderr, "*** ERROR: Line %d: 'A' command is missing Wave field.\n", line);
				err = true;
				break;
			}
			const unsigned int wave_no = strtoul(token, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "*** ERROR: Line %d: 'A' command: Invalid Wave. '%s'.\n", line, token);
				err = true;
				break;
			} else if (wave_no > 0xFF) {
				fprintf(stderr, "*** ERROR: Line %d: 'A' command: Wave is out of range: %u (0x%02X)\n", line, id, id);
				err = true;
				break;
			}

			// Name
			token = strtok_r(nullptr, ":", &saveptr);
			if (!token || token[0] == '\0') {
				fprintf(stderr, "*** ERROR: Line %d: 'A' command is missing Name field.\n", line);
				err = true;
				break;
			}

			// Check if we already have this amiibo.
			if (id < amiiboTable.size()) {
				if (amiiboTable[id].name != 0) {
					fprintf(stderr, "*** ERROR: Line %d: 'A' command has duplicate ID: %u (0x%04X)\n", line, id, id);
					err = true;
					break;
				}
			} else {
				amiiboTable.resize(id+1);
			}

			// Add the name to the string table and save the amiibo.
			AmiiboIDTableEntry &entry = amiiboTable[id];
			entry.release_no = release_no;
			entry.wave_no = wave_no;
			entry.reserved = 0;
			entry.name = getStringTableOffset(token);
			if (verbose) {
				printf("A: ID=%04X, release_no=%u, wave_no=%u, name=%s, offset=%u\n",
					id, release_no, wave_no, token, entry.name);
			}
		} else {
			// Invalid command.
			fprintf(stderr, "*** ERROR: Line %d: Invalid command '%s'.\n", line, token);
			err = true;
			break;
		}
	}
	fclose(f_in);

	if (err) {
		return EXIT_FAILURE;
	}

	// Check if any tables are 0 bytes.
	if (stringTable.empty()) {
		fputs("*** ERROR: String table is empty.\n", stderr);
		return EXIT_FAILURE;
	} else if (charSeriesTable.empty()) {
		fputs("*** ERROR: Character Series table is empty.\n", stderr);
		return EXIT_FAILURE;
	} else if (charTable.empty()) {
		fputs("*** ERROR: Character table is empty.\n", stderr);
		return EXIT_FAILURE;
	} else if (amiiboSeriesTable.empty()) {
		fputs("*** ERROR: amiibo Series table is empty.\n", stderr);
		return EXIT_FAILURE;
	} else if (amiiboTable.empty()) {
		fputs("*** ERROR: amiibo table is empty.\n", stderr);
		return EXIT_FAILURE;
	}

	// Write the binary data.
	FILE *f_out = fopen(argv[optind++], "wb");
	if (!f_out) {
		fprintf(stderr, "*** ERROR opening output file '%s': %s\n", argv[2], strerror(errno));
		return EXIT_FAILURE;
	}

	// TODO: Check fwrite() return values.

	// Write the initial header.
	// It will be rewritten once everything is finalized.
	fwrite(&binHeader, 1, sizeof(binHeader), f_out);

	// Character series table.
	alignFileTo16Bytes(f_out);
	const uint32_t cseries_len = static_cast<uint32_t>(charSeriesTable.size() * sizeof(charSeriesTable[0]));
	binHeader.cseries_offset = cpu_to_le32(static_cast<uint32_t>(ftello(f_out)));
	binHeader.cseries_len = cpu_to_le32(cseries_len);
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	// Byteswap the series table to little-endian.
	rp_byte_swap_32_array(charSeriesTable.data(), charSeriesTable.size() * sizeof(charSeriesTable[0]));
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	fwrite(charSeriesTable.data(), 1, cseries_len, f_out);

	// Character table
	alignFileTo16Bytes(f_out);
	const uint32_t char_len = static_cast<uint32_t>(charTable.size() * sizeof(CharTableEntry));
	binHeader.char_offset = cpu_to_le32(static_cast<uint32_t>(ftello(f_out)));
	binHeader.char_len = cpu_to_le32(char_len);
	for (const auto &p : charTable) {
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
		// Byteswap the entry first.
		CharTableEntry entry = p.second;
		entry.char_id	= cpu_to_le32(entry.char_id);
		entry.name	= cpu_to_le32(entry.name);
		fwrite(&entry, 1, sizeof(entry), f_out);
#else /* SYS_BYTEORDER == SYS_LIL_ENDIAN */
		// No byteswap is needed.
		fwrite(&(p.second), 1, sizeof(p.second), f_out);
#endif
	}

	// Character variant table
	alignFileTo16Bytes(f_out);
	uint32_t cvar_len = 0;
	binHeader.cvar_offset = cpu_to_le32(static_cast<uint32_t>(ftello(f_out)));
	for (const auto &p : charVarTable) {
		for (const auto &q : p.second) {
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
			// Byteswap the entry first.
			CharVariantTableEntry entry = q.second;
			entry.char_id	= cpu_to_le16(entry.char_id);
			entry.name	= cpu_to_le32(entry.name);
			fwrite(&entry, 1, sizeof(entry), f_out);
#else /* SYS_BYTEORDER == SYS_LIL_ENDIAN */
			// No byteswap is needed.
			fwrite(&(q.second), 1, sizeof(q.second), f_out);
#endif
			cvar_len += static_cast<uint32_t>(sizeof(q.second));
		}
	}
	binHeader.cvar_len = cpu_to_le32(cvar_len);

	// amiibo series table.
	alignFileTo16Bytes(f_out);
	const uint32_t aseries_len = static_cast<uint32_t>(amiiboSeriesTable.size() * sizeof(amiiboSeriesTable[0]));
	binHeader.aseries_offset = cpu_to_le32(static_cast<uint32_t>(ftello(f_out)));
	binHeader.aseries_len = cpu_to_le32(aseries_len);
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	// Byteswap the series table to little-endian.
	rp_byte_swap_32_array(amiiboSeriesTable.data(), amiiboSeriesTable.size() * sizeof(amiiboSeriesTable[0]));
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	fwrite(amiiboSeriesTable.data(), 1, aseries_len, f_out);

	// amiibo ID table
	alignFileTo16Bytes(f_out);
	const uint32_t amiibo_len = static_cast<uint32_t>(amiiboTable.size() * sizeof(AmiiboIDTableEntry));
	binHeader.amiibo_offset = cpu_to_le32(static_cast<uint32_t>(ftello(f_out)));
	binHeader.amiibo_len = cpu_to_le32(amiibo_len);
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
	// Byteswapping is needed, so we have to process each entry.
	for (const AmiiboIDTableEntry &p : amiiboTable) {
		AmiiboIDTableEntry entry = p;
		entry.release_no	= cpu_to_le16(entry.release_no);
		entry.name		= cpu_to_le32(entry.name);
		fwrite(&entry, 1, sizeof(entry), f_out);
	}
#else /* SYS_BYTEORDER == SYS_LIL_ENDIAN */
	// No byteswap is needed.
	fwrite(amiiboTable.data(), 1, amiibo_len, f_out);
#endif

	// Make sure the string table is a mulitple of 16 bytes.
	stringTable.resize(ALIGN_BYTES(16, stringTable.size()));
	alignFileTo16Bytes(f_out);
	binHeader.strtbl_offset = cpu_to_le32(static_cast<uint32_t>(ftello(f_out)));
	binHeader.strtbl_len = cpu_to_le32(static_cast<uint32_t>(stringTable.size()));
	fwrite(stringTable.data(), 1, stringTable.size(), f_out);

	// Write the updated header.
	rewind(f_out);
	fwrite(&binHeader, 1, sizeof(binHeader), f_out);
	fclose(f_out);

	// We're done here.
	return EXIT_SUCCESS;
}
