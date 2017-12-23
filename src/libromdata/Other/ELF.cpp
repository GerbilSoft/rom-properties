/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ELF.cpp: Executable and Linkable Format reader.                         *
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

#include "ELF.hpp"
#include "librpbase/RomData_p.hpp"

#include "data/ELFData.hpp"
#include "elf_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

class ELFPrivate : public LibRpBase::RomDataPrivate
{
	public:
		ELFPrivate(ELF *q, LibRpBase::IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(ELFPrivate)

	public:
		// ELF format.
		enum Elf_Format {
			ELF_FORMAT_UNKNOWN	= -1,
			ELF_FORMAT_32LSB	= 0,
			ELF_FORMAT_64LSB	= 1,
			ELF_FORMAT_32MSB	= 2,
			ELF_FORMAT_64MSB	= 3,

			// Host/swap endian formats.

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
			ELF_FORMAT_32HOST	= ELF_FORMAT_32LSB,
			ELF_FORMAT_64HOST	= ELF_FORMAT_64LSB,
			ELF_FORMAT_32SWAP	= ELF_FORMAT_32MSB,
			ELF_FORMAT_64SWAP	= ELF_FORMAT_64MSB,
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
			ELF_FORMAT_32HOST	= ELF_FORMAT_32MSB,
			ELF_FORMAT_64HOST	= ELF_FORMAT_64MSB,
			ELF_FORMAT_32SWAP	= ELF_FORMAT_32LSB,
			ELF_FORMAT_64SWAP	= ELF_FORMAT_64LSB,
#endif

			ELF_FORMAT_MAX
		};
		int elfFormat;

		// ELF header.
		union {
			Elf_PrimaryEhdr primary;
			Elf32_Ehdr elf32;
			Elf64_Ehdr elf64;
		} Elf_Header;
};

/** ELFPrivate **/

ELFPrivate::ELFPrivate(ELF *q, IRpFile *file)
	: super(q, file)
	, elfFormat(ELF_FORMAT_UNKNOWN)
{
	// Clear the structs.
	memset(&Elf_Header, 0, sizeof(Elf_Header));
}

/** ELF **/

/**
 * Read an ELF executable.
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
ELF::ELF(IRpFile *file)
	: super(new ELFPrivate(this, file))
{
	// This class handles different types of files.
	// d->fileType will be set later.
	RP_D(ELF);
	d->className = "ELF";
	d->fileType = FTYPE_UNKNOWN;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Assume this is a 64-bit ELF executable and read a 64-bit header.
	// 32-bit executables have a smaller header, but they should have
	// more data than just the header.
	d->file->rewind();
	size_t size = d->file->read(&d->Elf_Header, sizeof(d->Elf_Header));
	if (size != sizeof(d->Elf_Header))
		return;

	// Check if this executable is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->Elf_Header);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->Elf_Header);
	info.ext = nullptr;	// Not needed for ELF.
	info.szFile = 0;	// Not needed for ELF.
	d->elfFormat = isRomSupported_static(&info);

	d->isValid = (d->elfFormat >= 0);
	if (!d->isValid) {
		// Not an ELF executable.
		return;
	}

	// Swap endianness if needed.
	switch (d->elfFormat) {
		default:
			// Unsupported...
			d->isValid = false;
			d->elfFormat = ELFPrivate::ELF_FORMAT_UNKNOWN;
			return;

		case ELFPrivate::ELF_FORMAT_32HOST:
		case ELFPrivate::ELF_FORMAT_64HOST:
			// Host-endian. Nothing to do.
			break;

		case ELFPrivate::ELF_FORMAT_32SWAP: {
			// 32-bit, swapped endian.
			Elf32_Ehdr *const elf32 = &d->Elf_Header.elf32;
			elf32->e_type		= __swab16(elf32->e_type);
			elf32->e_machine	= __swab16(elf32->e_machine);
			elf32->e_version	= __swab32(elf32->e_version);
			elf32->e_entry		= __swab32(elf32->e_entry);
			elf32->e_phoff		= __swab32(elf32->e_phoff);
			elf32->e_shoff		= __swab32(elf32->e_shoff);
			elf32->e_flags		= __swab32(elf32->e_flags);
			elf32->e_ehsize		= __swab16(elf32->e_ehsize);
			elf32->e_phentsize	= __swab16(elf32->e_phentsize);
			elf32->e_phnum		= __swab16(elf32->e_phnum);
			elf32->e_shentsize	= __swab16(elf32->e_shentsize);
			elf32->e_shnum		= __swab16(elf32->e_shnum);
			elf32->e_shstrndx	= __swab16(elf32->e_shstrndx);
			break;
		}

		case ELFPrivate::ELF_FORMAT_64SWAP: {
			// 64-bit, swapped endian.
			Elf64_Ehdr *const elf64 = &d->Elf_Header.elf64;
			elf64->e_type		= __swab16(elf64->e_type);
			elf64->e_machine	= __swab16(elf64->e_machine);
			elf64->e_version	= __swab32(elf64->e_version);
			elf64->e_entry		= __swab64(elf64->e_entry);
			elf64->e_phoff		= __swab64(elf64->e_phoff);
			elf64->e_shoff		= __swab64(elf64->e_shoff);
			elf64->e_flags		= __swab32(elf64->e_flags);
			elf64->e_ehsize		= __swab16(elf64->e_ehsize);
			elf64->e_phentsize	= __swab16(elf64->e_phentsize);
			elf64->e_phnum		= __swab16(elf64->e_phnum);
			elf64->e_shentsize	= __swab16(elf64->e_shentsize);
			elf64->e_shnum		= __swab16(elf64->e_shnum);
			elf64->e_shstrndx	= __swab16(elf64->e_shstrndx);
			break;
		}
	}

	// Determine the file type.
	switch (d->Elf_Header.primary.e_type) {
		case ELF_TYPE_RELOCATABLE:
			d->fileType = FTYPE_RELOCATABLE_OBJECT;
			break;
		case ELF_TYPE_EXECUTABLE:
			d->fileType = FTYPE_EXECUTABLE;
			break;
		case ELF_TYPE_SHARED:
			// TODO: Detect PIE.
			d->fileType = FTYPE_SHARED_LIBRARY;
			break;
		case ELF_TYPE_CORE:
			d->fileType = FTYPE_CORE_DUMP;
			break;
		default:
			// Should not happen...
			break;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int ELF::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(Elf_PrimaryEhdr))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// TODO: Use 32-bit and/or 16-bit reads to improve performance.
	// (Manual vectorization.)

	const Elf_PrimaryEhdr *const pHdr =
		reinterpret_cast<const Elf_PrimaryEhdr*>(info->header.pData);

	// Check the magic number.
	if (!memcmp(pHdr->e_magic, ELF_MAGIC, sizeof(pHdr->e_magic))) {
		// Verify the bitness and endianness fields.
		switch (pHdr->e_endianness) {
			case ELF_ENDIANNESS_LSB:
				// Little-endian.
				switch (pHdr->e_bitness) {
					case ELF_BITNESS_32BIT:
						// 32-bit LSB.
						return ELFPrivate::ELF_FORMAT_32LSB;
					case ELF_BITNESS_64BIT:
						// 64-bit LSB.
						return ELFPrivate::ELF_FORMAT_64LSB;
					default:
						// Unknown bitness.
						break;
				}
				break;

			case ELF_ENDIANNESS_MSB:
				// Big-endian.
				switch (pHdr->e_bitness) {
					case ELF_BITNESS_32BIT:
						// 32-bit MSB.
						return ELFPrivate::ELF_FORMAT_32MSB;
					case ELF_BITNESS_64BIT:
						// 64-bit MSB.
						return ELFPrivate::ELF_FORMAT_64MSB;
					default:
						// Unknown bitness.
						break;
				}
				break;

			default:
				// Unknown endianness.
				break;
		}
	}

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int ELF::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *ELF::systemName(unsigned int type) const
{
	RP_D(const ELF);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// TODO: Identify the OS, or list that in the fields instead?
	static_assert(SYSNAME_TYPE_MASK == 3,
		"ELF::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Executable and Linkable Format", "ELF", "ELF", nullptr
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
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *ELF::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".",		// FIXME: Does this work for files with no extension?
		".elf",		// Common for Wii homebrew.
		".so",		// Shared libraries. (TODO: Versioned .so files.)
		".o",		// Relocatable object files.
		".core",	// Core dumps.
		".debug",	// Split debug files.

		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *ELF::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int ELF::loadFieldData(void)
{
	RP_D(ELF);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unsupported file.
		return -EIO;
	}

	// Primary ELF header.
	const Elf_PrimaryEhdr *const primary = &d->Elf_Header.primary;
	d->fields->reserve(2);	// Maximum of 2 fields.

	// NOTE: Executable type is used as File Type.
	// TODO: Add more fields.

	// CPU.
	const char *const cpu = ELFData::lookup_cpu(primary->e_machine);
	if (cpu) {
		d->fields->addField_string(C_("ELF", "CPU"), cpu);
	} else {
		d->fields->addField_string(C_("ELF", "CPU"),
			rp_sprintf(C_("ELF", "Unknown (0x%04X)"), primary->e_machine));
	}

	// Bitness/Endianness. (consolidated as "format")
	static const char *const elf_formats[] = {
		"32-bit Little-Endian",
		"64-bit Little-Endian",
		"32-bit Big-Endian",
		"64-bit Big-Endian",
	};
	if (d->elfFormat > ELFPrivate::ELF_FORMAT_UNKNOWN &&
	    d->elfFormat < ARRAY_SIZE(elf_formats))
	{
		d->fields->addField_string(C_("ELF", "Format"), elf_formats[d->elfFormat]);
	}
	else
	{
		// TODO: Show individual values.
		// NOTE: This shouldn't happen...
		d->fields->addField_string(C_("ELF", "Format"), C_("ELF", "Unknown"));
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

}
