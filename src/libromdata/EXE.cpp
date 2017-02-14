/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE.cpp: DOS/Windows executable reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "EXE.hpp"
#include "RomData_p.hpp"

#include "exe_structs.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

class EXEPrivate : public RomDataPrivate
{
	public:
		EXEPrivate(EXE *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(EXEPrivate)

	public:
		// Executable type.
		enum ExeType {
			EXE_TYPE_UNKNOWN = -1,	// Unknown EXE type.

			EXE_TYPE_MZ = 0,	// DOS MZ
			EXE_TYPE_NE = 1,	// 16-bit New Executable
			EXE_TYPE_LE = 2,	// 32-bit Linear Executable
			EXE_TYPE_LX = 3,	// Mixed 16/32/64-bit Linear Executable
			EXE_TYPE_PE = 4,	// 32-bit Portable Executable
			EXE_TYPE_PE32PLUS = 5,	// 64-bit Portable Executable

			EXE_TYPE_LAST
		};
		int exeType;

	public:
		// DOS MZ header.
		IMAGE_DOS_HEADER mz;

		// PE header.
		#pragma pack(1)
		struct PACKED {
			uint32_t Signature;
			IMAGE_FILE_HEADER FileHeader;
			union {
				uint16_t Magic;
				IMAGE_OPTIONAL_HEADER32 opt32;
				IMAGE_OPTIONAL_HEADER64 opt64;
			} OptionalHeader;
		} pe;
		#pragma pack()
};

/** EXEPrivate **/

EXEPrivate::EXEPrivate(EXE *q, IRpFile *file)
	: super(q, file)
	, exeType(EXE_TYPE_UNKNOWN)
{
	// Clear the structs.
	memset(&mz, 0, sizeof(mz));
	memset(&pe, 0, sizeof(pe));
}

/** EXE **/

/**
 * Read a DOS/Windows executable.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
EXE::EXE(IRpFile *file)
	: super(new EXEPrivate(this, file))
{
	// This class handles executables.
	// TODO: Different type for DLL, etc.?
	RP_D(EXE);
	d->fileType = FTYPE_EXECUTABLE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the DOS MZ header.
	d->file->rewind();
	size_t size = d->file->read(&d->mz, sizeof(d->mz));
	if (size != sizeof(d->mz))
		return;

	// Check if this executable is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->mz);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->mz);
	info.ext = nullptr;	// Not needed for EXE.
	info.szFile = 0;	// Not needed for EXE.
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		// Not an MZ executable.
		return;
	}

	// NOTE: isRomSupported_static() only determines if the
	// file has a DOS MZ executable stub. The actual executable
	// type is determined here.

	// If the relocation table address is less than 0x40,
	// it's an MS-DOS executable.
	if (le16_to_cpu(d->mz.e_lfarlc) < 0x40) {
		// MS-DOS executable.
		d->exeType = EXEPrivate::EXE_TYPE_MZ;
		return;
	}

	// Load the PE header.
	// TODO: NE/LE/LX.
	uint32_t pe_addr = le32_to_cpu(d->mz.e_lfanew);
	if (pe_addr < sizeof(d->mz) || pe_addr >= (d->file->size() - sizeof(d->pe))) {
		// PE header address is out of range.
		d->exeType = EXEPrivate::EXE_TYPE_MZ;
		return;
	}

	int ret = d->file->seek(pe_addr);
	if (ret != 0) {
		// Seek error.
		d->exeType = EXEPrivate::EXE_TYPE_UNKNOWN;
		d->isValid = false;
		return;
	}
	size = d->file->read(&d->pe, sizeof(d->pe));
	if (size != sizeof(d->pe)) {
		// Read error.
		d->exeType = EXEPrivate::EXE_TYPE_UNKNOWN;
		d->isValid = false;
		return;
	}

	// Verify the PE signature.
	// FIXME: MSVC handles 'PE\0\0' as 0x00504500.
	if (be32_to_cpu(d->pe.Signature) != 0x50450000 /*'PE\0\0'*/) {
		// Not a PE executable.
		d->exeType = EXEPrivate::EXE_TYPE_MZ;
		return;
	}

	// This is a PE executable.
	// Check if it's PE or PE32+. (TODO: .NET?)
	switch (le16_to_cpu(d->pe.OptionalHeader.Magic)) {
		case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
			d->exeType = EXEPrivate::EXE_TYPE_PE;
			break;
		case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
			d->exeType = EXEPrivate::EXE_TYPE_PE32PLUS;
			break;
		default:
			// Unsupported PE executable.
			d->exeType = EXEPrivate::EXE_TYPE_UNKNOWN;
			d->isValid = false;
			return;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int EXE::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(IMAGE_DOS_HEADER))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const IMAGE_DOS_HEADER *const pMZ =
		reinterpret_cast<const IMAGE_DOS_HEADER*>(info->header.pData);

	// Check the magic number.
	// NOTE: 'MZ' is stored as a string, so we have to
	// handle it as if it's big-endian.
	if (be16_to_cpu(pMZ->e_magic) == 'MZ') {
		// This is a DOS "MZ" executable.
		// Specific subtypes are checked later.
		return EXEPrivate::EXE_TYPE_MZ;
	}

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int EXE::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *EXE::systemName(uint32_t type) const
{
	RP_D(const EXE);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// TODO: DOS, Windows, etc.
	// Bits 0-1: Type. (short, long, abbreviation)
	static const rp_char *const sysNames[4] = {
		// FIXME: "NGC" in Japan?
		_RP("DOS/Windows EXE (placeholder)"), _RP("DOS/Windows"), _RP("EXE"), nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> EXE::supportedFileExtensions_static(void)
{
	// References:
	// - https://en.wikipedia.org/wiki/Portable_Executable
	static const rp_char *const exts[] = {
		// PE extensions
		_RP(".exe"), _RP(".dll"),
		_RP(".acm"), _RP(".ax"),
		_RP(".cpl"), _RP(".drv"),
		_RP(".efi"), _RP(".mui"),
		_RP(".ocx"), _RP(".scr"),
		_RP(".sys"), _RP(".tsp"),

		// LE extensions
		_RP("*.vxd"),
	};
	return vector<const rp_char*>(exts, exts + ARRAY_SIZE(exts));
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> EXE::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int EXE::loadFieldData(void)
{
	RP_D(EXE);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->exeType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	d->fields->reserve(6);	// Maximum of 6 fields.

	// Executable type.
	static const rp_char *const exeTypes[EXEPrivate::EXE_TYPE_LAST] = {
		_RP("MS-DOS Executable"),		// EXE_TYPE_MZ
		_RP("16-bit New Executable"),		// EXE_TYPE_NE
		_RP("32-bit Linear Executable"),	// EXE_TYPE_LE
		_RP("Mixed-Mode Linear Executable"),	// EXE_TYPE_LX
		_RP("32-bit Portable Executable"),	// EXE_TYPE_PE
		_RP("64-bit Portable Executable"),	// EXE_TYPE_PE32PLUS
	};
	if (d->exeType >= EXEPrivate::EXE_TYPE_MZ && d->exeType < EXEPrivate::EXE_TYPE_LAST) {
		d->fields->addField_string(_RP("Type"), exeTypes[d->exeType]);
	} else {
		d->fields->addField_string(_RP("Type"), _RP("Unknown"));
	}

	switch (d->exeType) {
		case EXEPrivate::EXE_TYPE_PE:
		case EXEPrivate::EXE_TYPE_PE32PLUS: {
			// Common PE 32/64 fields.
			// TODO: bsearch()?
			const rp_char *cpu = nullptr;
			const uint16_t machine = le16_to_cpu(d->pe.FileHeader.Machine);
			switch (machine) {
				case IMAGE_FILE_MACHINE_AM33:
					cpu = _RP("Matsushita AM33");
					break;
				case IMAGE_FILE_MACHINE_AMD64:
					cpu = _RP("AMD64");
					break;
				case IMAGE_FILE_MACHINE_ARM:
					cpu = _RP("ARM");
					break;
				case IMAGE_FILE_MACHINE_EBC:
					cpu = _RP("EFI Byte Code");
					break;
				case IMAGE_FILE_MACHINE_I386:
					cpu = _RP("Intel i386");
					break;
				case IMAGE_FILE_MACHINE_IA64:
					cpu = _RP("Intel Itanium");
					break;
				case IMAGE_FILE_MACHINE_M32R:
					cpu = _RP("Mitsubishi M32R");
					break;
				case IMAGE_FILE_MACHINE_MIPS16:
					cpu = _RP("MIPS16");
					break;
				case IMAGE_FILE_MACHINE_MIPSFPU:
					cpu = _RP("MIPS with FPU");
					break;
				case IMAGE_FILE_MACHINE_MIPSFPU16:
					cpu = _RP("MIPS16 with FPU");
					break;
				case IMAGE_FILE_MACHINE_POWERPC:
					cpu = _RP("PowerPC");
					break;
				case IMAGE_FILE_MACHINE_POWERPCFP:
					cpu = _RP("PowerPC with FPU");
					break;
				case IMAGE_FILE_MACHINE_R4000:
					cpu = _RP("MIPS R4000");
					break;
				case IMAGE_FILE_MACHINE_SH3:
					cpu = _RP("Hitachi SH3");
					break;
				case IMAGE_FILE_MACHINE_SH3DSP:
					cpu = _RP("Hitachi SH3 DSP");
					break;
				case IMAGE_FILE_MACHINE_SH4:
					cpu = _RP("Hitachi SH4");
					break;
				case IMAGE_FILE_MACHINE_SH5:
					cpu = _RP("Hitachi SH5");
					break;
				case IMAGE_FILE_MACHINE_THUMB:
					cpu = _RP("ARM Thumb");
					break;
				case IMAGE_FILE_MACHINE_WCEMIPSV2:
					cpu = _RP("MIPS (WCE v2)");
					break;
				default:
					break;
			}

			if (cpu != nullptr) {
				d->fields->addField_string(_RP("CPU"), cpu);
			} else {
				char buf[32];
				int len = snprintf(buf, sizeof(buf), "Unknown (0x%04X)", machine);
				if (len > (int)sizeof(buf))
					len = (int)sizeof(buf);
				d->fields->addField_string(_RP("CPU"),
					len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));
			}

			const uint32_t pe_flags = d->pe.FileHeader.Characteristics;

			// Get the architecture-specific fields.
			uint16_t subsystem;
			uint32_t dll_flags;
			if (d->exeType == EXEPrivate::EXE_TYPE_PE) {
				subsystem = d->pe.OptionalHeader.opt32.Subsystem;
				dll_flags = d->pe.OptionalHeader.opt32.DllCharacteristics;
			} else /*if (d->exeType == EXEPrivate::EXE_TYPE_PE32PLUS)*/ {
				subsystem = d->pe.OptionalHeader.opt64.Subsystem;
				dll_flags = d->pe.OptionalHeader.opt64.DllCharacteristics;
			}

			// Subsystem names.
			static const rp_char *const subsysNames[IMAGE_SUBSYSTEM_XBOX+1] = {
				nullptr,			// IMAGE_SUBSYSTEM_UNKNOWN
				_RP("Native"),			// IMAGE_SUBSYSTEM_NATIVE
				_RP("Windows"),			// IMAGE_SUBSYSTEM_WINDOWS_GUI
				_RP("Console"),			// IMAGE_SUBSYSTEM_WINDOWS_CUI
				nullptr,
				_RP("OS/2 Console"),		// IMAGE_SUBSYSTEM_OS2_CUI
				nullptr,
				_RP("POSIX Console"),		// IMAGE_SUBSYSTEM_POSIX_CUI
				_RP("Win9x Native Driver"),	// IMAGE_SUBSYSTEM_NATIVE_WINDOWS
				_RP("Windows CE"),		// IMAGE_SUBSYSTEM_WINDOWS_CE_GUI
				_RP("EFI Application"),		// IMAGE_SUBSYSTEM_EFI_APPLICATION
				_RP("EFI Boot Service Driver"),	// IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER
				_RP("EFI Runtime Driver"),	// IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER
				_RP("EFI ROM Image"),		// IMAGE_SUBSYSTEM_EFI_ROM
				_RP("Xbox"),			// IMAGE_SUBSYSTEM_XBOX
			};
			if (subsystem < ARRAY_SIZE(subsysNames)) {
				d->fields->addField_string(_RP("Subsystem"), subsysNames[subsystem]);
			} else {
				char buf[32];
				int len = snprintf(buf, sizeof(buf), "Unknown (%u)", subsystem);
				if (len > (int)sizeof(buf))
					len = (int)sizeof(buf);
				d->fields->addField_string(_RP("Subsystem"),
					len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));
			}

			// PE flags. (characteristics)
			// NOTE: Only important flags will be listed.
			static const rp_char *const pe_flags_names[] = {
				nullptr, _RP("Executable"), nullptr,
				nullptr, nullptr, _RP(">2GB addressing"),
				nullptr, nullptr, nullptr,
				nullptr, nullptr, nullptr,
				nullptr, _RP("DLL"), nullptr,
				nullptr
			};
			vector<rp_string> *v_pe_flags_names = RomFields::strArrayToVector(
				pe_flags_names, ARRAY_SIZE(pe_flags_names));
			d->fields->addField_bitfield(_RP("PE Flags"),
				v_pe_flags_names, 3, pe_flags);

			// DLL flags. (characteristics)
			static const rp_char *const dll_flags_names[] = {
				nullptr, nullptr, nullptr,
				nullptr, nullptr, _RP("High Entropy VA"),
				_RP("Dynamic Base"), _RP("Force Integrity"), _RP("NX Compatible"),
				_RP("No Isolation"), _RP("No SEH"), _RP("No Bind"),
				_RP("AppContainer"), _RP("WDM Driver"), _RP("Control Flow Guard"),
				_RP("TS Aware"),
			};
			vector<rp_string> *v_dll_flags_names = RomFields::strArrayToVector(
				dll_flags_names, ARRAY_SIZE(dll_flags_names));
			d->fields->addField_bitfield(_RP("DLL Flags"),
				v_dll_flags_names, 3, dll_flags);
			break;
		}

		default:
			// TODO: Other executable types.
			break;
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

}
