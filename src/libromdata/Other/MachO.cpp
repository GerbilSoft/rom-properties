/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MachO.cpp: Mach-O executable format.                                    *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "MachO.hpp"
#include "librpbase/RomData_p.hpp"

#include "macho_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData {

ROMDATA_IMPL(MachO)

class MachOPrivate : public LibRpBase::RomDataPrivate
{
	public:
		MachOPrivate(MachO *q, LibRpBase::IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(MachOPrivate)

	public:
		// Mach-O format.
		enum Mach_Format {
			MACH_FORMAT_UNKNOWN	= -1,
			MACH_FORMAT_32LSB	= 0,
			MACH_FORMAT_64LSB	= 1,
			MACH_FORMAT_32MSB	= 2,
			MACH_FORMAT_64MSB	= 3,

			// Host/swap endian formats.

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
			MACH_FORMAT_32HOST	= MACH_FORMAT_32LSB,
			MACH_FORMAT_64HOST	= MACH_FORMAT_64LSB,
			MACH_FORMAT_32SWAP	= MACH_FORMAT_32MSB,
			MACH_FORMAT_64SWAP	= MACH_FORMAT_64MSB,
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
			MACH_FORMAT_32HOST	= MACH_FORMAT_32MSB,
			MACH_FORMAT_64HOST	= MACH_FORMAT_64MSB,
			MACH_FORMAT_32SWAP	= MACH_FORMAT_32LSB,
			MACH_FORMAT_64SWAP	= MACH_FORMAT_64LSB,
#endif

			MACHO_FORMAT_MAX
		};
		int machFormat;

		// Mach-O header.
		mach_header machHeader;
};

/** MachOPrivate **/

MachOPrivate::MachOPrivate(MachO *q, IRpFile *file)
	: super(q, file)
	, machFormat(MACH_FORMAT_UNKNOWN)
{
	// Clear the structs.
	memset(&machHeader, 0, sizeof(machHeader));
}

/** MachO **/

/**
 * Read a Mach-O executable.
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
MachO::MachO(IRpFile *file)
	: super(new MachOPrivate(this, file))
{
	// This class handles different types of files.
	// d->fileType will be set later.
	RP_D(MachO);
	d->className = "MachO";
	d->fileType = FTYPE_UNKNOWN;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the Mach-O header.
	d->file->rewind();
	size_t size = d->file->read(&d->machHeader, sizeof(d->machHeader));
	if (size != sizeof(d->machHeader)) {
		delete d->file;
		d->file = nullptr;
		return;
	}

	// Check if this executable is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->machHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->machHeader);
	info.ext = nullptr;	// Not needed for ELF.
	info.szFile = 0;	// Not needed for ELF.
	d->machFormat = isRomSupported_static(&info);

	d->isValid = (d->machFormat >= 0);
	if (!d->isValid) {
		// Not a Mach-O executable.
		delete d->file;
		d->file = nullptr;
		return;
	}

	// Swap endianness if needed.
	switch (d->machFormat) {
		default:
			// Should not get here...
			d->isValid = false;
			d->machFormat = MachOPrivate::MACH_FORMAT_UNKNOWN;
			delete d->file;
			d->file = nullptr;
			return;

		case MachOPrivate::MACH_FORMAT_32HOST:
		case MachOPrivate::MACH_FORMAT_64HOST:
			// Host-endian. Nothing to do.
			break;

		case MachOPrivate::MACH_FORMAT_32SWAP:
		case MachOPrivate::MACH_FORMAT_64SWAP: {
			// Swapped endian.
			// NOTE: Not swapping the magic number.
			mach_header *const machHeader = &d->machHeader;
			machHeader->cputype	= __swab32(machHeader->cputype);
			machHeader->cpusubtype	= __swab32(machHeader->cpusubtype);
			machHeader->filetype	= __swab32(machHeader->filetype);
			machHeader->ncmds	= __swab32(machHeader->ncmds);
			machHeader->sizeofcmds	= __swab32(machHeader->sizeofcmds);
			machHeader->flags	= __swab32(machHeader->flags);
			break;
		}
	}

	// Determine the file type.
	switch (d->machHeader.filetype) {
		default:
			// Should not happen...
			d->fileType = FTYPE_UNKNOWN;
			break;
		case MH_OBJECT:
			d->fileType = FTYPE_RELOCATABLE_OBJECT;
			break;
		case MH_EXECUTE:
		case MH_PRELOAD:
			// TODO: Special FTYPE for MH_PRELOAD?
			d->fileType = FTYPE_EXECUTABLE;
			break;
		case MH_FVMLIB:
			// "Fixed VM" library file.
			// TODO: Add a separate FTYPE?
			d->fileType = FTYPE_SHARED_LIBRARY;
			break;
		case MH_CORE:
			d->fileType = FTYPE_CORE_DUMP;
			break;
		case MH_DYLIB:
			d->fileType = FTYPE_SHARED_LIBRARY;
			break;
		case MH_DYLINKER:
			// TODO
			d->fileType = FTYPE_UNKNOWN;
			break;
		case MH_BUNDLE:
			d->fileType = FTYPE_BUNDLE;
			break;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int MachO::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(mach_header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const mach_header *const machHeader =
		reinterpret_cast<const mach_header*>(info->header.pData);

	// Check the magic number.
	// NOTE: Checking in order of Mac OS X usage as of 2019.
	if (machHeader->magic == cpu_to_le32(MH_MAGIC_64)) {
		// LE64 magic number.
		return MachOPrivate::MACH_FORMAT_64LSB;
	} else if (machHeader->magic == cpu_to_le32(MH_MAGIC)) {
		// LE32 magic number.
		return MachOPrivate::MACH_FORMAT_32LSB;
	} else if (machHeader->magic == cpu_to_be32(MH_MAGIC)) {
		// BE32 magic number.
		return MachOPrivate::MACH_FORMAT_32MSB;
	} else if (machHeader->magic == cpu_to_be32(MH_MAGIC_64)) {
		// BE64 magic number.
		return MachOPrivate::MACH_FORMAT_64MSB;
	}

	// Not supported.
	return MachOPrivate::MACH_FORMAT_UNKNOWN;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *MachO::systemName(unsigned int type) const
{
	RP_D(const MachO);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// TODO: Identify the OS, or list that in the fields instead?
	static_assert(SYSNAME_TYPE_MASK == 3,
		"MachO::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Mach-O Microkernel", "Mach-O", "Mach-O", nullptr
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
const char *const *MachO::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		//".",		// FIXME: Does this work for files with no extension?
		".bin",
		".so",		// Shared libraries. (TODO: Versioned .so files.)
		".dylib"	// Dynamic libraries. (TODO: Versioned .dylib files.)
		".bundle",	// Bundles.
		// TODO: More?

		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *MachO::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.

		// FIXME: Defining the magic numbers for Mach-O
		// executables in rom-properties.xml causes
		// KDE to lock up due to a conflict with the
		// standard definitions. Hence, we're using our
		// own types.

		// TODO: Upstream the Mach-O definitions.

		"application/x-mach-executable",
		"application/x-mach-sharedlib",
		"application/x-mach-core",
		"application/x-mach-bundle",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int MachO::loadFieldData(void)
{
	RP_D(MachO);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unsupported file.
		return -EIO;
	}

	// Mach-O header.
	const mach_header *const machHeader = &d->machHeader;
	d->fields->reserve(3);	// Maximum of 3 fields.

	// Executable format.
	static const char *const exec_type_tbl[] = {
		NOP_C_("RomData|ExecType", "32-bit Little-Endian"),
		NOP_C_("RomData|ExecType", "64-bit Little Endian"),
		NOP_C_("RomData|ExecType", "32-bit Big-Endian"),
		NOP_C_("RomData|ExecType", "64-bit Big-Endian"),
	};
	if (d->machFormat > MachOPrivate::MACH_FORMAT_UNKNOWN &&
	    d->machFormat < ARRAY_SIZE(exec_type_tbl))
	{
		d->fields->addField_string(C_("MachO", "Format"),
			dpgettext_expr(RP_I18N_DOMAIN, "RomData|ExecType", exec_type_tbl[d->machFormat]));
	} else {
		// TODO: Show individual values.
		// NOTE: This shouldn't happen...
		d->fields->addField_string(C_("MachO", "Format"),
			C_("RomData", "Unknown"));
	}

	// CPU.
	// TODO: Verify that ABI matches the format?
	const unsigned int abi = (machHeader->cputype >> 24) & 1;
	const unsigned int cpu = (machHeader->cputype & 0xFFFFFF);

	static const char *const cpu_tbl[2][19] = {
		{
			// 32-bit CPUs
			nullptr, "VAX", nullptr, "ROMP",
			"NS32032", "NS32332", "MC680x0", "i386",
			"MIPS", "NS32532", "MC98000", "HPPA",
			"ARM", "MC88000", "SPARC", "i860",
			"Alpha", "RS/6000", "PowerPC"
		},
		{
			// 64-bit CPUs
			nullptr, nullptr, nullptr, nullptr,
			nullptr, nullptr, nullptr, "amd64",
			nullptr, nullptr, nullptr, nullptr,
			"ARM64", nullptr, nullptr, nullptr,
			nullptr, nullptr, "PowerPC 64"
		}
	};

	const char *s_cpu = nullptr;
	if (cpu < ARRAY_SIZE(cpu_tbl[0])) {
		s_cpu = cpu_tbl[abi][cpu];
	}

	if (s_cpu) {
		d->fields->addField_string(C_("Mach-O", "CPU"), s_cpu);
	} else {
		d->fields->addField_string(C_("Mach-O", "CPU"),
			rp_sprintf(C_("ELF", "Unknown (%u)"), cpu));
	}

	// CPU subtype.
	const unsigned int cpu_subtype = (machHeader->cpusubtype & 0xFFFFFF);
	const char *s_cpu_subtype = nullptr;
	switch (cpu) {
		default:
			break;

		case CPU_TYPE_VAX: {
			static const char *const cpu_subtype_vax_tbl[] = {
				nullptr, "VAX-11/780", "VAX-11/785", "VAX-11/750",
				"VAX-11/730", "MicroVAX I", "MicroVAX II", "VAX 8200",
				"VAX 8500", "VAX 8600", "VAX 8650", "VAX 8800",
				"MicroVAX III"
			};
			if (cpu_subtype < ARRAY_SIZE(cpu_subtype_vax_tbl)) {
				s_cpu_subtype = cpu_subtype_vax_tbl[cpu_subtype];
			}
			break;
		}

		case CPU_TYPE_MC680x0: {
			static const char *const cpu_subtype_m68k_tbl[] = {
				nullptr, nullptr, "MC68040", "MC68030"
			};
			if (cpu_subtype < ARRAY_SIZE(cpu_subtype_m68k_tbl)) {
				s_cpu_subtype = cpu_subtype_m68k_tbl[cpu_subtype];
			}
			break;
		}

		case CPU_TYPE_I386: {
			// 32-bit
			if (!abi) {
				switch (cpu_subtype & 0xF) {
					default:
						break;
					case CPU_SUBTYPE_386:
						s_cpu_subtype = "i386";
						break;
					case CPU_SUBTYPE_486:
						s_cpu_subtype = "i486";
						break;
					case CPU_SUBTYPE_486SX:
						s_cpu_subtype = "i486SX";
						break;
					case CPU_SUBTYPE_PENT:
						s_cpu_subtype = "Pentium";
						break;

					case CPU_SUBTYPE_INTEL(6, 0):
						// i686 class
						switch (cpu_subtype >> 4) {
							default:
							case 0:
								s_cpu_subtype = "i686";
								break;
							case 1:
								s_cpu_subtype = "Pentium Pro";
								break;
							case 2:
								s_cpu_subtype = "Pentium II (M2)";
								break;
							case 3:
								s_cpu_subtype = "Pentium II (M3)";
								break;
							case 4:
								s_cpu_subtype = "Pentium II (M4)";
								break;
							case 5:
								s_cpu_subtype = "Pentium II (M5)";
								break;
						}
						break;

					case CPU_SUBTYPE_CELERON:
						// Celeron
						if (cpu_subtype != CPU_SUBTYPE_CELERON_MOBILE) {
							s_cpu_subtype = "Celeron";
						} else {
							s_cpu_subtype = "Celeron (Mobile)";
						}
						break;

					case CPU_SUBTYPE_PENTIII:
						// Pentium III
						switch (cpu_subtype >> 4) {
							default:
							case 0:
								s_cpu_subtype = "Pentium III";
								break;
							case 1:
								s_cpu_subtype = "Pentium III-M";
								break;
							case 2:
								s_cpu_subtype = "Pentium III Xeon";
								break;
						}
						break;

					case CPU_SUBTYPE_PENTIUM_M:
						s_cpu_subtype = "Pentium M";
						break;

					case CPU_SUBTYPE_PENTIUM_4:
						s_cpu_subtype = "Pentium 4";
						break;

					case CPU_SUBTYPE_ITANIUM:
						if (cpu_subtype == CPU_SUBTYPE_ITANIUM_2) {
							s_cpu_subtype = "Itanium 2";
						} else {
							s_cpu_subtype = "Itanium";
						}
						break;

					case CPU_SUBTYPE_XEON:
						if (cpu_subtype != CPU_SUBTYPE_XEON_MP) {
							s_cpu_subtype = "Xeon MP";
						} else {
							s_cpu_subtype = "Xeon MP";
						}
						break;
				}
			} else {
				switch (cpu_subtype) {
					default:
						break;
					case CPU_SUBTYPE_AMD64_ARCH1:
						s_cpu_subtype = "arch1";
						break;
					case CPU_SUBTYPE_AMD64_HASWELL:
						s_cpu_subtype = "Haswell";
						break;
				}
			}

			break;
		}

		case CPU_TYPE_MIPS: {
			static const char *const cpu_subtype_mips_tbl[] = {
				nullptr, "R2300", "R2600", "R2800",
				"R2000a", "R2000", "R3000a", "R3000"
			};
			if (cpu_subtype < ARRAY_SIZE(cpu_subtype_mips_tbl)) {
				s_cpu_subtype = cpu_subtype_mips_tbl[cpu_subtype];
			}
			break;
		}

		case CPU_TYPE_MC98000:
			if (cpu_subtype == CPU_SUBTYPE_MC98601) {
				s_cpu_subtype = "MC98601";
			}
			break;

		case CPU_TYPE_HPPA: {
			static const char *const cpu_subtype_hppa_tbl[] = {
				nullptr, "HP/PA 7100", "HP/PA 7100LC"
			};
			if (cpu_subtype < ARRAY_SIZE(cpu_subtype_hppa_tbl)) {
				s_cpu_subtype = cpu_subtype_hppa_tbl[cpu_subtype];
			}
			break;
		}

		case CPU_TYPE_MC88000: {
			static const char *const cpu_subtype_m88k_tbl[] = {
				nullptr, "MC88100", "MC88110"
			};
			if (cpu_subtype < ARRAY_SIZE(cpu_subtype_m88k_tbl)) {
				s_cpu_subtype = cpu_subtype_m88k_tbl[cpu_subtype];
			}
			break;
		}

		case CPU_TYPE_ARM: {
			if (abi && cpu_subtype == CPU_SUBTYPE_ARM64_V8) {
				s_cpu_subtype = "ARMv8";
			} else if (!abi) {
				static const char *const cpu_subtype_arm_tbl[] = {
					nullptr, nullptr, nullptr, nullptr,
					nullptr, "ARMv4T", "ARMv6", "ARMv5TEJ",
					"XScale", "ARMv7", "ARMv7f", "ARMv7s",
					"ARMv7k", "ARMv8", "ARMv6-M", "ARMv7-M",
					"ARMv7E-M"
				};
				if (cpu_subtype < ARRAY_SIZE(cpu_subtype_arm_tbl)) {
					s_cpu_subtype = cpu_subtype_arm_tbl[cpu_subtype];
				}
			}
			break;
		}

		case CPU_TYPE_POWERPC: {
			static const char *const cpu_subtype_ppc_tbl[] = {
				nullptr, "601", "602", "603",
				"603e", "603ev", "604", "604e",
				"620", "750", "7400", "7450"
			};
			if (cpu_subtype < ARRAY_SIZE(cpu_subtype_ppc_tbl)) {
				s_cpu_subtype = cpu_subtype_ppc_tbl[cpu_subtype];
			} else if (cpu_subtype == CPU_SUBTYPE_POWERPC_970) {
				s_cpu_subtype = "970";
			}
			break;
		}
	}

	if (s_cpu_subtype) {
		d->fields->addField_string(C_("Mach-O", "CPU Subtype"), s_cpu_subtype);
	}

	// TODO: Flags.

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

}
