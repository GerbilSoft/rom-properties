/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE_NE.cpp: DOS/Windows executable reader.                              *
 * 16-bit New Executable format.                                           *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
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
			// FIXME: Is it VBRUN400 or VBRUN416 for 16-bit?
			if (!strncmp(pDllName, "VBRUN400", 8) || !strncmp(pDllName, "VBRUN416", 8)) {
				refDesc = rp_sprintf(
					C_("EXE|Runtime", "Microsoft Visual Basic %s Runtime"), "4.0");
				if (refHasKernel)
					break;
			} else if (!strncmp(pDllName, "VBRUN300", 8)) {
				refDesc = rp_sprintf(
					C_("EXE|Runtime", "Microsoft Visual Basic %s Runtime"), "3.0");
				if (refHasKernel)
					break;
			} else if (!strncmp(pDllName, "VBRUN200", 8)) {
				refDesc = rp_sprintf(
					C_("EXE|Runtime", "Microsoft Visual Basic %s Runtime"), "2.0");
				if (refHasKernel)
					break;
			} else if (!strncmp(pDllName, "VBRUN100", 8)) {
				refDesc = rp_sprintf(
					C_("EXE|Runtime", "Microsoft Visual Basic %s Runtime"), "1.0");
				refLink = "https://download.microsoft.com/download/vb30/sampleaa/1/w9xnt4/en-us/vbrun100.exe";
				if (refHasKernel)
					break;
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
	// Up to 3 tabs.
	fields->reserveTabs(3);

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
	if (ret != 0 || !rsrcReader) {
		// Unable to load resources.
		// We're done here.
		return;
	}

	// Load the version resource.
	// NOTE: load_VS_VERSION_INFO loads it in host-endian.
	VS_FIXEDFILEINFO vsffi;
	IResourceReader::StringFileInfo vssfi;
	ret = rsrcReader->load_VS_VERSION_INFO(VS_VERSION_INFO, -1, &vsffi, &vssfi);
	if (ret != 0) {
		// Unable to load the version resource.
		// We're done here.
		return;
	}

	// Add the version fields.
	fields->setTabName(1, C_("EXE", "Version"));
	fields->setTabIndex(1);
	addFields_VS_VERSION_INFO(&vsffi, &vssfi);
}

}
