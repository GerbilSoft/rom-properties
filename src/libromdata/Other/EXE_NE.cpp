/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE_NE.cpp: DOS/Windows executable reader.                              *
 * 16-bit New Executable format.                                           *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "EXE_p.hpp"
#include "disc/NEResourceReader.hpp"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cerrno>

// C++ includes.
#include <string>
#include <vector>
using std::string;
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
	} else if (exeType != EXE_TYPE_NE) {
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
		delete rsrcReader;
		rsrcReader = nullptr;
		return (err != 0 ? err : -EIO);
	}

	// Resource table loaded.
	return 0;
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

	// Target OS.
	const char *targetOS = (hdr.ne.targOS < ARRAY_SIZE(NE_TargetOSes))
					? NE_TargetOSes[hdr.ne.targOS]
					: nullptr;
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

	// Load resources.
	int ret = loadNEResourceTable();
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
