/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE_NE.cpp: DOS/Windows executable reader.                              *
 * 16-bit New Executable format.                                           *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * Copyright (c) 2022 by Egor.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "EXE_p.hpp"
#include "disc/NEResourceReader.hpp"
#include "data/EXENEEntries.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;
using std::vector;

using vhvc::span;
using vhvc::reinterpret_span;
using vhvc::reinterpret_span_limit;

namespace LibRomData {

/** EXEPrivate **/

/**
 * Load the redisent portion of NE header
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::loadNEResident(void)
{
	if (ne_resident_loaded) {
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

	// Offsets in the NE header are relative to the start of the header.
	const uint32_t ne_hdr_addr = le32_to_cpu(mz.e_lfanew);
	const uint32_t entry_table_addr = le16_to_cpu(hdr.ne.EntryTableOffset);
	const uint32_t ne_hdr_len = entry_table_addr + le16_to_cpu(hdr.ne.EntryTableLength);
	ne_resident.resize(ne_hdr_len);
	size_t nread = file->seekAndRead(ne_hdr_addr, ne_resident.data(), ne_resident.size());
	if (nread != ne_resident.size())
		return -EIO; // Short read

	span<const uint8_t> ne_segment_raw;
	span<const uint8_t> ne_resident_name_raw;
	span<const uint8_t> ne_modref_raw;
	span<const uint8_t> ne_imported_name_raw;

	// NOTE: For performance reasons on 64-bit, we're using uint32_t instead of size_t.
	// It's not possible for an NE executable to be larger than 16 MB, anyway.
	uint32_t end = static_cast<uint32_t>(ne_resident.size());
	auto set_span = [this, &end](span<const uint8_t> &sp, unsigned int offset) -> bool {
		if (offset > end)
			return true;
		sp = span<const uint8_t>(ne_resident.data() + offset, end - offset);
		end = offset;
		return false;
	};
	// NOTE: The order of the tables in the resident part of NE header is fixed.
	if (set_span(ne_entry_table,       entry_table_addr) ||
	    set_span(ne_imported_name_raw, le16_to_cpu(hdr.ne.ImportNameTable)) ||
	    set_span(ne_modref_raw,        le16_to_cpu(hdr.ne.ModRefTable)) ||
	    set_span(ne_resident_name_raw, le16_to_cpu(hdr.ne.ResidNamTable)) ||
	    set_span(ne_resource_table,    le16_to_cpu(hdr.ne.ResTableOffset)) ||
	    set_span(ne_segment_raw,       le16_to_cpu(hdr.ne.SegTableOffset)) ||
	    end < sizeof(NE_Header)) {
		return -EIO;
	}

	const size_t segment_count = le16_to_cpu(hdr.ne.SegCount);
	ne_segment_table = reinterpret_span_limit<const NE_Segment>(ne_segment_raw, segment_count);
	assert(segment_count <= ne_segment_table.size());

	ne_resident_name_table = reinterpret_span<const char>(ne_resident_name_raw);

	const size_t modref_count = le16_to_cpu(hdr.ne.ModRefs);
	ne_modref_table = reinterpret_span_limit<const uint16_t>(ne_modref_raw, modref_count);
	assert(modref_count <= ne_modref_table.size());

	ne_imported_name_table = reinterpret_span<const char>(ne_imported_name_raw);

	ne_resident_loaded = true;

	return 0;
}


/**
 * Load the non-resident name table. (NE)
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::loadNENonResidentNames(void)
{
	if (ne_nonresident_name_table_loaded) {
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
	ne_nonresident_name_table.resize(le16_to_cpu(hdr.ne.NoResNamesTabSiz));
	size_t nread = file->seekAndRead(le32_to_cpu(hdr.ne.OffStartNonResTab),
		ne_nonresident_name_table.data(),
		ne_nonresident_name_table.size());
	if (nread != ne_nonresident_name_table.size())
		return -EIO; // Short read
	ne_nonresident_name_table_loaded = true;
	return 0;
}
/**
 * Load the NE resource table.
 * @return 0 on success; negative POSIX error code on error. (-ENOENT if not found)
 */
int EXEPrivate::loadNEResourceTable(void)
{
	if (rsrcReader) {
		// Resource reader is already initialized.
		return 0;
	}
	int res = loadNEResident();
	if (res < 0)
		return res;
	if (!file || !file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!isValid) {
		// Unknown executable type.
		return -EIO;
	} else if (exeType != ExeType::NE) {
		// Unsupported executable type.
		return -ENOTSUP;
	}

	// FIXME: NEResourceReader should be able to just take ne_resource_table.
	// NE resource table offset is relative to the start of the NE header.
	const uint32_t ResTableOffset = le16_to_cpu(mz.e_lfanew) + le16_to_cpu(hdr.ne.ResTableOffset);
	if (ResTableOffset < le16_to_cpu(mz.e_lfanew)) {
		// Offse overflow
		return -EIO;
	}

	// Load the resources using NEResourceReader.
	rsrcReader = std::make_shared<NEResourceReader>(file, ResTableOffset,
		static_cast<uint32_t>(ne_resource_table.size()));
	if (!rsrcReader->isOpen()) {
		// Failed to open the resource table.
		int err = rsrcReader->lastError();
		rsrcReader.reset();
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

	int res = loadNEResident();
	if (res < 0)
		return res;

	if (ne_modref_table.size() == 0) {
		// No module references.
		return -ENOENT;
	}

	// Visual Basic DLL version to display version table.
	struct msvb_dll_t {
		uint8_t ver_major;
		uint8_t ver_minor;
		const char dll_name[8];	// NOT NULL-terminated!
		const char *url;
	};
	static const array<msvb_dll_t, 5> msvb_dll_tbl = {{
		{4,0, {'V','B','R','U','N','4','0','0'}, nullptr},
		{4,0, {'V','B','R','U','N','4','1','6'}, nullptr},	// TODO: Is this a thing?
		{3,0, {'V','B','R','U','N','3','0','0'}, nullptr},
		{2,0, {'V','B','R','U','N','2','0','0'}, nullptr},
		{1,0, {'V','B','R','U','N','1','0','0'}, "https://download.microsoft.com/download/vb30/sampleaa/1/w9xnt4/en-us/vbrun100.exe"},
	}};

	// FIXME: Alignment?
	for (const uint16_t modRef : ne_modref_table) {
		const unsigned int nameOffset = le16_to_cpu(modRef);
		assert(nameOffset < ne_imported_name_table.size());
		if (nameOffset >= ne_imported_name_table.size()) {
			// Out of range.
			// TODO: Return an error?
			break;
		}

		const uint8_t count = static_cast<uint8_t>(ne_imported_name_table[nameOffset]);
		assert(nameOffset + 1 + count <= ne_imported_name_table.size());
		if (nameOffset + 1 + count > ne_imported_name_table.size()) {
			// Out of range.
			// TODO: Return an error?
			break;
		}

		const char *const pDllName = &ne_imported_name_table[nameOffset + 1];

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
			if (!strncasecmp(pDllName, "KERNEL", 6)) {
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
				if (!strncasecmp(pDllName, p.dll_name, sizeof(p.dll_name))) {
					// Found a matching version.
					refDesc = fmt::format(C_("EXE|Runtime", "Microsoft Visual Basic {:d}.{:d} Runtime"),
						p.ver_major, p.ver_minor);
					if (p.url) {
						refLink = p.url;
					}
					break;
				}
			}
		}

		if (!refDesc.empty()) {
			// Found the runtime DLL.
			break;
		}
	}

	return (!refDesc.empty() ? 0 : -ENOENT);
}

/**
 * Add fields for NE executables.
 */
void EXEPrivate::addFields_NE(void)
{
	// Up to 5 tabs.
	fields.reserveTabs(5);

	// NE Header
	fields.setTabName(0, "NE");
	fields.setTabIndex(0);

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

	// Target OS
	const char *targetOS = nullptr;
	if (hdr.ne.targOS == NE_OS_UNKNOWN) {
		// Either old OS/2 or Windows 1.x/2.x.
		targetOS = (hasKernel ? "Windows 1.x/2.x" : "Old OS/2");
	} else if (hdr.ne.targOS < NE_TargetOSes.size()) {
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
		fields.addField_string(targetOS_title, targetOS);
	} else {
		fields.addField_string(targetOS_title,
			fmt::format(C_("RomData", "Unknown (0x{:0>2X})"), hdr.ne.targOS));
	}

	// DGroup type.
	static const array<const char*, 4> dgroupTypes = {{
		NOP_C_("EXE|DGroupType", "None"),
		NOP_C_("EXE|DGroupType", "Single Shared"),
		NOP_C_("EXE|DGroupType", "Multiple"),
		NOP_C_("EXE|DGroupType", "(null)"),
	}};
	fields.addField_string(C_("EXE", "DGroup Type"),
		pgettext_expr("EXE|DGroupType", dgroupTypes[hdr.ne.ProgFlags & 3]));

	// Program flags
	static const array<const char*, 8> ProgFlags_names = {{
		nullptr, nullptr,	// DGroup Type
		NOP_C_("EXE|ProgFlags", "Global Init"),
		NOP_C_("EXE|ProgFlags", "Protected Mode Only"),
		NOP_C_("EXE|ProgFlags", "8086 insns"),
		NOP_C_("EXE|ProgFlags", "80286 insns"),
		NOP_C_("EXE|ProgFlags", "80386 insns"),
		NOP_C_("EXE|ProgFlags", "FPU insns"),
	}};
	vector<string> *const v_ProgFlags_names = RomFields::strArrayToVector_i18n("EXE|ProgFlags", ProgFlags_names);
	fields.addField_bitfield("Program Flags",
		v_ProgFlags_names, 2, hdr.ne.ProgFlags);

	// Application type
	const char *applType;
	if (hdr.ne.targOS == NE_OS_OS2) {
		// Only mentioning Presentation Manager for OS/2 executables.
		static const array<const char*, 4> applTypes_OS2 = {{
			NOP_C_("EXE|ApplType", "None"),
			NOP_C_("EXE|ApplType", "Full Screen (not aware of Presentation Manager)"),
			NOP_C_("EXE|ApplType", "Presentation Manager compatible"),
			NOP_C_("EXE|ApplType", "Presentation Manager application"),
		}};
		applType = applTypes_OS2[hdr.ne.ApplFlags & 3];
	} else {
		// Assume Windows for everything else.
		static const array<const char*, 4> applTypes_Win = {{
			NOP_C_("EXE|ApplType", "None"),
			NOP_C_("EXE|ApplType", "Full Screen (not aware of Windows)"),
			NOP_C_("EXE|ApplType", "Windows compatible"),
			NOP_C_("EXE|ApplType", "Windows application"),
		}};
		applType = applTypes_Win[hdr.ne.ApplFlags & 3];
	}
	fields.addField_string(C_("EXE", "Application Type"),
		pgettext_expr("EXE|ApplType", applType));

	// Application flags
	static const array<const char*, 8> ApplFlags_names = {{
		nullptr, nullptr,	// Application type
		nullptr,
		NOP_C_("EXE|ApplFlags", "OS/2 Application"),
		nullptr,
		NOP_C_("EXE|ApplFlags", "Image Error"),
		NOP_C_("EXE|ApplFlags", "Non-Conforming"),
		NOP_C_("EXE|ApplFlags", "DLL"),
	}};
	vector<string> *const v_ApplFlags_names = RomFields::strArrayToVector_i18n("EXE|ApplFlags", ApplFlags_names);
	fields.addField_bitfield(C_("EXE", "Application Flags"),
		v_ApplFlags_names, 2, hdr.ne.ApplFlags);

	// Other flags
	// NOTE: Indicated as OS/2 flags by OSDev Wiki,
	// but may be set on Windows programs too.
	// References:
	// - http://wiki.osdev.org/NE
	// - http://www.program-transformation.org/Transform/PcExeFormat
	static const array<const char*, 4> OtherFlags_names = {{
		NOP_C_("EXE|OtherFlags", "Long File Names"),
		NOP_C_("EXE|OtherFlags", "Protected Mode"),
		NOP_C_("EXE|OtherFlags", "Proportional Fonts"),
		NOP_C_("EXE|OtherFlags", "Gangload Area"),
	}};
	vector<string> *const v_OtherFlags_names = RomFields::strArrayToVector_i18n("EXE|OtherFlags", OtherFlags_names);
	fields.addField_bitfield(C_("EXE", "Other Flags"),
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
			fields.addField_dateTime(C_("EXE", "Timestamp"), ne_time,
				RomFields::RFT_DATETIME_HAS_DATE |
				RomFields::RFT_DATETIME_HAS_TIME |
				RomFields::RFT_DATETIME_IS_UTC);	// no timezone
		}
	}

	// Expected Windows version
	// TODO: Is this used in OS/2 executables?
	if (hdr.ne.targOS == NE_OS_WIN || hdr.ne.targOS == NE_OS_WIN386) {
		fields.addField_string(C_("EXE", "Windows Version"),
			fmt::format(FSTR("{:d}.{:d}"), hdr.ne.expctwinver[1], hdr.ne.expctwinver[0]));
	}

	// Runtime DLL
	// NOTE: Strings were obtained earlier.
	if (hdr.ne.targOS == NE_OS_WIN && !runtime_dll.empty()) {
		// TODO: Show the link?
		fields.addField_string(C_("EXE", "Runtime DLL"), runtime_dll);
	}

	// Module Name and Module Description
	auto get_first_string = [](span<const char> sp, string &out) -> bool {
		if (sp.size() == 0)
			return false;
		const size_t len = static_cast<uint8_t>(sp[0]);
		if (!len || sp.size() - 1 < len)
			return false;
		out.assign(sp.data() + 1, len);
		return true;
	};
	string module_name, module_desc;
	if (loadNEResident() == 0 && get_first_string(ne_resident_name_table, module_name))
		fields.addField_string(C_("EXE", "Module Name"), module_name);
	if (loadNENonResidentNames() == 0 && get_first_string(ne_nonresident_name_table, module_desc))
		fields.addField_string(C_("EXE", "Module Description"), module_desc);

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
			fields.setTabName(1, C_("RomData", "Version"));
			fields.setTabIndex(1);
			addFields_VS_VERSION_INFO(&vsffi, &vssfi);
		}
	}

	// Add entries
	addFields_NE_Entry();

	// Add imports
	addFields_NE_Import();
}

/**
 * Add fields for NE entry table.
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::addFields_NE_Entry(void)
{
	int res = loadNEResident();
	if (res < 0)
		return res;
	res = loadNENonResidentNames();
	if (res < 0)
		return res;

	struct Entry {
		span<const char> name;
		uint16_t ordinal;
		uint8_t flags;
		uint8_t segment;
		uint16_t offset;
		bool is_movable : 1;
		bool has_name : 1;
		bool is_resident : 1;
	};
	vector<Entry> ents;
	ents.reserve(ne_entry_table.size() / 4);

	// Read entry table
	auto p = ne_entry_table.begin();
	for (int ordinal = 1;;) {
		// Entry table consists of bundles of symbols
		// Each bundle is starts with count and segment of the symbols
		if (p >= ne_entry_table.end())
			return -ENOENT;
		const unsigned int bundle_count = *p++;
		if (bundle_count == 0)
			break; // end of table
		if (p >= ne_entry_table.end())
			return -ENOENT;
		const uint8_t bundle_segment = *p++;
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
			if (p + bundle_count*3 > ne_entry_table.end())
				return -ENOENT;
			for (unsigned int i = 0; i < bundle_count; i++) {
				Entry ent;
				ent.ordinal = ordinal++;
				ent.flags = p[0];
				ent.segment = bundle_segment;
				ent.offset = p[1] | p[2]<<8;
				ent.is_movable = false;
				ent.has_name = false;
				ent.is_resident = false;
				ents.emplace_back(std::move(ent));
				p += 3;
			}
			break;
		case 0xFF:
			/* Segment value 0xFF is used for movable segments.
			 * - DB flags
			 * - DW INT 3F
			 * - DB segment
			 * - DW offset */
			if (p + bundle_count*6 > ne_entry_table.end())
				return -ENOENT;
			for (unsigned int i = 0; i < bundle_count; i++) {
				if (p[1] != 0xCD && p[2] != 0x3F) // INT 3Fh
					return -ENOENT;
				Entry ent;
				ent.ordinal = ordinal++;
				ent.flags = p[0];
				ent.segment = p[3];
				ent.offset = p[4] | p[5]<<8;
				ent.is_movable = true;
				ent.has_name = false;
				ent.is_resident = false;
				ents.emplace_back(std::move(ent));
				p += 6;
			}
			break;
		}
	}

	/* Currently ents is sorted by ordinal. For duplicate names we'll add
	 * more entries, so we must remember the original size so we can do
	 * a binary search. */
	size_t last = ents.size();
	auto readNames = [&](span<const char> sp, bool is_resident) -> int {
		const char *p = sp.data();
		const char *const end = sp.data() + sp.size();
		if (p == end)
			return -ENOENT;
		/* Skip first string. For resident strings it's module name, and
		 * for non-resident strings it's module description. */
		p += static_cast<uint8_t>(*p) + 3;
		if (p >= end)
			return -ENOENT;

		while (*p) {
			const uint8_t len = static_cast<uint8_t>(*p++);
			if (p + len + 2 >= end) // next length byte >= end
				return -ENOENT;
			span<const char> name(p, len);
			const uint16_t ordinal = static_cast<uint8_t>(p[len]) | static_cast<uint8_t>(p[len+1])<<8;

			// binary search for the ordinal
			auto it = std::lower_bound(ents.begin(), ents.begin()+last, ordinal,
				[](const Entry &lhs, uint16_t rhs) noexcept -> bool {
					return (lhs.ordinal < rhs);
				});
			if (it == ents.begin()+last || it->ordinal != ordinal) {
				// name points to non-existent ordinal
				return -ENOENT;
			}

			if (it->has_name) {
				// This ordinal already has a name.
				// Duplicate the entry, replace name in the copy.
				Entry ent = *it;
				ent.name = name;
				ent.is_resident = is_resident;
				ents.emplace_back(std::move(ent)); // `it` is invalidated here
			} else {
				it->has_name = true;
				it->name = name;
				it->is_resident = is_resident;
			}

			p += len + 2;
		}
		return 0;
	};

	// Read names
	res = readNames(ne_resident_name_table, true);
	if (res)
		return res;
	res = readNames(ne_nonresident_name_table, false);
	if (res)
		return res;

	const char *const s_no_name = C_("EXE|Exports", "(No name)");
	const char *const s_address_mf = C_("EXE|Exports", "{0:0>2X}:{1:0>4X} ({2:s})");
	const char *const s_address_movable = C_("EXE|Exports", "Movable");
	const char *const s_address_fixed = C_("EXE|Exports", "Fixed");
	const char *const s_address_constant = C_("EXE|Exports", "0x{:0>4X} (Constant)");

	auto *const vv_data = new RomFields::ListData_t();
	vv_data->reserve(ents.size());
	for (const Entry &ent : ents) {
		vv_data->emplace_back();
		auto &row = vv_data->back();
		row.reserve(4);
		/* NODATA and RESIDENTNAME are from DEF files. EXPORT and PARAMS
		 * are names I made up (in DEF files you can't specify internal
		 * entries, and parameter count is specified by just a number).
		 * Typical flags values are 3 for exports, 0 for internal
		 * entries. */
		string flags;
		if (ent.flags & 1) {
			flags = "EXPORT ";
		}
		if (!(ent.flags & 2)) {
			flags += "NODATA ";
		}
		if (ent.flags & 4) {
			flags += "(bit 2) ";
		}
		/* Parameter count. I haven't found any module where this is
		 * actually used. */
		if (ent.flags & 0xF8) {
			flags += fmt::format(FSTR("PARAMS={:d} "), ent.flags>>3);
		}
		if (ent.has_name && ent.is_resident) {
			flags += "RESIDENTNAME ";
		}
		if (!flags.empty()) {
			flags.resize(flags.size()-1);
		}

		row.emplace_back(fmt::to_string(ent.ordinal));
		if (ent.has_name) {
			row.emplace_back(ent.name.data(), ent.name.size());
		} else {
			row.emplace_back(s_no_name);
		}
		if (ent.is_movable) {
			row.emplace_back(fmt::format(s_address_mf, ent.segment, ent.offset, s_address_movable));
		} else if (ent.segment != 0xFE) {
			row.emplace_back(fmt::format(s_address_mf, ent.segment, ent.offset, s_address_fixed));
		} else {
			row.emplace_back(fmt::format(s_address_constant, ent.offset));
		}
		row.emplace_back(std::move(flags));
	}

	// Create the tab if we have any exports.
	if (!vv_data->empty()) {
		// tr: this is the EXE NE equivalent of EXE PE export table
		fields.addTab(C_("EXE", "Entries"));
		fields.reserve(1);

		static const array<const char*, 4> field_names = {{
			NOP_C_("EXE|Exports", "Ordinal"),
			NOP_C_("EXE|Exports", "Name"),
			NOP_C_("EXE|Exports", "Address"),
			NOP_C_("EXE|Exports", "Flags"),
		}};
		vector<string> *const v_field_names = RomFields::strArrayToVector_i18n("EXE|Exports", field_names);

		RomFields::AFLD_PARAMS params;
		params.flags = RomFields::RFT_LISTDATA_SEPARATE_ROW;
		params.headers = v_field_names;
		params.data.single = vv_data;
		// TODO: Header alignment?
		params.col_attrs.align_data = AFLD_ALIGN4(TXA_R, TXA_D, TXA_D, TXA_D);
		params.col_attrs.sorting    = AFLD_ALIGN4(COLSORT_NUM, COLSORT_NC, COLSORT_STD, COLSORT_STD);
		params.col_attrs.sort_col   = 0;	// Ordinal
		params.col_attrs.sort_dir   = RomFields::COLSORTORDER_ASCENDING;
		fields.addField_listData(C_("EXE", "Entries"), &params);
	} else {
		delete vv_data;
	}
	return 0;
}

/**
 * Add fields for NE import table.
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::addFields_NE_Import(void)
{
	int res = loadNEResident();
	if (res < 0)
		return res;
	if (!file || !file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!isValid) {
		// Unknown executable type.
		return -EIO;
	} else if (exeType != ExeType::NE) {
		// Unsupported executable type.
		return -ENOTSUP;
	}

	// Helper funcs for reading import name table and modrefs.
	auto get_name = [&](size_t offset, string &out) -> bool {
		assert(offset < ne_imported_name_table.size());
		if (offset >= ne_imported_name_table.size())
			return false;
		const uint8_t count = static_cast<uint8_t>(ne_imported_name_table[offset]);

		assert(offset + 1 + count <= ne_imported_name_table.size());
		if (offset + 1 + count > ne_imported_name_table.size())
			return false;
		out.assign(&ne_imported_name_table[offset+1], count);
		return true;
	};
	auto get_modref = [&](size_t modref, string &out) -> bool {
		// NOTE: modref is 1-indexed (and this is not mentioned anywhere in the docs)
		assert(modref-1 < ne_modref_table.size());
		if (modref-1 >= ne_modref_table.size())
			return false;
		return get_name(le16_to_cpu(ne_modref_table[modref-1]), out);
	};

	/* IMPORTORDINAL
	 *   target1 --> modref index
	 *   target2 --> ordinal
	 * IMPORTNAME
	 *   target1 --> modref index
	 *   target2 --> imported names offset
	 */
	struct hash2x16 {
		size_t operator()(const std::pair<uint16_t, uint16_t> &p) const {
			return std::hash<uint32_t>()(p.first<<16 | p.second);
		}
	};
	std::unordered_set<std::pair<uint16_t, uint16_t>, hash2x16> ordinal_set, name_set;

	for (const auto &seg : ne_segment_table) {
		if (seg.offset == 0)
			continue; // No data
		if (!(le16_to_cpu(seg.flags) & NE_SEG_RELOCINFO))
			continue; // No relocations

		// the logic for seg_size is from Wine's NE_LoadSegment
		const size_t seg_offset = (size_t)le16_to_cpu(seg.offset) << le16_to_cpu(hdr.ne.FileAlnSzShftCnt);
		const size_t seg_size =
			seg.filesz ? le16_to_cpu(seg.filesz) :
			seg.memsz ? le16_to_cpu(seg.memsz) : 0x10000;
		if (seg_offset + seg_size < seg_offset)
			return -EIO; // Overflow

		uint16_t rel_count;
		size_t nread = file->seekAndRead(seg_offset + seg_size, &rel_count, 2);
		if (nread != 2)
			return -EIO; // Short read
		rel_count = le16_to_cpu(rel_count);

		rp::uvector<uint8_t> rel_buf;
		rel_buf.resize(rel_count*sizeof(NE_Reloc));
		nread = file->seekAndRead(seg_offset + seg_size + 2, rel_buf.data(), rel_buf.size());
		if (nread != rel_buf.size())
			return -EIO; // Short read
		auto relocs = reinterpret_span<const NE_Reloc>(rel_buf);

		for (const auto &reloc : relocs) {
			switch (reloc.flags & NE_REL_TARGET_MASK) {
			case NE_REL_IMPORTORDINAL:
				ordinal_set.emplace(le16_to_cpu(reloc.target1), le16_to_cpu(reloc.target2));
				break;
			case NE_REL_IMPORTNAME:
				name_set.emplace(le16_to_cpu(reloc.target1), le16_to_cpu(reloc.target2));
				break;
			}
		}
	}

	const char *const s_no_name = C_("EXE|Exports", "(No name)");

	auto *const vv_data = new RomFields::ListData_t();
	vv_data->reserve(ordinal_set.size() + name_set.size());
	for (const auto &imp : ordinal_set) {
		string modname;
		if (!get_modref(imp.first, modname))
			continue;

		std::vector<string> row;
		row.reserve(3);
		const char *const name = EXENEEntries::lookup_ordinal(modname.c_str(), imp.second);
		row.emplace_back(name ? name : s_no_name);
		row.emplace_back(fmt::to_string(imp.second));
		row.emplace_back(std::move(modname));
		vv_data->emplace_back(std::move(row));
	}
	for (const auto &imp : name_set) {
		string modname;
		if (!get_modref(imp.first, modname))
			continue;
		string name;
		if (!get_name(imp.second, name))
			continue;

		std::vector<string> row;
		row.reserve(3);
		row.emplace_back(std::move(name));
		row.emplace_back();
		row.emplace_back(std::move(modname));
		vv_data->emplace_back(std::move(row));
	}

	// Sort the list data by (module, name, ordinal).
	std::sort(vv_data->begin(), vv_data->end(),
		[](const vector<string> &lhs, const vector<string> &rhs) -> bool {
			// Vector index 0: Name
			// Vector index 1: Ordinal
			// Vector index 2: Module

			int res = strcasecmp(lhs[2].c_str(), rhs[2].c_str());
			if (res < 0) return true;
			if (res > 0) return false;

			res = strcasecmp(lhs[0].c_str(), rhs[0].c_str());
			if (res < 0) return true;
			if (res > 0) return false;

			// Numeric sort for ordinals.
			const unsigned long ord1 = strtoul(lhs[1].c_str(), nullptr, 10);
			const unsigned long ord2 = strtoul(rhs[1].c_str(), nullptr, 10);
			return (ord1 < ord2);
		});

	if (!vv_data->size()) {
		delete vv_data;
		return 0;
	}

	// Add the tab.
	fields.addTab(C_("EXE", "Imports"));
	fields.reserve(1);

	// Intentionally sharing the translation context with the exports tab.
	static const array<const char*, 3> field_names = {{
		NOP_C_("EXE|Exports", "Name"),
		NOP_C_("EXE|Exports", "Ordinal"),
		NOP_C_("EXE|Exports", "Module")
	}};
	vector<string> *const v_field_names = RomFields::strArrayToVector_i18n("EXE|Exports", field_names);

	RomFields::AFLD_PARAMS params;
	params.flags = RomFields::RFT_LISTDATA_SEPARATE_ROW;
	params.headers = v_field_names;
	params.data.single = vv_data;
	// TODO: Header alignment?
	params.col_attrs.align_data = AFLD_ALIGN3(TXA_D, TXA_R, TXA_D);
	params.col_attrs.sorting    = AFLD_ALIGN3(COLSORT_NC, COLSORT_NUM, COLSORT_NC);
	params.col_attrs.sort_col   = 0;	// Ordinal
	params.col_attrs.sort_dir   = RomFields::COLSORTORDER_ASCENDING;
	fields.addField_listData(C_("EXE", "Imports"), &params);
	return 0;
}

}
