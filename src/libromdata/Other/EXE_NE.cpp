/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE_NE.cpp: DOS/Windows executable reader.                              *
 * 16-bit New Executable format.                                           *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "EXE_p.hpp"
#include "disc/NEResourceReader.hpp"

// librpbase
using namespace LibRpBase;

// C++ STL classes.
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

/** EXEPrivate **/

/**
 * Load the NE resource table.
 * @return 0 on success; negative POSIX error code on error. (-ENOENT if not found)
 */
int EXEPrivate::loadNEResourceTable(void)
{
	if (rsrcReader) {
		// Resource reader is already initialized.
		return 0;
	} else if (!file || !file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!isValid) {
		// Unknown executable type.
		return -EIO;
	} else if (exeType != ExeType::NE) {
		// Unsupported executable type.
		return -ENOTSUP;
	}

	// NE resource table offset is relative to the start of the NE header.
	// It must be >= the size of the NE header.
	uint32_t ResTableOffset = le16_to_cpu(hdr.ne.ResTableOffset);
	if (ResTableOffset < sizeof(NE_Header)) {
		// Resource table cannot start in the middle of the NE header.
		return -EIO;
	}

	// Check for the lowest non-zero offset after ResTableOffset.
	// This will be used for the resource table size.
	uint32_t maxOffset = 0;
	uint32_t resTableSize = 0;
	{
		#define TABLE_CHECK(var) do { \
			const uint32_t var = le16_to_cpu(hdr.ne.var); \
			if (var != 0 && var > ResTableOffset && \
			    (maxOffset == 0 || var < maxOffset)) \
			{ \
				maxOffset = var; \
			} \
		} while (0)
		TABLE_CHECK(SegTableOffset);
		TABLE_CHECK(ResidNamTable);
		TABLE_CHECK(ModRefTable);
		TABLE_CHECK(ImportNameTable);
		// TODO: OffStartNonResTab is from the start of the file.
		// Not sure if we need to check it.

		if (maxOffset != 0 && maxOffset > ResTableOffset) {
			// Found a valid maximum.
			resTableSize = maxOffset - ResTableOffset;
		}
	}

	// Adjust ResTableOffset to make it an absolute address.
	ResTableOffset += le16_to_cpu(mz.e_lfanew);

	// Check if a size is known.
	if (resTableSize == 0) {
		// Not known. Go with the file size.
		// NOTE: Limited to 32-bit file sizes.
		resTableSize = static_cast<uint32_t>(file->size()) - ResTableOffset;
	}

	// Load the resources using NEResourceReader.
	rsrcReader = new NEResourceReader(file, ResTableOffset, resTableSize);
	if (!rsrcReader->isOpen()) {
		// Failed to open the resource table.
		int err = rsrcReader->lastError();
		UNREF_AND_NULL_NOCHK(rsrcReader);
		return (err != 0 ? err : -EIO);
	}

	// Resource table loaded.
	return 0;
}

/**
 * Find the runtime DLL. (NE version)
 * @param refDesc String to store the description.
 * @param refLink String to store the download link.
 * @param refHasKernel Set to true if KERNEL is present in the import table.
 *                     Used to distinguish between old Windows and OS/2 executables.
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::findNERuntimeDLL(string &refDesc, string &refLink, bool &refHasKernel)
{
	refDesc.clear();
	refLink.clear();
	refHasKernel = false;

	if (!file || !file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!isValid) {
		// Unknown executable type.
		return -EIO;
	}

	// Offsets in the NE header are relative to the start of the header.
	const uint32_t ne_hdr_addr = le32_to_cpu(mz.e_lfanew);

	// Load the module references and imported names tables.
	// NOTE: Imported names table has counted strings, and
	// since Win3.1 used 8.3 filenames (and extensions are
	// not present here), there's a maximum of 9*ModRefs
	// bytes to be read.
	// NOTE 2: There might be some extra empty strings at
	// the beginning, so read some extra data in case.
	const unsigned int modRefs = le16_to_cpu(hdr.ne.ModRefs);
	const unsigned int modRefTable_addr = ne_hdr_addr + le16_to_cpu(hdr.ne.ModRefTable);
	const unsigned int importNameTable_addr = ne_hdr_addr + le16_to_cpu(hdr.ne.ImportNameTable);

	if (modRefs == 0) {
		// No module references.
		return -ENOENT;
	} else if (modRefTable_addr < ne_hdr_addr || importNameTable_addr < ne_hdr_addr) {
		// One of the addresses is out of range.
		return -EIO;
	}

	// TODO: Check for overlapping tables?

	// Determine the low address.
	// ModRefTable is usually first, but we can't be certain.
	uint32_t read_low_addr;
	uint32_t read_size;
	uint32_t nameTable_size;
	if (modRefTable_addr < importNameTable_addr) {
		// ModRefTable is first.
		// Add 256 bytes for the nametable, since we can't determine how
		// big the nametable actually is without reading it.
		read_low_addr = modRefTable_addr;
		nameTable_size = (9 * (static_cast<uint32_t>(modRefs) + 2)) + 256;
		read_size = (importNameTable_addr - modRefTable_addr) + nameTable_size;
	} else {
		// ImportNameTable is first.
		read_low_addr = importNameTable_addr;
		nameTable_size = (modRefTable_addr - importNameTable_addr);
		read_size = nameTable_size + (modRefs * static_cast<uint32_t>(sizeof(uint16_t)));
	}

	if (read_size > 128*1024) {
		// Shouldn't be more than 128 KB...
		// (Actually, it probably shouldn't be more than 64 KB.)
		return -EIO;
	}

	unique_ptr<uint8_t[]> tbls(new uint8_t[read_size]);
	size_t size = file->seekAndRead(read_low_addr, tbls.get(), read_size);
	if (size != read_size) {
		// Error reading the tables.
		// NOTE: Even with the extra padding, this shouldn't happen,
		// since there's executable code afterwards...
		return -EIO;
	}

	const uint16_t *pModRefTable;
	char *pNameTable;
	if (modRefTable_addr < importNameTable_addr) {
		// ModRefTable is first.
		pModRefTable = reinterpret_cast<const uint16_t*>(tbls.get());
		pNameTable = reinterpret_cast<char*>(&tbls[importNameTable_addr - modRefTable_addr]);
	} else {
		// ImportNameTable is first.
		pNameTable = reinterpret_cast<char*>(tbls.get());
		pModRefTable = reinterpret_cast<const uint16_t*>(&tbls[modRefTable_addr - importNameTable_addr]);
	}

	// Convert the name table to uppercase for comparison purposes.
	// NOTE: Uppercase due to DOS/Win16 conventions. PE uses lowercase.
	{
		char *const p_end = &pNameTable[nameTable_size];
		for (char *p = pNameTable; p < p_end; p++) {
			if (*p >= 'a' && *p <= 'z') *p &= ~0x20;
		}
	}

	// Visual Basic DLL version to display version table.
	static const struct {
		uint8_t ver_major;
		uint8_t ver_minor;
		const char dll_name[8];	// NOT NULL-terminated!
		const char *url;
	} msvb_dll_tbl[] = {
		{4,0, {'V','B','R','U','N','4','0','0'}, nullptr},
		{4,0, {'V','B','R','U','N','4','1','6'}, nullptr},	// TODO: Is this a thing?
		{3,0, {'V','B','R','U','N','3','0','0'}, nullptr},
		{2,0, {'V','B','R','U','N','2','0','0'}, nullptr},
		{1,0, {'V','B','R','U','N','1','0','0'}, "https://download.microsoft.com/download/vb30/sampleaa/1/w9xnt4/en-us/vbrun100.exe"},
	};

	// FIXME: Alignment?
	const uint16_t *pModRef = pModRefTable;
	const uint16_t *const pModRefEnd = &pModRef[modRefs];
	for (; pModRef < pModRefEnd; pModRef++) {
		const unsigned int nameOffset = le16_to_cpu(*pModRef);
		assert(nameOffset < nameTable_size);
		if (nameOffset >= nameTable_size) {
			// Out of range.
			// TODO: Return an error?
			break;
		}

		const uint8_t count = pNameTable[nameOffset];
		assert(nameOffset + 1 + count < nameTable_size);
		if (nameOffset + 1 + count >= nameTable_size) {
			// Out of range.
			// TODO: Return an error?
			break;
		}

		const char *const pDllName = reinterpret_cast<const char*>(&pNameTable[nameOffset + 1]);

		// Check the DLL name.
		// TODO: More checks.
		// NOTE: There's only four 16-bit versions of Visual Basic,
		// so we're hard-coding everything.
		// NOTE 2: Not breaking immediately on success, since we also want to
		// check if KERNEL is present. If we already found KERNEL, *then* we'll
		// break on success.
		if (count == 6) {
			// If KERNEL is imported, this is a Windows executable.
			// This is needed in order to distinguish between really old
			// OS/2 and Windows executables target OS == 0.
			// Reference: https://github.com/wine-mirror/wine/blob/ba9f3dc198dfc81bb40159077b73b797006bb73c/dlls/kernel32/module.c#L262
			if (!strncmp(pDllName, "KERNEL", 6)) {
				// Found KERNEL.
				refHasKernel = true;
				if (!refDesc.empty())
					break;
			}
		} else if (count == 8) {
			// Check for Visual Basic DLLs.
			// NOTE: There's only three 32-bit versions of Visual Basic,
			// and .NET versions don't count.
			for (const auto &p : msvb_dll_tbl) {
				if (!strncmp(pDllName, p.dll_name, sizeof(p.dll_name))) {
					// Found a matching version.
					refDesc = rp_sprintf(C_("EXE|Runtime", "Microsoft Visual Basic %u.%u Runtime"),
						p.ver_major, p.ver_minor);
					refLink = p.url;
					break;
				}
			}
		}
	}

	return (!refDesc.empty() ? 0 : -ENOENT);
}

/**
 * Add fields for NE executables.
 */
void EXEPrivate::addFields_NE(void)
{
	// Up to 4 tabs.
	fields->reserveTabs(4);

	// NE Header
	fields->setTabName(0, "NE");
	fields->setTabIndex(0);

	// Get the runtime DLL and if KERNEL is imported.
	string runtime_dll, runtime_link;
	bool hasKernel = false;
	int ret = findNERuntimeDLL(runtime_dll, runtime_link, hasKernel);
	if (ret != 0) {
		// Unable to get the runtime DLL.
		// NOTE: Assuming KERNEL is present.
		runtime_dll.clear();
		runtime_link.clear();
		hasKernel = true;
	}

	// Target OS.
	const char *targetOS = nullptr;
	if (hdr.ne.targOS == NE_OS_UNKNOWN) {
		// Either old OS/2 or Windows 1.x/2.x.
		targetOS = (hasKernel ? "Windows 1.x/2.x" : "Old OS/2");
	} else if (hdr.ne.targOS < ARRAY_SIZE(NE_TargetOSes)) {
		targetOS = NE_TargetOSes[hdr.ne.targOS];
	}
	if (!targetOS) {
		// Check for Phar Lap extenders.
		if (hdr.ne.targOS == NE_OS_PHARLAP_286_OS2) {
			targetOS = NE_TargetOSes[NE_OS_OS2];
		} else if (hdr.ne.targOS == NE_OS_PHARLAP_286_WIN) {
			targetOS = NE_TargetOSes[NE_OS_WIN];
		}
	}

	const char *const targetOS_title = C_("EXE", "Target OS");
	if (targetOS) {
		fields->addField_string(targetOS_title, targetOS);
	} else {
		fields->addField_string(targetOS_title,
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"), hdr.ne.targOS));
	}

	// DGroup type.
	static const char *const dgroupTypes[] = {
		NOP_C_("EXE|DGroupType", "None"),
		NOP_C_("EXE|DGroupType", "Single Shared"),
		NOP_C_("EXE|DGroupType", "Multiple"),
		NOP_C_("EXE|DGroupType", "(null)"),
	};
	fields->addField_string(C_("EXE", "DGroup Type"),
		dpgettext_expr(RP_I18N_DOMAIN, "EXE|DGroupType", dgroupTypes[hdr.ne.ProgFlags & 3]));

	// Program flags.
	static const char *const ProgFlags_names[] = {
		nullptr, nullptr,	// DGroup Type
		NOP_C_("EXE|ProgFlags", "Global Init"),
		NOP_C_("EXE|ProgFlags", "Protected Mode Only"),
		NOP_C_("EXE|ProgFlags", "8086 insns"),
		NOP_C_("EXE|ProgFlags", "80286 insns"),
		NOP_C_("EXE|ProgFlags", "80386 insns"),
		NOP_C_("EXE|ProgFlags", "FPU insns"),
	};
	vector<string> *const v_ProgFlags_names = RomFields::strArrayToVector_i18n(
		"EXE|ProgFlags", ProgFlags_names, ARRAY_SIZE(ProgFlags_names));
	fields->addField_bitfield("Program Flags",
		v_ProgFlags_names, 2, hdr.ne.ProgFlags);

	// Application type.
	const char *applType;
	if (hdr.ne.targOS == NE_OS_OS2) {
		// Only mentioning Presentation Manager for OS/2 executables.
		static const char *const applTypes_OS2[] = {
			NOP_C_("EXE|ApplType", "None"),
			NOP_C_("EXE|ApplType", "Full Screen (not aware of Presentation Manager)"),
			NOP_C_("EXE|ApplType", "Presentation Manager compatible"),
			NOP_C_("EXE|ApplType", "Presentation Manager application"),
		};
		applType = applTypes_OS2[hdr.ne.ApplFlags & 3];
	} else {
		// Assume Windows for everything else.
		static const char *const applTypes_Win[] = {
			NOP_C_("EXE|ApplType", "None"),
			NOP_C_("EXE|ApplType", "Full Screen (not aware of Windows)"),
			NOP_C_("EXE|ApplType", "Windows compatible"),
			NOP_C_("EXE|ApplType", "Windows application"),
		};
		applType = applTypes_Win[hdr.ne.ApplFlags & 3];
	}
	fields->addField_string(C_("EXE", "Application Type"),
		dpgettext_expr(RP_I18N_DOMAIN, "EXE|ApplType", applType));

	// Application flags.
	static const char *const ApplFlags_names[] = {
		nullptr, nullptr,	// Application type
		nullptr,
		NOP_C_("EXE|ApplFlags", "OS/2 Application"),
		nullptr,
		NOP_C_("EXE|ApplFlags", "Image Error"),
		NOP_C_("EXE|ApplFlags", "Non-Conforming"),
		NOP_C_("EXE|ApplFlags", "DLL"),
	};
	vector<string> *const v_ApplFlags_names = RomFields::strArrayToVector_i18n(
		"EXE|ApplFlags", ApplFlags_names, ARRAY_SIZE(ApplFlags_names));
	fields->addField_bitfield(C_("EXE", "Application Flags"),
		v_ApplFlags_names, 2, hdr.ne.ApplFlags);

	// Other flags.
	// NOTE: Indicated as OS/2 flags by OSDev Wiki,
	// but may be set on Windows programs too.
	// References:
	// - http://wiki.osdev.org/NE
	// - http://www.program-transformation.org/Transform/PcExeFormat
	static const char *const OtherFlags_names[] = {
		NOP_C_("EXE|OtherFlags", "Long File Names"),
		NOP_C_("EXE|OtherFlags", "Protected Mode"),
		NOP_C_("EXE|OtherFlags", "Proportional Fonts"),
		NOP_C_("EXE|OtherFlags", "Gangload Area"),
	};
	vector<string> *const v_OtherFlags_names = RomFields::strArrayToVector_i18n(
		"EXE|OtherFlags", OtherFlags_names, ARRAY_SIZE(OtherFlags_names));
	fields->addField_bitfield(C_("EXE", "Other Flags"),
		v_OtherFlags_names, 2, hdr.ne.OS2EXEFlags);

	// Timestamp (Early NE executables; pre-Win1.01)
	// NOTE: Uses the same field as CRC, so use
	// heuristics to determine if it's valid.
	// High 16 bits == date; low 16 bits = time
	// Reference: https://docs.microsoft.com/en-us/cpp/c-runtime-library/32-bit-windows-time-date-formats?view=msvc-170
	// TODO: Also add to metadata?
	const uint32_t ne_dos_time = le32_to_cpu(hdr.ne.FileLoadCRC);
	struct tm ne_tm;
	// tm_year is year - 1900; DOS timestamp starts at 1980.
	// NOTE: Only allowing 1983-1985.
	ne_tm.tm_year = ((ne_dos_time >> 25) & 0x7F) + 80;
	if (ne_tm.tm_year >= 83 && ne_tm.tm_year <= 85) {
		ne_tm.tm_mon	= ((ne_dos_time >> 21) & 0x0F) - 1;	// DOS is 1-12; Unix is 0-11
		ne_tm.tm_mday	= ((ne_dos_time >> 16) & 0x1F);
		ne_tm.tm_hour	= ((ne_dos_time >> 11) & 0x1F);
		ne_tm.tm_min	= ((ne_dos_time >>  5) & 0x3F);
		ne_tm.tm_sec	=  (ne_dos_time & 0x1F) * 2;

		// tm_wday and tm_yday are output variables.
		ne_tm.tm_wday = 0;
		ne_tm.tm_yday = 0;
		ne_tm.tm_isdst = 0;

		// Verify ranges.
		if (ne_tm.tm_mon <= 11 && ne_tm.tm_mday <= 31 &&
		    ne_tm.tm_hour <= 23 && ne_tm.tm_min <= 60 && ne_tm.tm_sec <= 59)
		{
			// In range.
			const time_t ne_time = timegm(&ne_tm);
			fields->addField_dateTime(C_("EXE", "Timestamp"), ne_time,
				RomFields::RFT_DATETIME_HAS_DATE |
				RomFields::RFT_DATETIME_HAS_TIME |
				RomFields::RFT_DATETIME_IS_UTC);	// no timezone
		}
	}

	// Expected Windows version.
	// TODO: Is this used in OS/2 executables?
	if (hdr.ne.targOS == NE_OS_WIN || hdr.ne.targOS == NE_OS_WIN386) {
		fields->addField_string(C_("EXE", "Windows Version"),
			rp_sprintf("%u.%u", hdr.ne.expctwinver[1], hdr.ne.expctwinver[0]));
	}

	// Runtime DLL.
	// NOTE: Strings were obtained earlier.
	if (hdr.ne.targOS == NE_OS_WIN) {
		// TODO: Show the link?
		fields->addField_string(C_("EXE", "Runtime DLL"), runtime_dll);
	}

	// Load resources.
	ret = loadNEResourceTable();
	if (ret == 0 && rsrcReader) {
		// Load the version resource.
		// NOTE: load_VS_VERSION_INFO loads it in host-endian.
		VS_FIXEDFILEINFO vsffi;
		IResourceReader::StringFileInfo vssfi;
		ret = rsrcReader->load_VS_VERSION_INFO(VS_VERSION_INFO, -1, &vsffi, &vssfi);
		if (ret == 0) {
			// Add the version fields.
			fields->setTabName(1, C_("EXE", "Version"));
			fields->setTabIndex(1);
			addFields_VS_VERSION_INFO(&vsffi, &vssfi);
		}
	}

	// Add entries
	addFields_NE_Entry();
}

/**
 * Add fields for NE entry table.
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::addFields_NE_Entry(void)
{
	if (!file || !file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!isValid) {
		// Unknown executable type.
		return -EIO;
	}

	// Offsets in the NE header are relative to the start of the header.
	const uint32_t ne_hdr_addr = le32_to_cpu(mz.e_lfanew);
	const uint32_t enttab_off = ne_hdr_addr + le16_to_cpu(hdr.ne.EntryTableOffset);
	const uint16_t enttab_len = le16_to_cpu(hdr.ne.EntryTableLength);

	ao::uvector<uint8_t> enttab;
	enttab.resize(enttab_len);
	size_t nread = file->seekAndRead(enttab_off, enttab.data(), enttab.size());
	if (nread != enttab.size())
		return -EIO; // Short read

	const uint32_t restab_off = ne_hdr_addr + le16_to_cpu(hdr.ne.ResidNamTable);
	const uint16_t restab_len = le16_to_cpu(hdr.ne.ModRefTable) - le16_to_cpu(hdr.ne.ResidNamTable);
	ao::uvector<uint8_t> restab;
	restab.resize(restab_len);
	nread = file->seekAndRead(restab_off, restab.data(), restab.size());
	if (nread != restab.size())
		return -EIO; // Short read

	const uint32_t nrestab_off = le32_to_cpu(hdr.ne.OffStartNonResTab);
	const uint16_t nrestab_len = le16_to_cpu(hdr.ne.NoResNamesTabSiz);
	ao::uvector<uint8_t> nrestab;
	nrestab.resize(nrestab_len);
	nread = file->seekAndRead(nrestab_off, nrestab.data(), nrestab.size());
	if (nread != nrestab.size())
		return -EIO; // Short read

	struct Entry {
		uint16_t ordinal;
		uint8_t flags;
		uint8_t segment;
		uint16_t offset;
		bool is_movable;
		bool is_resident;
		bool has_name;
		string name;
	};
	vector<Entry> ents;

	// Read entry table
	auto p = enttab.begin();
	for (int ordinal = 1;;) {
		// Entry table consists of bundles of symbols
		// Each bundle is starts with count and segment of the symbols
		if (p >= enttab.end())
			return -ENOENT;
		uint16_t bundle_count = *p++;
		if (bundle_count == 0)
			break; // end of table
		if (p >= enttab.end())
			return -ENOENT;
		uint16_t bundle_segment = *p++;
		switch (bundle_segment) {
		case 0:
			// Segment value 0 is used for skipping over unused ordinal values.
			ordinal += bundle_count;
			break;
		default:
			/* Segment values 0x01-0xFE are used for fixed segments.
			 * - DB flags
			 * - DW offset
			 * 0xFE is used for constants.*/
			if (p + bundle_count*3 > enttab.end())
				return -ENOENT;
			for (int i = 0; i < bundle_count; i++) {
				Entry ent;
				ent.ordinal = ordinal++;
				ent.flags = p[0];
				ent.segment = bundle_segment;
				ent.offset = p[1] | p[2]<<8;
				ent.is_movable = false;
				ents.push_back(ent);
				p += 3;
			}
			break;
		case 0xFF:
			/* Segment value 0xFF is used for movable segments.
			 * - DB flags
			 * - DW INT 3F
			 * - DB segment
			 * - DW offset */
			if (p + bundle_count*6 > enttab.end())
				return -ENOENT;
			for (int i = 0; i < bundle_count; i++) {
				if (p[1] != 0xCD && p[2] != 0x3F) // INT 3Fh
					return -ENOENT;
				Entry ent;
				ent.ordinal = ordinal++;
				ent.flags = p[0];
				ent.segment = p[3];
				ent.offset = p[4] | p[5]<<8;
				ent.is_movable = true;
				ents.push_back(ent);
				p += 6;
			}
			break;
		}
	}

	/* Currently ents is sorted by ordinal. For duplicate names we'll add
	 * more entries, so we must remember the original size so we can do
	 * a binary search. */
	size_t last = ents.size();
	auto readNames = [&](const uint8_t *p, const uint8_t *end, bool is_resident) -> int {
		if (p == end)
			return -ENOENT;
		/* Skip first string. For resident strings it's module name, and
		 * for non-resident strings it's module description. */
		p += *p + 3;
		if (p >= end)
			return -ENOENT;

		while (*p) {
			uint8_t len = *p++;
			if (p + len + 2 >= end) // next length byte >= end
				return -ENOENT;
			string name(reinterpret_cast<const char *>(p), len);
			uint16_t ordinal = p[len] | p[len+1]<<8;

			// binary search for the ordinal
			auto it = std::lower_bound(ents.begin(), ents.begin()+last, ordinal,
				[](const Entry &lhs, uint16_t rhs) {
					return lhs.ordinal < rhs;
				});
			if (it == ents.begin()+last || it->ordinal != ordinal) {
				// name points to non-existent ordinal
				return -ENOENT;
			}

			if (it->has_name) {
				// This ordinal already has a name.
				// Temporarily move the old name out to avoid a copy.
				string oldname = std::move(it->name);
				// Duplicate the entry, replace name in the copy.
				Entry ent = *it;
				it->name = std::move(oldname);
				ent.name = std::move(name);
				ent.is_resident = is_resident;
				ents.push_back(std::move(ent)); // `it` is invalidated here
			} else {
				it->has_name = true;
				it->name = std::move(name);
				it->is_resident = is_resident;
			}

			p += len + 2;
		}
		return 0;
	};

	// Read names
	int res;
	res = readNames(restab.begin(), restab.end(), true);
	if (res)
		return res;
	res = readNames(nrestab.begin(), nrestab.end(), false);
	if (res)
		return res;

	auto vv_data = new RomFields::ListData_t();
	vv_data->reserve(ents.size());
	for (unsigned int i = 0; i < static_cast<unsigned int>(ents.size()); i++) {
		const Entry &ent = ents[i];
		vv_data->emplace_back();
		auto &row = vv_data->back();
		row.reserve(4);
		/* NODATA and RESIDENTNAME are from DEF files. EXPORT and PARAMS
		 * are names I made up (in DEF files you can't specify internal
		 * entries, and parameter count is specified by just a number).
		 * Typical flags values are 3 for exports, 0 for internal
		 * entries. */
		string flags;
		if (ent.flags & 1)
			flags = " EXPORT";
		if (!(ent.flags & 2))
			flags += " NODATA";
		if (ent.flags & 4)
			flags += " (bit 2)";
		/* Parameter count. I haven't found any module where this is
		 * actually used. */
		if (ent.flags & 0xF8)
			flags += rp_sprintf(" PARAMS=%d", ent.flags>>3);
		if (ent.is_resident)
			flags += " RESIDENTNAME";
		flags.erase(0, 1);

		row.emplace_back(rp_sprintf("%d", ent.ordinal));
		row.emplace_back(ent.has_name ? ent.name : C_("EXE|Exports", "(No name)"));
		if (ent.is_movable)
			row.emplace_back(rp_sprintf(C_("EXE|Exports", "%02X:%04X (Movable)"),
				ent.segment, ent.offset));
		else if (ent.segment != 0xFE)
			row.emplace_back(rp_sprintf(C_("EXE|Exports", "%02X:%04X (Fixed)"),
				ent.segment, ent.offset));
		else
			row.emplace_back(rp_sprintf(C_("EXE|Exports", "%04X (Constant)"),
				ent.offset));
		row.emplace_back(flags);
	}

	// tr: this is the EXE NE equivalent of EXE PE export table
	fields->addTab(C_("EXE", "Entries"));
	fields->reserve(1);

	// Create the tab if we have any exports.
	if (vv_data->size()) {
		static const char *const field_names[] = {
			NOP_C_("EXE|Exports", "Ordinal"),
			NOP_C_("EXE|Exports", "Name"),
			NOP_C_("EXE|Exports", "Address"),
			NOP_C_("EXE|Exports", "Flags"),
		};
		vector<string> *const v_field_names = RomFields::strArrayToVector_i18n(
			"EXE|Exports", field_names, ARRAY_SIZE(field_names));

		RomFields::AFLD_PARAMS params;
		params.flags = RomFields::RFT_LISTDATA_SEPARATE_ROW;
		params.headers = v_field_names;
		params.data.single = vv_data;
		fields->addField_listData(C_("EXE", "Entries"), &params);
	} else {
		delete vv_data;
	}
	return 0;
}

}
